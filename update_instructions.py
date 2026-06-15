#!/usr/bin/env python3
"""Bulk-update lab instruction .tex and .md files from macOS paths to Windows."""

import os
import re

LABS_DIR = r'C:\Development\RTOS\labs'

TEX_FILES = []
MD_FILES  = []

for root, dirs, files in os.walk(LABS_DIR):
    for f in files:
        path = os.path.join(root, f)
        if f.endswith('_instructions.tex'):
            TEX_FILES.append(path)
        elif f.endswith('_instructions.md'):
            MD_FILES.append(path)

# ── Substitutions shared by both .tex and .md ─────────────────────────────────
COMMON_SUBS = [
    # Toolchain version label
    ('ARM GNU Toolchain 12', 'ARM GNU Toolchain 14.2'),
    # macOS toolchain path (plain text / markdown)
    ('/opt/homebrew/Cellar/arm-gcc-bin@12/12.2.Rel1/bin',
     'C:/Users/rusle/.mcuxpressotools/arm-gnu-toolchain-14.2.rel1-mingw-w64-x86_64-arm-none-eabi/bin'),
    # pyocd venv path — macOS
    ('/Users/rus/Development/RTOS/.venv/bin/pyocd',
     'C:/Development/RTOS/.venv/Scripts/pyocd.exe'),
    # SDK root — macOS long form
    ('/Users/rus/Development/NXP_project/SDK/mcuxsdk/mcuxsdk',
     'C:/nxp_sdk/mcuxsdk/mcuxsdk'),
    ('/Users/<you>/Development/NXP_project/SDK/mcuxsdk/mcuxsdk',
     'C:/nxp_sdk/mcuxsdk/mcuxsdk'),
    # pyocd pack install — part name
    ('pyocd pack install NXP.MCXN236_DFP', 'pyocd pack install MCXN236VDF'),
    # venv activation — macOS
    ('.venv/bin/activate', '.venv/Scripts/activate'),
    # pip install path note
    ('/Users/rus/Development/RTOS/.venv', 'C:/Development/RTOS/.venv'),
]

# ── .tex-only substitutions ────────────────────────────────────────────────────
TEX_SUBS = [
    # LaTeX-escaped macOS toolchain path (in \texttt{})
    (r'\texttt{/opt/homebrew/Cellar/arm-gcc-bin@12/12.2.Rel1/bin}',
     r'\texttt{C:/Users/rusle/.mcuxpressotools/arm-gnu-toolchain-14.2.rel1-mingw-w64-x86\_64-arm-none-eabi/bin}'),
    # pyocd path in \texttt
    (r'\texttt{/Users/rus/Development/RTOS/.venv/bin/pyocd}',
     r'\texttt{C:/Development/RTOS/.venv/Scripts/pyocd.exe}'),
    # macOS serial port
    (r'ls /dev/cu.usbmodem*',    r'# Windows: check Device Manager for COMx port'),
    (r'/dev/cu.usbmodemLBAZMNNVFKEDG3', r'COM3'),
    (r"/dev/cu.usbmodemXXXXXX", r'COMx'),
    (r"s = serial.Serial('/dev/cu.usbmodemXXXXXX'", r"s = serial.Serial('COMx'"),
    (r'screen /dev/cu.usbmodemXXXXXX 115200', r'# Windows: use PuTTY or Tera Term (COMx, 115200 baud)'),
    (r'# Press Ctrl-A then Ctrl-\\ to quit', r''),
]

# ── .md-only substitutions ─────────────────────────────────────────────────────
MD_SUBS = [
    (r'ls /dev/cu.usbmodem*', r'# Windows: check Device Manager for COMx port'),
    (r'# Example: /dev/cu.usbmodemLBAZMNNVFKEDG3', r'# Example: COM3'),
    (r'screen /dev/cu.usbmodemXXXXXX 115200', r'# Windows: use PuTTY or Tera Term (COMx, 115200 baud)'),
    (r"serial.Serial('/dev/cu.usbmodemXXXXXX'", r"serial.Serial('COMx'"),
]


def apply_subs(text, subs):
    for old, new in subs:
        text = text.replace(old, new)
    return text


def patch_make_in_bash_listings(text):
    """Replace bare `make` / `make flash` etc. with mingw32-make in bash listings."""
    result = []
    in_bash = False
    for line in text.splitlines(keepends=True):
        stripped = line.strip()
        if stripped.startswith(r'\begin{lstlisting}') and 'bashstyle' in stripped:
            in_bash = True
        if stripped.startswith(r'\end{lstlisting}'):
            in_bash = False
        if in_bash:
            # Replace `make` standalone or at start of command, but not in comments
            line = re.sub(r'\bmake\b', 'mingw32-make', line)
        result.append(line)
    return ''.join(result)


def process(path, extra_subs):
    with open(path, 'rb') as f:
        raw = f.read()
    crlf = b'\r\n' in raw
    text = raw.replace(b'\r\n', b'\n').decode('utf-8')
    original = text

    text = apply_subs(text, COMMON_SUBS)
    text = apply_subs(text, extra_subs)

    if path.endswith('.tex'):
        text = patch_make_in_bash_listings(text)

    if text != original:
        out = text.encode('utf-8')
        if crlf:
            out = out.replace(b'\n', b'\r\n')
        with open(path, 'wb') as f:
            f.write(out)
        label = os.path.relpath(path, LABS_DIR)
        print(f'  updated: {label}')
    else:
        label = os.path.relpath(path, LABS_DIR)
        print(f'  no change: {label}')


print('=== Patching .tex files ===')
for p in sorted(TEX_FILES):
    process(p, TEX_SUBS)

print('\n=== Patching .md files ===')
for p in sorted(MD_FILES):
    process(p, MD_SUBS)
