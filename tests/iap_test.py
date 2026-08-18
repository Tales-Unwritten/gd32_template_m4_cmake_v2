#!/usr/bin/env python3
"""
IAP 升级协议验证工具（M5 硬件实测用）

用法：
  1. 板子进入 boot 升级模式（按住 PC13 按键复位，或 app 发 iap_upgrade 命令后复位）
  2. 查询状态：      python3 tests/iap_test.py /dev/ttyACM0 query
  3. 完整升级：      python3 tests/iap_test.py /dev/ttyACM0 upgrade --file build/iap/iap_gd32f470_app_b.bin
  4. 单命令：        python3 tests/iap_test.py /dev/ttyACM0 start --len 22964 --crc32 0x09BBA3D4 --slot B
                     python3 tests/iap_test.py /dev/ttyACM0 activate

协议（与 protocol/iap/inc/iap_conf.hpp 一致）：
  帧：0xA5 0x5A + CMD + LEN(2 LE) + PAYLOAD + CRC16(2 LE, Modbus)
  应答负载：[status(1)][next_seq(2)][extra...]
"""

import argparse
import struct
import sys
import time
import zlib

import serial

HEAD0, HEAD1 = 0xA5, 0x5A
CMD = {"QUERY": 0x01, "START": 0x02, "DATA": 0x03,
       "END": 0x04, "ACTIVATE": 0x05, "ABORT": 0x06, "REBOOT": 0x07}
ACK_NAMES = {0: "OK", 1: "ERR_PARAM", 2: "ERR_SLOT", 3: "ERR_CRC",
             4: "ERR_STATE", 5: "ERR_BUSY", 0x10: "NEED_RESTART"}
STATE_NAMES = {0: "IDLE", 1: "REQ_UPGRADE", 2: "TRANSFERRING", 3: "PENDING_ACTIVATE"}
HEADER_SIZE = 64
PAYLOAD_MAX = 256


