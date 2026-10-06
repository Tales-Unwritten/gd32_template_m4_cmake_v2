#!/usr/bin/env python3
"""把 freertos/README.md 中形如 `name  /// ...` 的 API 行，
把 name 替换成指向**代码实现**（.c 优先）的 markdown 链接。

规则（按优先级）：
  1) 该名字在 .c 中的函数定义          → 链接到 .c
  2) 若是宏：解析宏体调用的函数，递归找 .c 定义（如 xQueueCreate → queue.c: xQueueGenericCreate）
  3) 头文件原型
  4) 兜底：宏自身的定义处（纯宏，无 .c 对应）
排除：MPU 别名头、空体桩宏；优先本工程实际编译的 heap_4.c / GCC 端口。
脚本幂等：已链接过的行会先还原再重新链接。
"""
import re, glob, os

ROOT = 'freertos'
README = os.path.join(ROOT, 'README.md')

HEADERS = sorted(glob.glob(f'{ROOT}/include/*.h')) + sorted(glob.glob(f'{ROOT}/portable/**/*.h', recursive=True))
MACRO_FILES = [h for h in HEADERS
               if os.path.basename(h) not in ('mpu_wrappers.h', 'deprecated_definitions.h')]

KERNEL_CS = sorted(glob.glob(f'{ROOT}/*.c'))
HEAP_CS = [f'{ROOT}/portable/MemMang/heap_4.c']
PORT_CS = sorted(glob.glob(f'{ROOT}/portable/GCC/**/*.c', recursive=True))
OTHER_CS = [c for c in sorted(glob.glob(f'{ROOT}/portable/**/*.c', recursive=True))
            if c not in HEAP_CS and c not in PORT_CS]
CS = KERNEL_CS + HEAP_CS + PORT_CS + OTHER_CS

CTRL = {'return', 'if', 'while', 'for', 'switch', 'else', 'do', 'break', 'continue',
        'case', 'default', 'goto', 'sizeof', 'configASSERT', 'configPRINTF'}
CAND_SKIP = CTRL | {'volatile', 'asm', 'static', 'const', 'inline', 'extern', 'struct',
                    'union', 'enum', 'unsigned', 'signed', 'long', 'short', 'int', 'char',
                    'float', 'double', 'void', 'register', 'typedef', 'restrict'}


def c_def(name):
    """在 .c 中找函数定义（行首优先，其次缩进定义，排除控制关键字前缀）"""
    esc = re.escape(name)
    r = None
    for path in CS:
        with open(path, errors='ignore') as f:
            for i, line in enumerate(f, 1):
                if re.match(r'^[A-Za-z_][A-Za-z0-9_ \t\*]*\b' + esc + r'\s*\(', line):
                    return path, i
    for path in CS:
        with open(path, errors='ignore') as f:
            for i, line in enumerate(f, 1):
                m = re.match(r'^\s*([A-Za-z_][A-Za-z0-9_ \t\*]*)\b' + esc + r'\s*\(', line)
                if m and m.group(1).strip() not in CTRL:
                    return path, i
    return r


def find_macro(name):
    """返回 (path, line, body)；跳过形如 `#define f( x )` 的空体桩宏"""
    esc = re.escape(name)
    for h in MACRO_FILES:
        lines = open(h, errors='ignore').read().split('\n')
        for i, line in enumerate(lines):
            m = re.match(r'\s*#\s*define\s+' + esc + r'\b(.*)$', line)
            if not m:
                continue
            body = m.group(1)
            if body.strip().startswith('('):
                close = body.find(')')
                if close < 0 or body[close + 1:].strip() == '':
                    continue
            j = i
            while lines[j].rstrip().endswith('\\') and j + 1 < len(lines):
                j += 1
                body += ' ' + lines[j]
            return h, i + 1, body
    return None


def proto(name):
    esc = re.escape(name)
    for h in HEADERS:
        with open(h, errors='ignore') as f:
            for i, line in enumerate(f, 1):
                if re.match(r'^\s*(?:extern\s+)?[A-Za-z_][A-Za-z0-9_ \t\*]*\b' + esc + r'\s*\(', line):
                    return h, i
    return None


def find_def(name, depth=0):
    r = c_def(name)                                  # 1) .c 实现
    if r:
        return r
    mac = find_macro(name)
    if mac and depth < 4:                            # 2) 宏 → 递归解析到 .c
        path, ln, body = mac
        for cand in re.findall(r'([A-Za-z_][A-Za-z0-9_]*)\s*\(', body):
            if cand == name or cand in CAND_SKIP or cand.startswith('__'):
                continue
            r = find_def(cand, depth + 1)
            if r and r[0].endswith('.c'):
                return r
        return path, ln                              # 4) 纯宏兜底：宏定义处
    if mac:
        return mac[0], mac[1]
    return proto(name)                               # 3) 头文件原型


# 同时兼容「已链接」与「未链接」两种行，脚本幂等
line_re = re.compile(r'^(?:\[([^\]\s]+)\]\([^)]*\)|(\S+))(\s+///\s.*)$')
resolved, hard = 0, []
out = []
with open(README, errors='ignore') as f:
    for line in f:
        m = line_re.match(line.rstrip('\n'))
        if m:
            token = m.group(1) or m.group(2)
            rest = m.group(3)
            name = re.sub(r'\(.*\)$', '', token)
            hit = find_def(name)
            if hit:
                path, ln = hit
                out.append(f'[{token}]({os.path.relpath(path, ROOT)}#L{ln}){rest}\n')
                resolved += 1
                if not path.endswith('.c'):
                    hard.append(name)
            else:
                out.append(line)
        else:
            out.append(line)

with open(README, 'w') as f:
    f.writelines(out)

print(f'resolved={resolved}  links_to_header={len(hard)}')
print('仍指向头文件（无 .c 可解析）:', ' '.join(hard))
