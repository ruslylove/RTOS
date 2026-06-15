import os

MAKEFILES = [
    r'C:\Development\RTOS\labs\lab01_setup_verify\Makefile',
    r'C:\Development\RTOS\labs\lab02_freertos_basics\Makefile',
    r'C:\Development\RTOS\labs\lab03_aperiodic_server\Makefile',
    r'C:\Development\RTOS\labs\lab04_priority_inversion\Makefile',
    r'C:\Development\RTOS\labs\lab05_watchdog\Makefile',
    r'C:\Development\RTOS\labs\lab06_wcet_dwt\Makefile',
    r'C:\Development\RTOS\labs\lab07_stack_overflow\Makefile',
    r'C:\Development\RTOS\labs\lab08_trustzone\nonsecure\Makefile',
    r'C:\Development\RTOS\labs\lab08_trustzone\secure\Makefile',
    r'C:\Development\RTOS\labs\lab09_dual_core\cm33_0\Makefile',
    r'C:\Development\RTOS\labs\lab09_dual_core\cm33_1\Makefile',
    r'C:\Development\RTOS\labs\lab10_tickless\Makefile',
]

# Convert /c/ style Unix paths to C:/ Windows paths for make variable defaults
# and PYOCD path
SUBS = [
    ('SDK_ROOT   ?= /c/nxp_sdk/mcuxsdk/mcuxsdk',
     'SDK_ROOT   ?= C:/nxp_sdk/mcuxsdk/mcuxsdk'),
    ('TOOLCHAIN ?= /c/Users/rusle/.mcuxpressotools/arm-gnu-toolchain-14.2.rel1-mingw-w64-x86_64-arm-none-eabi/bin',
     'TOOLCHAIN ?= C:/Users/rusle/.mcuxpressotools/arm-gnu-toolchain-14.2.rel1-mingw-w64-x86_64-arm-none-eabi/bin'),
    ('PYOCD ?= /c/Development/RTOS/.venv/Scripts/pyocd.exe',
     'PYOCD ?= C:/Development/RTOS/.venv/Scripts/pyocd.exe'),
]

for path in MAKEFILES:
    label = '/'.join(path.split('\\')[-3:])
    with open(path, 'rb') as f:
        raw = f.read()
    crlf = b'\r\n' in raw
    text = raw.replace(b'\r\n', b'\n').decode('utf-8')
    original = text
    for old, new in SUBS:
        text = text.replace(old, new)
    if text != original:
        out = text.encode('utf-8')
        if crlf:
            out = out.replace(b'\n', b'\r\n')
        with open(path, 'wb') as f:
            f.write(out)
        print(f'OK: {label}')
    else:
        print(f'--: {label}')