def crc16(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = ((crc >> 1) ^ 0xA001) if (crc & 1) else (crc >> 1)
    return crc


def build_frame(cmd: int, payload: bytes = b"") -> bytes:
    body = bytes([HEAD0, HEAD1, cmd]) + struct.pack("<H", len(payload)) + payload
    return body + struct.pack("<H", crc16(body))


def recv_frame(ser, timeout: float = 3.0):
    """接收一帧（帧头定位 + 按 LEN 收全 + CRC 校验），失败返回 None"""
    ser.timeout = 0.1
    buf = b""
    end = time.time() + timeout
    while time.time() < end:
        buf += ser.read(64)
        idx = buf.find(bytes([HEAD0, HEAD1]))
        if idx > 0:
            buf = buf[idx:]
        if idx < 0 and len(buf) > 1:
            buf = buf[-1:]
        if buf and len(buf) >= 5:
            plen = buf[3] | (buf[4] << 8)
            need = 5 + plen + 2
            while len(buf) < need and time.time() < end:
                buf += ser.read(need - len(buf))
            if len(buf) >= need:
                frame = buf[:need]
                if (frame[-2] | (frame[-1] << 8)) == crc16(frame[:-2]):
                    return frame
                buf = buf[1:]
    return None


def send_frame(ser, cmd: int, payload: bytes = b"", label: str = "", timeout: float = 3.0) -> dict:
    ser.write(build_frame(cmd, payload))
    frame = recv_frame(ser, timeout)
    if frame is None:
        print(f"[{label or cmd}] 无应答（超时）")
        return {"status": None}
    status = frame[5]
    next_seq = frame[6] | (frame[7] << 8)
    extra = frame[8:]
    print(f"[{label or cmd}] status={status}({ACK_NAMES.get(status, '?')}) next_seq={next_seq} extra={extra.hex() if extra else '-'}")
    return {"status": status, "next_seq": next_seq, "extra": extra}


def cmd_query(ser):
    r = send_frame(ser, CMD["QUERY"], label="QUERY")
    if r["status"] == 0 and len(r["extra"]) >= 15:
        state, active, pending = r["extra"][0], chr(r["extra"][1]), chr(r["extra"][2])
        boot_ver, act_ver, oth_ver = struct.unpack("<III", r["extra"][3:15])
        print(f"  state={STATE_NAMES.get(state, state)} active={active} pending={pending}")
        print(f"  boot_ver={boot_ver >> 16}.{(boot_ver >> 8) & 0xFF}.{boot_ver & 0xFF}")
        print(f"  active_ver={act_ver >> 16}.{(act_ver >> 8) & 0xFF}.{act_ver & 0xFF}")
        print(f"  other_ver ={oth_ver >> 16}.{(oth_ver >> 8) & 0xFF}.{oth_ver & 0xFF}")


def cmd_upgrade(ser, path: str):
    with open(path, "rb") as f:
        image = f.read()
    magic, version, platform, image_len, crc32, slot = struct.unpack("<IIIIIB", image[:21])
    assert magic == 0x31504149, "文件不是 IAP 升级包（magic 错误）"
    assert image_len == len(image), "文件长度与头部不一致"
    slot_ch = chr(slot)
    print(f"镜像: slot={slot_ch} version={version >> 16}.{(version >> 8) & 0xFF}.{version & 0xFF} "
          f"len={image_len} crc32=0x{crc32:08X}")

    # 1. START（擦除整个目标分区需数秒，应答超时放宽）
    payload = struct.pack("<IIIB", version, image_len, crc32, slot)
    r = send_frame(ser, CMD["START"], payload, label="START", timeout=20.0)
    if r["status"] != 0:
        print("START 失败，终止")
        return 1

    # 2. DATA 分包（256B，末包短；包 0 含 64B 镜像头部，从分区基址写入）
    payload_data = image
    total_pkts = (len(payload_data) + PAYLOAD_MAX - 1) // PAYLOAD_MAX
    for i in range(total_pkts):
        chunk = payload_data[i * PAYLOAD_MAX:(i + 1) * PAYLOAD_MAX]
        seq = i
        for attempt in range(3):
            r = send_frame(ser, CMD["DATA"], struct.pack("<H", seq) + chunk,
                           label=f"DATA[{i}/{total_pkts}]")
            if r["status"] == 0 and r["next_seq"] == seq + 1:
                break
            if r["status"] is None:
                print("  DATA 无应答，重试")
                time.sleep(0.1)
            elif r["next_seq"] == seq:
                print("  DATA 被拒，重试")
        else:
            print(f"DATA[{i}] 重试 3 次失败，终止")
            return 1

    # 3. END（全量校验）
    r = send_frame(ser, CMD["END"], label="END")
    if r["status"] != 0:
        print("END 校验未通过，终止")
        return 1

    # 4. ACTIVATE（切换并复位）
    r = send_frame(ser, CMD["ACTIVATE"], label="ACTIVATE")
    print("升级完成，板子即将复位运行新固件")
    return 0


def main():
    ap = argparse.ArgumentParser(description="IAP upgrade protocol test tool")
    ap.add_argument("port", help="serial port (e.g. /dev/ttyACM0)")
    ap.add_argument("action", choices=["query", "upgrade", "start", "end",
                                       "activate", "abort", "reboot"])
    ap.add_argument("--file", help="upgrade image path (for 'upgrade')")
    ap.add_argument("--len", type=int, help="image length (for 'start')")
    ap.add_argument("--crc32", type=lambda s: int(s, 0), help="payload crc32 (for 'start')")
    ap.add_argument("--slot", choices=["A", "B"], help="target slot (for 'start')")
    ap.add_argument("--version", default="1.0.0", help="version MAJOR.MINOR.PATCH (for 'start')")
    ap.add_argument("--baud", type=int, default=115200)
    args = ap.parse_args()

    ser = serial.Serial(args.port, args.baud, timeout=0.1)
    ser.reset_input_buffer()

    if args.action == "query":
        cmd_query(ser)
    elif args.action == "upgrade":
        if not args.file:
            sys.exit("--file required for 'upgrade'")
        sys.exit(cmd_upgrade(ser, args.file))
    elif args.action == "start":
        if not (args.len and args.crc32 is not None and args.slot):
            sys.exit("--len/--crc32/--slot required for 'start'")
        major, minor, patch = (int(x) for x in args.version.split("."))
        ver = (major << 16) | (minor << 8) | patch
        payload = struct.pack("<IIIB", ver, args.len, args.crc32, ord(args.slot))
        send_frame(ser, CMD["START"], payload, label="START")
    else:
        send_frame(ser, CMD[args.action.upper()], label=args.action.upper())

    ser.close()


if __name__ == "__main__":
    main()
