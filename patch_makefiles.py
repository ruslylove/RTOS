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

REPLACEMENTS = [
    # SDK_ROOT: change = to ?= and update path
    (
        'SDK_ROOT   = /Users/rus/Development/NXP_project/SDK/mcuxsdk/mcuxsdk',
        '# macOS: SDK_ROOT = /Users/rus/Development/NXP_project/SDK/mcuxsdk/mcuxsdk\nSDK_ROOT   ?= /c/nxp_sdk/mcuxsdk/mcuxsdk'
    ),
    # Toolchain
    (
        'TOOLCHAIN ?= /opt/homebrew/Cellar/arm-gcc-bin@12/12.2.Rel1/bin',
        '# macOS: TOOLCHAIN = /opt/homebrew/Cellar/arm-gcc-bin@12/12.2.Rel1/bin\nTOOLCHAIN ?= /c/Users/rusle/.mcuxpressotools/arm-gnu-toolchain-14.2.rel1-mingw-w64-x86_64-arm-none-eabi/bin'
    ),
    # PYOCD
    (
        'PYOCD ?= /Users/rus/Development/RTOS/.venv/bin/pyocd',
        'PYOCD ?= /c/Development/RTOS/.venv/Scripts/pyocd.exe'
    ),
    # Add __START=main to DEFINES (after __STARTUP_CLEAR_BSS)
    (
        '    -D__STARTUP_CLEAR_BSS \\\n    -DDEBUG',
        '    -D__STARTUP_CLEAR_BSS \\\n    -D__START=main \\\n    -DDEBUG'
    ),
    # Fix %.o: %.s to include DEFINES
    (
        '\t$(CC) $(CPU_FLAGS) $(OPT_FLAGS) -c $< -o $@',
        '\t$(CC) $(CPU_FLAGS) $(OPT_FLAGS) $(DEFINES) -c $< -o $@'
    ),
    # Add explicit startup rule before %.o: %.S, and strip old -D__START=main from %.o: %.S
    (
        '%.o: %.S\n\t$(CC) $(CPU_FLAGS) $(OPT_FLAGS) $(DEFINES) -D__START=main -c $< -o $@',
        '# Explicit rule for startup file (overrides case-insensitive pattern matching on Windows)\n$(SDK_DEVICE)/gcc/startup_MCXN236.o: $(SDK_DEVICE)/gcc/startup_MCXN236.S\n\t$(CC) $(CPU_FLAGS) $(OPT_FLAGS) $(DEFINES) -c $< -o $@\n\n%.o: %.S\n\t$(CC) $(CPU_FLAGS) $(OPT_FLAGS) $(DEFINES) -c $< -o $@'
    ),
]

for lab in LABS:
    path = os.path.join(BASE, lab, 'Makefile')
    with open(path, 'r', newline='') as f:
        content = f.read()
    original = content
    for old, new in REPLACEMENTS:
        if old in content:
            content = content.replace(old, new)
        else:
            print(f'  MISS [{lab}]: {old[:50]!r}')
    if content == original:
        print(f'WARN: no changes applied to {lab}')
    else:
        with open(path, 'w', newline='') as f:
            f.write(content)
        print(f'OK: patched {lab}')
