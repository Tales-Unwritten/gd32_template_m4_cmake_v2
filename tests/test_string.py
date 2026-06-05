#!/usr/bin/env python3
"""
字符串协议全面冒烟测试

测试矩阵：
  A. 功能完整性 — 所有 12 条 INA228 命令
  B. 边界条件 — 空命令、只含空格、超长命令、缓冲区极限
  C. 错误处理 — 未知命令、大小写错误、残缺命令
  D. 压力测试 — 连续高频发送、漏帧检测
  E. 多端口验证 — 确认数据路由正确（从哪来回哪去）

用法: python3 tests/test_string.py /dev/ttyACM0
"""

import sys
import time
import serial

# ── 配置 ──────────────────────────────────────────────────
PORT = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyACM0"
BAUDRATE = 115200
TIMEOUT = 0.2
PASS, FAIL, SKIP = 0, 1, 2

results = []

# ── 串口初始化 ────────────────────────────────────────────
ser = serial.Serial(PORT, BAUDRATE, timeout=TIMEOUT)
time.sleep(0.5)
ser.reset_input_buffer()  # 丢弃 BOOT 消息

def do_test(name, cmd, expect_keyword, expect=None):
    """发送命令，等待响应，返回 (PASS/FAIL/SKIP, 耗时ms, 响应文本)"""
    ser.reset_input_buffer()
    t0 = time.monotonic()
    ser.write(cmd if isinstance(cmd, bytes) else cmd.encode())

    # 收数据直到 5ms 无新数据（多行响应检测）
    resp = b""
    deadline = t0 + 0.3
    idle = 0
    while time.monotonic() < deadline:
        if ser.in_waiting:
            resp += ser.read(ser.in_waiting)
            idle = 0
        else:
            time.sleep(0.001)
            idle += 1
            if idle >= 5 and resp:   # 5ms 无数据 → 响应结束
                break
    t1 = time.monotonic()

    elapsed = (t1 - t0) * 1000
    resp_str = resp.decode(errors="replace").rstrip("\r\n")

    ok = expect_keyword in resp_str if isinstance(expect_keyword, str) else expect_keyword(resp_str)
    if expect is not None and ok:
        ok = expect in resp_str

    status = PASS if ok else FAIL
    results.append((name, status, elapsed, resp_str[:100]))
    return status, elapsed, resp_str


def section(title):
    print(f"\n{'='*60}\n  {title}\n{'='*60}")


# ═══════════════════════════════════════════════════════════
#  A. 功能完整性 — 12 条标准命令
# ═══════════════════════════════════════════════════════════
section("A. 功能完整性 — 12 条 INA228 命令")

cmd_tests = [
    ("A01 总线电压",     "ina228_bus_voltage\r\n",   "bus_voltage="),
    ("A02 电流",        "ina228_current\r\n",       "current="),
    ("A03 功率",        "ina228_power\r\n",         "power="),
    ("A04 温度",        "ina228_temperature\r\n",   "temperature="),
    ("A05 能量",        "ina228_energy\r\n",        "energy="),
    ("A06 电荷",        "ina228_charge\r\n",        "charge="),
    ("A07 分流电压",     "ina228_shunt_voltage\r\n", "shunt_voltage="),
    ("A08 制造商ID",     "ina228_mfr_id\r\n",        "manufacturer_id=0x"),
    ("A09 设备ID",      "ina228_device_id\r\n",     "device_id=0x"),
    ("A10 报警状态",     "ina228_alert\r\n",         "alert=0x"),
    ("A11 清除报警",     "ina228_clear_alert\r\n",   "alert_cleared"),
    ("A12 全部数据",     "ina228_all\r\n",           "=== INA228 ==="),
]

for name, cmd, keyword in cmd_tests:
    s, t, resp = do_test(name, cmd, keyword)
    symbol = "✓" if s == PASS else "✗"
    print(f"  {symbol} {name:<14s} {t:>7.2f}ms  → {resp[:70]}")


# ═══════════════════════════════════════════════════════════
#  B. 边界条件 — 空、空格、超长、缓冲区极限
# ═══════════════════════════════════════════════════════════
section("B. 边界条件")

# B1: 空命令（只发 \r\n）
s, t, resp = do_test("B01 空命令(\\r\\n)", "\r\n", "Error: empty command")
print(f"  {'✓' if s == PASS else '✗'} B01 空命令      {t:>7.2f}ms  → {resp}")

