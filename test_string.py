#!/usr/bin/env python3
"""
ASCII 字符串指令协议验证脚本
用法: python3 test_string.py /dev/ttyACM1
"""

import sys
import time

import serial

tests = [
    ("ina228_bus_voltage\r\n", "BusVoltage"),
    ("ina228_shunt_voltage\r\n", "ShuntVoltage"),
    ("ina228_current\r\n", "Current"),
    ("ina228_power\r\n", "Power"),
    ("ina228_temperature\r\n", "Temperature"),
    ("ina228_energy\r\n", "Energy"),
    ("ina228_charge\r\n", "Charge"),
    ("ina228_mfr_id\r\n", "MfrID=0x"),
    ("ina228_device_id\r\n", "DeviceID=0x"),
    ("ina228_clear_alert\r\n", "Cleared"),
    ("ina228_all\r\n", "=== INA228 ==="),
]

port = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyACM1"
ser = serial.Serial(port, 115200, timeout=0.5)

passed = 0
for cmd, keyword in tests:
    ser.write(cmd.encode())
    time.sleep(0.15)
    resp = ser.read(512).decode(errors="replace").strip()
    ok = keyword in resp
    if ok:
        passed += 1
    print(f"[{'PASS' if ok else 'FAIL'}] {cmd.strip()} → {resp[:80]}")

ser.close()
print(f"\n=== {passed}/{len(tests)} 通过 ===")
