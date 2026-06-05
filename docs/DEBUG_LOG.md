# Modbus 调试速查表

## 通信参数

| 参数 | 值 |
|------|-----|
| 从机地址 | 0x01 |
| 波特率 | 115200 |
| 数据位 | 8 |
| 停止位 | 1 |
| 校验 | 无 (NONE) |
| 串口 | USART0 (PA9 TX / PA10 RX) |
| CRC | CRC-16 Modbus (多项式 0x8005) |

## 支持的功能码

| 功能码 | 名称 | 状态 |
|--------|------|------|
| 0x03 | 读保持寄存器 (Read Holding Register) | ✅ |
| 0x04 | 读输入寄存器 (Read Input Register) | ✅ |
| 0x06 | 写单个寄存器 (Write Single Register) | ✅ |
| 0x10 | 写多个寄存器 (Write Multiple Registers) | ✅ |

## 寄存器映射

### HOLDING 寄存器（功能码 0x03 / 0x06 / 0x10）

| 地址 | 名称 | 类型 | 大小 | 说明 |
|------|------|------|------|------|
| 0x0000 | DeviceModel | uint16 | 1 | 设备型号，默认 1 |
| 0x0001 | FirmwareVersion | uint16 | 1 | 固件版本，默认 100 |
| 0x0002-0x0009 | Reserved | uint16 | 8 | 保留 |
| 0x000A-0x003B | ControlBlock[0..49] | uint16 | 50 | 控制参数，可读写 |

### INPUT 寄存器（功能码 0x04，只读）

| 地址 | 名称 | 单位 | 大小 | 说明 |
|------|------|------|------|------|
| 0x0040-0x0041 | BusVoltage | μV | uint32 | 总线电压 |
| 0x0042-0x0043 | ShuntVoltage | μV | uint32 | 分流电压 |
| 0x0044-0x0045 | Current | mA | uint32 | 电流 |
| 0x0046-0x0047 | Power | mW | uint32 | 功率 |
| 0x0048-0x0049 | Temperature | m°C | uint32 | 温度 |
| 0x004A-0x004D | Energy | mJ | uint64 | 能量累计 |
| 0x004E-0x0051 | Charge | mC | uint64 | 电荷累计 |
| 0x0052 | ManufacturerID | - | uint16 | 制造商 ID (0x5449=TI) |
| 0x0053 | DeviceID | - | uint16 | 设备 ID |
| 0x0054 | AlertFlags | - | uint16 | 报警标志 |

## 快速测试命令

```bash
# 读取设备型号和固件版本（地址 0x0000，2 个寄存器）
$ python3 -c "
import serial, struct
ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
def crc16(d):
    c=0xFFFF
    for x in d:
        c^=x
        for _ in range(8): c=(c>>1)^0xA001 if c&1 else c>>1
    return c&0xFFFF
# 0x03 读保持寄存器
f=struct.pack('>BBHH',0x01,0x03,0x0000,2)
f+=struct.pack('<H',crc16(f))
ser.write(f);time.sleep(0.1)
r=ser.read(256)
print('Model:', (r[3]<<8)|r[4], '  FW:', (r[5]<<8)|r[6])
ser.close()
"

# 读取传感器数据（地址 0x0040，10 个寄存器）
$ python3 -c "
import serial, struct, time
ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
def crc16(d):
    c=0xFFFF
    for x in d:
        c^=x
        for _ in range(8): c=(c>>1)^0xA001 if c&1 else c>>1
    return c&0xFFFF
# 0x04 读输入寄存器
f=struct.pack('>BBHH',0x01,0x04,0x0040,10)
f+=struct.pack('<H',crc16(f))
ser.write(f);time.sleep(0.1)
r=ser.read(256)
if len(r)>=23:
    busv=(r[3]<<24)|(r[4]<<16)|(r[5]<<8)|r[6]
    cur=(r[7]<<24)|(r[8]<<16)|(r[9]<<8)|r[10]
    pwr=(r[11]<<24)|(r[12]<<16)|(r[13]<<8)|r[14]
    temp=(r[15]<<24)|(r[16]<<16)|(r[17]<<8)|r[18]
    print(f'BusV={busv}uV  Cur={cur}mA  Pwr={pwr}mW  Temp={temp}mC')
ser.close()
"

# 写单个寄存器（地址 0x000A，值 1234）
$ python3 -c "
import serial, struct, time
ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
def crc16(d):
    c=0xFFFF
    for x in d:
        c^=x
        for _ in range(8): c=(c>>1)^0xA001 if c&1 else c>>1
    return c&0xFFFF
# 0x06 写单个
f=struct.pack('>BBHH',0x01,0x06,0x000A,1234)
f+=struct.pack('<H',crc16(f))
ser.write(f);time.sleep(0.1)
print('Write: ser.write OK')
# 读出验证
f=struct.pack('>BBHH',0x01,0x03,0x000A,1)
f+=struct.pack('<H',crc16(f))
ser.write(f);time.sleep(0.1)
r=ser.read(256)
print('Read back:', (r[3]<<8)|r[4] if len(r)>=5 else 'N/A')
ser.close()
"
```

## 帧格式

### 0x03 读保持寄存器

```
请求:  01 03 00 00 00 02 [CRC16]      ← 读 0x0000 起 2 个寄存器
响应:  01 03 04 [data..data] [CRC16]  ← 返回 4 字节数据 + CRC

字段说明:
  [0]     = 从机地址 (0x01)
  [1]     = 功能码 (0x03)
  [2]     = 起始地址高位
  [3]     = 起始地址低位
  [4]     = 寄存器数量高位
  [5]     = 寄存器数量低位
  [last-2]~[last-1] = CRC16

响应:
  [0]     = 从机地址 (0x01)
  [1]     = 功能码 (0x03)
  [2]     = 数据字节数
  [3]~[n] = 寄存器值（大端序）
  [last-2]~[last-1] = CRC16
```

### 0x04 读输入寄存器

```
请求:  01 04 00 40 00 0A [CRC16]      ← 读 0x0040 起 10 个寄存器
响应:  01 04 14 [data..data] [CRC16]  ← 返回 20 字节数据 + CRC
```

### 0x06 写单个寄存器

```
请求:  01 06 00 0A 04 D2 [CRC16]  ← 写 0x000A = 1234
响应:  01 06 00 0A 04 D2 [CRC16]  ← 回显写入值
```

### 0x10 写多个寄存器

```
请求:  01 10 00 0A 00 02 04 [data..data] [CRC16]  ← 写 0x000A 起 2 个寄存器
响应:  01 10 00 0A 00 02 [CRC16]                    ← 回显起始地址和数量
```

## 常用 Python 测试脚本

完整测试脚本：`tests/test_modbus.py`

```bash
python3 tests/test_modbus.py /dev/ttyACM0
```