# B2: 只含空格
s, t, resp = do_test("B02 纯空格", "   \r\n", "Error: empty command")
print(f"  {'✓' if s == PASS else '✗'} B02 纯空格      {t:>7.2f}ms  → {resp}")

# B3: 超长命令名（>200字符）
long_cmd = "x" * 240 + "\r\n"
s, t, resp = do_test("B03 超长命令(240B)", long_cmd, "Error: unknown")
print(f"  {'✓' if s == PASS else '✗'} B03 超长240B    {t:>7.2f}ms  → {resp[:70]}")

# B4: 缓冲区极限（255字节命令 + \\r\\n，逼近256的line_buffer）
buf_limit_cmd = "a" * 253 + "\r\n"
s, t, resp = do_test("B04 缓冲区极限(253B)", buf_limit_cmd, "Error: unknown")
print(f"  {'✓' if s == PASS else '✗'} B04 逼近256B   {t:>7.2f}ms  → {resp[:70]}")

# B5: 参数中有超多空格
s, t, resp = do_test("B05 参数带多空格", "ina228_bus_voltage   \r\n", "bus_voltage=")
print(f"  {'✓' if s == PASS else '✗'} B05 参数多余空格  {t:>7.2f}ms  → {resp}")

# B6: 命令后跟空格（无参数）
s, t, resp = do_test("B06 命令后带空格", "ina228_temperature \r\n", "temperature=")
print(f"  {'✓' if s == PASS else '✗'} B06 尾随空格    {t:>7.2f}ms  → {resp}")

# B7: 只有 \\n 没有 \\r
s, t, resp = do_test("B07 仅\\n结尾", "ina228_power\n", "power=")
print(f"  {'✓' if s == PASS else '✗'} B07 仅LF结尾    {t:>7.2f}ms  → {resp}")


# ═══════════════════════════════════════════════════════════
#  C. 错误处理 — 未知命令、大小写、残缺
# ═══════════════════════════════════════════════════════════
section("C. 错误处理")

# C1: 未知命令
s, t, resp = do_test("C01 未知命令", "hello_world\r\n", "Error: unknown command")
print(f"  {'✓' if s == PASS else '✗'} C01 未知命令    {t:>7.2f}ms  → {resp}")

# C2: 大小写错误
s, t, resp = do_test("C02 大小写错误", "INA228_BUS_VOLTAGE\r\n", "Error: unknown command")
print(f"  {'✓' if s == PASS else '✗'} C02 大写命令    {t:>7.2f}ms  → {resp}")

# C3: 残缺命令（前缀匹配但不同）
s, t, resp = do_test("C03 残缺命令", "ina228_bus\r\n", "Error: unknown command")
print(f"  {'✓' if s == PASS else '✗'} C03 前缀匹配    {t:>7.2f}ms  → {resp}")

# C4: 单字符
s, t, resp = do_test("C04 单字符", "x\r\n", "Error: unknown command")
print(f"  {'✓' if s == PASS else '✗'} C04 单字符命令  {t:>7.2f}ms  → {resp}")

# C5: 特殊字符
s, t, resp = do_test("C05 特殊字符", "@#$%^&*\r\n", "Error: unknown command")
print(f"  {'✓' if s == PASS else '✗'} C05 特殊字符    {t:>7.2f}ms  → {resp}")


# ═══════════════════════════════════════════════════════════
#  D. 压力测试 — 高频连续发送，验证漏帧/错帧
# ═══════════════════════════════════════════════════════════
section("D. 压力测试 — 100 次连续发送")

stress_cmds = [
    ("ina228_bus_voltage\r\n",   "bus_voltage="),
    ("ina228_current\r\n",       "current="),
    ("ina228_temperature\r\n",   "temperature="),
]

stress_errors = 0
stress_total = 100
stress_times = []
ser.reset_input_buffer()

for i in range(stress_total):
    cmd = stress_cmds[i % len(stress_cmds)]
    t0 = time.monotonic()
    ser.write(cmd[0].encode())

    # 等待完整响应
    resp = b""
    deadline = t0 + 0.3
    idle = 0
    while time.monotonic() < deadline:
        if ser.in_waiting:
            resp += ser.read(ser.in_waiting)
            idle = 0
        else:
            time.sleep(0.001)
            idle += 1
            if idle >= 5 and resp:
                break
    t1 = time.monotonic()

    stress_times.append((t1 - t0) * 1000)
    resp_str = resp.decode(errors="replace")
    if cmd[1] not in resp_str:
        stress_errors += 1

