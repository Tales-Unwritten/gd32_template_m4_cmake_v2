#!/usr/bin/env python3
"""
Modbus RTU 协议验证脚本
用法: python3 test_modbus.py /dev/ttyACM1
"""

import struct
import sys
import time

import serial


def crc16(data):
    crc = 0xFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
    return crc


def mb_read(ser, addr, func, start, count):
    frame = struct.pack(">BBHH", addr, func, start, count)
    crc = crc16(frame)
    frame += struct.pack("<H", crc)
    ser.write(frame)
    time.sleep(0.1)
    return ser.read(256)


def test(port):
    ser = serial.Serial(port, 115200, timeout=0.5)
    errors = 0

    # ── 测试 1: 读 HOLDING 寄存器 ──
    resp = mb_read(ser, 0x01, 0x03, 0x0000, 2)
    if len(resp) >= 7:
        model, fw = struct.unpack(">HH", resp[3:7])
        ok = model == 1 and fw == 100
        print(f"[{'PASS' if ok else 'FAIL'}] 0x03 HOLDING: 型号={model} 固件={fw}")
        if not ok:
            errors += 1
    else:
        print(f"[FAIL] 0x03 无应答 ({len(resp)}B)")
        errors += 1

    # ── 测试 2: 读 INPUT 寄存器 ──
    resp = mb_read(ser, 0x01, 0x04, 0x0040, 10)
    if len(resp) >= 23:
        v = struct.unpack(">HHHHHHHHHH", resp[3:23])
        bus_v = (v[0] << 16) | v[1]
        ok = bus_v > 10000000  # 12V = 12000000 μV
        print(
            f"[{'PASS' if ok else 'FAIL'}] 0x04 INPUT: BusV={bus_v}uV Cur={v[4] << 16 | v[5]}mA Pwr={v[6] << 16 | v[7]}mW Temp={v[8] << 16 | v[9]}mC"
        )
        if not ok:
            errors += 1
    else:
        print(f"[FAIL] 0x04 无应答 ({len(resp)}B)")
        errors += 1

    # ── 测试 3: 写单个寄存器 ──
    mb_read(ser, 0x01, 0x06, 0x000A, 0x04D2)  # 写 1234
    time.sleep(0.05)
    resp = mb_read(ser, 0x01, 0x03, 0x000A, 1)  # 读回
    ok = len(resp) >= 5 and struct.unpack(">H", resp[3:5])[0] == 1234
    print(
        f"[{'PASS' if ok else 'FAIL'}] 0x06 写单个: 写1234→读回{struct.unpack('>H', resp[3:5])[0] if len(resp) >= 5 else '?'}"
    )
    if not ok:
        errors += 1
    mb_read(ser, 0x01, 0x06, 0x000A, 0)  # 恢复

    # ── 测试 4: 写多个寄存器 ──
    pdu = struct.pack(">BBHHB", 0x01, 0x10, 0x000A, 2, 4)
    pdu += struct.pack(">HH", 100, 200)
    ser.write(pdu + struct.pack("<H", crc16(pdu)))
    time.sleep(0.1)
    resp = mb_read(ser, 0x01, 0x03, 0x000A, 2)
    if len(resp) >= 7:
        v1, v2 = struct.unpack(">HH", resp[3:7])
        ok = v1 == 100 and v2 == 200
        print(f"[{'PASS' if ok else 'FAIL'}] 0x10 写多个: 写[100,200]→读回[{v1},{v2}]")
        if not ok:
            errors += 1
    else:
        print(f"[FAIL] 0x10 无应答 ({len(resp)}B)")
        errors += 1

    ser.close()
    print(f"\n{'=== 全部通过 ===' if errors == 0 else f'=== {errors} 项失败 ==='}")


if __name__ == "__main__":
    test(sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyACM1")
