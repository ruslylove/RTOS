import os

LABS = [
    'lab02_freertos_basics',
    'lab03_aperiodic_server',
    'lab04_priority_inversion',
    'lab05_watchdog',
    'lab06_wcet_dwt',
    'lab07_stack_overflow',
    'lab10_tickless',
]

BASE = r'C:\Development\RTOS\labs'

for lab in LABS:
    path = os.path.join(BASE, lab, 'Makefile')
    with open(path, 'rb') as f:
        raw = f.read()

    # Normalize to LF for matching, remember original EOL style
    crlf = b'\r\n' in raw
    text = raw.replace(b'\r\n', b'\n').decode('utf-8')
    original = text

    # 1. Add -D__START=main to DEFINES block (after -D__STARTUP_CLEAR_BSS \)
    old1 = '    -D__STARTUP_CLEAR_BSS \\\n    -DDEBUG'
    new1 = '    -D__STARTUP_CLEAR_BSS \\\n    -D__START=main \\\n    -DDEBUG'
    if old1 in text:
        text = text.replace(old1, new1)
        print(f'  [OK] DEFINES __START added: {lab}')
    elif '-D__START=main' in text and 'DEFINES' in text:
        print(f'  [--] DEFINES __START already present: {lab}')
    else:
        print(f'  [!!] DEFINES pattern not found: {lab}')

    # 2. Replace %.o: %.S rule (remove old -D__START=main, add explicit startup rule)
    old2 = '%.o: %.S\n\t$(CC) $(CPU_FLAGS) $(OPT_FLAGS) $(DEFINES) -D__START=main -c $< -o $@'
    new2 = (
        '# Explicit rule for startup file (overrides case-insensitive pattern matching on Windows)\n'
        '$(SDK_DEVICE)/gcc/startup_MCXN236.o: $(SDK_DEVICE)/gcc/startup_MCXN236.S\n'
        '\t$(CC) $(CPU_FLAGS) $(OPT_FLAGS) $(DEFINES) -c $< -o $@\n'
        '\n'
        '%.o: %.S\n'
        '\t$(CC) $(CPU_FLAGS) $(OPT_FLAGS) $(DEFINES) -c $< -o $@'
    )
    if old2 in text:
        text = text.replace(old2, new2)
        print(f'  [OK] explicit startup rule added: {lab}')
    elif '$(SDK_DEVICE)/gcc/startup_MCXN236.o' in text:
        print(f'  [--] explicit startup rule already present: {lab}')
    else:
        print(f'  [!!] %.o: %.S pattern not found: {lab}')

    if text != original:
        # Restore original line endings
        out = text.encode('utf-8')
        if crlf:
            out = out.replace(b'\n', b'\r\n')
        with open(path, 'wb') as f:
            f.write(out)
        print(f'  => written: {lab}')
    else:
        print(f'  => no changes: {lab}')
