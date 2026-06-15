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

# Remove the explicit startup rule block (comment + rule + recipe)
PATTERNS_TO_REMOVE = [
    # With comment (from our patching)
    (
        '# Explicit rule for startup file (overrides case-insensitive pattern matching on Windows)\n'
        '$(SDK_DEVICE)/gcc/startup_MCXN236.o: $(SDK_DEVICE)/gcc/startup_MCXN236.S\n'
        '\t$(CC) $(CPU_FLAGS) $(OPT_FLAGS) $(DEFINES) -c $< -o $@\n'
        '\n'
    ),
    # lab01 variant (slightly different comment)
    (
        '# Explicit rule for the startup file (overrides case-insensitive pattern matching on Windows)\n'
        '$(SDK_DEVICE)/gcc/startup_MCXN236.o: $(SDK_DEVICE)/gcc/startup_MCXN236.S\n'
        '\t$(CC) $(CPU_FLAGS) $(OPT_FLAGS) $(DEFINES) -c $< -o $@\n'
        '\n'
    ),
]

for path in MAKEFILES:
    label = '/'.join(path.split('\\')[-3:])
    with open(path, 'rb') as f:
        raw = f.read()
    crlf = b'\r\n' in raw
    text = raw.replace(b'\r\n', b'\n').decode('utf-8')
    original = text

    for pat in PATTERNS_TO_REMOVE:
        text = text.replace(pat, '')

    if text != original:
        out = text.encode('utf-8')
        if crlf:
            out = out.replace(b'\n', b'\r\n')
        with open(path, 'wb') as f:
            f.write(out)
        print(f'OK: removed explicit startup rule: {label}')
    else:
        print(f'--: no explicit rule found: {label}')
