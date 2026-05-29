#!/bin/bash
# 协议切换 + 编译 + 烧录
# 用法: ./switch_protocol.sh modbus  或  ./switch_protocol.sh string

MODE=${1:-modbus}
HEADER="App/inc/protocol.hpp"

case $MODE in
    modbus|Modbus|MODBUS)
        sed -i 's/ACTIVE_PROTOCOL  PROTOCOL_STRING_CMD/ACTIVE_PROTOCOL  PROTOCOL_MODBUS/' $HEADER
        echo "→ Modbus RTU"
        ;;
    string|String|STRING)
        sed -i 's/ACTIVE_PROTOCOL  PROTOCOL_MODBUS/ACTIVE_PROTOCOL  PROTOCOL_STRING_CMD/' $HEADER
        echo "→ ASCII String"
        ;;
    *) echo "用法: $0 modbus|string"; exit 1 ;;
esac

# 强制重编译
touch App/src/protocol.cpp App/src/protocol_string.cpp user/src/founction.cpp
cmake --build --preset debug || exit 1

# 烧录
openocd -f interface/cmsis-dap.cfg \
    -c "transport select swd; adapter speed 1000" \
    -c "set CHIPNAME gd32f470; source [find target/stm32f4x.cfg]" \
    -c "program build/debug/gd32f470_template.elf verify reset exit" \
    2>&1 | grep -E "Program|Verif|Error"

echo "Done."
