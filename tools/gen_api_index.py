#!/usr/bin/env python3
"""从 freertos/README.md 提取「已启用(✅)」的 API，生成
code/src/freertos_api_index.c —— FreeRTOS API 索引（速查 + 编辑器跳转）。

每条形如：
    /// xQueueCreate()                          动态创建队列
    xQueueCreate(0, 0);                        /* 代码引用，保证转到定义可跳 */

设计：
  - `/// 名字()` 行：clangd 支持对文档注释内符号名转到定义（速度/版本有差异）。
  - 紧跟的代码引用行：标准转到定义，任何 clangd 都能跳，作为兜底。
  - 所有引用放进一个从不调用的 static inline 函数 → 编译后零代码/零数据。
  - 函数 → (void) &名字;   宏 → 名字(0,…);

在项目根目录运行；输出路径可用环境变量 OUT 覆盖（便于测试）。
"""
import re, glob, os

ROOT = 'freertos'
README = os.path.join(ROOT, 'README.md')
OUT = os.environ.get('OUT', 'code/src/freertos_api_index.c')

HEADERS = sorted(glob.glob(f'{ROOT}/include/*.h')) + sorted(glob.glob(f'{ROOT}/portable/**/*.h', recursive=True))
CS = (sorted(glob.glob(f'{ROOT}/*.c'))
      + [f'{ROOT}/portable/MemMang/heap_4.c']
      + sorted(glob.glob(f'{ROOT}/portable/GCC/**/*.c', recursive=True))
      + sorted(glob.glob(f'{ROOT}/portable/**/*.c', recursive=True)))
CTRL = {'return', 'if', 'while', 'for', 'switch', 'else', 'do', 'break',
        'continue', 'case', 'default', 'goto', 'sizeof'}


def has_c_def(name):
    esc = re.escape(name)
    for p in CS:
        for line in open(p, errors='ignore'):
            m = re.match(r'^\s*([A-Za-z_][A-Za-z0-9_ \t\*]*)\b' + esc + r'\s*\(', line)
            if m and m.group(1).strip() not in CTRL:
                return True
    return False


def find_macro(name):
    esc = re.escape(name)
    for h in HEADERS:
        if os.path.basename(h) in ('mpu_wrappers.h', 'deprecated_definitions.h'):
            continue
        lines = open(h, errors='ignore').read().split('\n')
        for i, l in enumerate(lines):
            m = re.match(r'\s*#\s*define\s+' + esc + r'\b(.*)$', l)
            if not m:
                continue
            body = m.group(1)
            if body.strip().startswith('('):
                c = body.find(')')
                if c < 0 or body[c + 1:].strip() == '':
                    continue
            j = i
            while lines[j].rstrip().endswith('\\') and j + 1 < len(lines):
                j += 1
                body += ' ' + lines[j]
            return body
    return None


def arity(name):
    b = (find_macro(name) or '').lstrip()
    if not b.startswith('('):
        return None
    d = 0
    for i, ch in enumerate(b):
        if ch == '(':
            d += 1
        elif ch == ')':
            d -= 1
            if d == 0:
                p = b[1:i].strip()
                return 0 if p == '' else len(p.split(','))
    return 0


def ref(name):
    if has_c_def(name):
        return f'(void) &{name};'
    a = arity(name)
    if a is None:
        return None
    return f'{name}(' + ', '.join(['0'] * a) + ');'


# ── 解析 README ─────────────────────────────────────────────
sections = []
line_re = re.compile(r'^(?:\[([^\]\s]+)\]\([^)]*\)|(\S+))(\s+///\s+)(.*)$')
for raw in open(README, errors='ignore').read().split('\n'):
    h = re.match(r'^###\s+(.+)$', raw)
    if h:
        sections.append((re.sub(r'\s*—.*$', '', h.group(1).strip()), []))
        continue
    m = line_re.match(raw)
    if m and sections:
        token = m.group(1) or m.group(2)
        desc = m.group(4).strip()
        if not desc.startswith('✅'):
            continue
        sections[-1][1].append((re.sub(r'\(.*\)$', '', token), desc[1:].strip()))

# ── 生成 ────────────────────────────────────────────────────
body, total = [], 0
for title, items in sections:
    if not items:
        continue
    body += ['', f'/* ══════════════════════════════════════════════════════════════',
             f' *  {title}', f' * ══════════════════════════════════════════════════════════════ */']
    for name, desc in items:
        body.append(f'/// {name}()'.ljust(45) + desc)
        body.append(ref(name) or f'/* {name}: 未找到定义/宏 */')
        total += 1

content = f'''/*
 * freertos_api_index.c — FreeRTOS API 索引（速查 + 编辑器跳转）
 * ---------------------------------------------------------------------------
 * 用法：对任意 API 名做「转到定义」（Zed: Cmd/Ctrl+点击）即可跳到它的声明/定义。
 *       每条 `/// 名字()` 下都有一行代码引用作为兜底（任何 clangd 都能跳）。
 *       想补说明，在 `/// 名字()` 行内或下面继续加即可。
 *
 * ★ 引用放在从不调用的 static inline 函数内，编译后零代码/零数据（nm 已验证）★
 * 由脚本自动生成（tools/gen_api_index.py）；收录 code/inc/FreeRTOSConfig.h 已启用的 API。
 * ---------------------------------------------------------------------------
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"
#include "event_groups.h"
#include "stream_buffer.h"
#include "message_buffer.h"

/* 下面这些引用仅为「保证转到定义可跳转」而存在；本函数从不被调用，编译为空。 */
static inline void freertos_api_index_( void )
{{''' + '\n'.join(body) + '\n}\n'

open(OUT, 'w').write(content)
print(f'wrote {OUT}: {total} APIs')