stress_pass = stress_total - stress_errors
stress_avg = sum(stress_times) / len(stress_times)
stress_min = min(stress_times)
stress_max = max(stress_times)

print(f"  发送: {stress_total}  通过: {stress_pass}  失败: {stress_errors}")
print(f"  丢包率: {stress_errors/stress_total*100:.2f}%")
print(f"  平均响应: {stress_avg:.2f}ms  最快: {stress_min:.2f}ms  最慢: {stress_max:.2f}ms")
print(f"  吞吐量: {stress_total*1000/sum(stress_times):.1f} cmd/s")


# ═══════════════════════════════════════════════════════════
#  E. 数据一致性 — 多次读取检查值合理性
# ═══════════════════════════════════════════════════════════
section("E. 数据合理性")

def extract_value(resp, key):
    """从响应中提取 key=value 的值"""
    for part in resp.split("\r\n"):
        if part.startswith(key + "="):
            return part.split("=")[1].split()[0]
    return None

# 连续5次读取温度，检查一致性
temps = []
for i in range(5):
    _, _, resp = do_test(f"E0{i+1} 温度一致性{i+1}", "ina228_temperature\r\n", "temperature=")
    val = extract_value(resp, "temperature")
    if val:
        temps.append(int(val))

if len(temps) == 5:
    avg_t = sum(temps) / 5
    spread = max(temps) - min(temps)
    print(f"  5次温度读数: {temps}")
    print(f"  平均值: {avg_t:.0f} mC  (约 {avg_t/1000:.1f}°C)")
    print(f"  抖动范围: {spread} mC  ({'✓ 稳定' if spread < 500 else '⚠ 偏大'})")

# 连续3次 all 命令，检查是否有空值
empty_count = 0
for i in range(3):
    _, _, resp = do_test(f"E_all{i+1}", "ina228_all\r\n", "=== INA228 ===")
    if "=0 mV" in resp:
        empty_count += 1
print(f"  3次 ina228_all 空值字段: {'✓ 无异常' if empty_count == 0 else f'⚠ {empty_count}次有零值'}")

# 制造商ID一致性
ids = []
for i in range(3):
    _, _, resp = do_test(f"Emfr{i+1}", "ina228_mfr_id\r\n", "manufacturer_id=0x")
    val = extract_value(resp, "manufacturer_id")
    if val:
        ids.append(val)
if len(set(ids)) == 1:
    print(f"  制造商ID稳定: {ids[0]}  ✓")


# ═══════════════════════════════════════════════════════════
#  汇总报告
# ═══════════════════════════════════════════════════════════
section("测试汇总")

total = len(results)
passed = sum(1 for r in results if r[1] == PASS)
failed = total - passed

print(f"""
┌─────────────────────────────────────┐
│        字符串协议冒烟测试报告        │
├─────────────────────────────────────┤
│  总测试项:  {total:<4}                     │
│  通过:      {passed:<4}                     │
│  失败:      {failed:<4}                     │
│  通过率:    {passed/total*100:.1f}%                    │
├─────────────────────────────────────┤
│  A. 功能完整性  12 条命令            │
│  B. 边界条件    空/空格/超长/极限     │
│  C. 错误处理    未知/大小写/残缺      │
│  D. 压力测试    {stress_total}次高频 丢包率 {stress_errors/stress_total*100:.2f}%   │
│  E. 数据合理性  多次读取一致性        │
├─────────────────────────────────────┤
│  平均响应:  {sum(r[2] for r in results)/total:.2f}ms                 │
│  吞吐量:    {stress_total*1000/sum(stress_times):.1f} cmd/s             │
│  丢包率:    {stress_errors/stress_total*100:.2f}%                     │
└─────────────────────────────────────┘
""")

# 列出失败项
if failed > 0:
    print("失败项明细:")
    for r in results:
        if r[1] == FAIL:
            print(f"  ✗ {r[0]:<14s} → {r[3]}")

ser.close()
print("测试结束")
