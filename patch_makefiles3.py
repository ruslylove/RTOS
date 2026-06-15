import os

MAKEFILES = [
    r'C:\Development\RTOS\labs\lab08_trustzone\nonsecure\Makefile',
    r'C:\Development\RTOS\labs\lab08_trustzone\secure\Makefile',
    r'C:\Development\RTOS\labs\lab09_dual_core\cm33_0\Makefile',
    r'C:\Development\RTOS\labs\lab09_dual_core\cm33_1\Makefile',
]

REPLACEMENTS = [
    (
        'SDK_ROOT   = /Users/rus/Development/NXP_project/SDK/mcuxsdk/mcuxsdk',
        '# macOS: SDK_ROOT = /Users/rus/Development/NXP_project/SDK/mcuxsdk/mcuxsdk\nSDK_ROOT   ?= /c/nxp_sdk/mcuxsdk/mcuxsdk'
    ),
    (
        'TOOLCHAIN ?= /opt/homebrew/Cellar/arm-gcc-bin@12/12.2.Rel1/bin',
        '# macOS: TOOLCHAIN = /opt/homebrew/Cellar/arm-gcc-bin@12/12.2.Rel1/bin\nTOOLCHAIN ?= /c/Users/rusle/.mcuxpressotools/arm-gnu-toolchain-14.2.rel1-mingw-w64-x86_64-arm-none-eabi/bin'
    ),
    (
        'PYOCD ?= /Users/rus/Development/RTOS/.venv/bin/pyocd',
        'PYOCD ?= /c/Development/RTOS/.venv/Scripts/pyocd.exe'
    ),
    (
        '    -D__STARTUP_CLEAR_BSS \\\n    -DDEBUG',
        '    -D__STARTUP_CLEAR_BSS \\\n    -D__START=main \\\n    -DDEBUG'
    ),
    # Fix %.o: %.s to include DEFINES
    (
        '\t$(CC) $(CPU_FLAGS) $(OPT_FLAGS) -c $< -o $@',
        '\t$(CC) $(CPU_FLAGS) $(OPT_FLAGS) $(DEFINES) -c $< -o $@'
    ),
    # Replace %.o: %.S rule (with or without -D__START=main) + add explicit startup rule
    (
        '%.o: %.S\n\t$(CC) $(CPU_FLAGS) $(OPT_FLAGS) $(DEFINES) -D__START=main -c $< -o $@',
        '# Explicit rule for startup file (overrides case-insensitive pattern matching on Windows)\n$(SDK_DEVICE)/gcc/startup_MCXN236.o: $(SDK_DEVICE)/gcc/startup_MCXN236.S\n\t$(CC) $(CPU_FLAGS) $(OPT_FLAGS) $(DEFINES) -c $< -o $@\n\n%.o: %.S\n\t$(CC) $(CPU_FLAGS) $(OPT_FLAGS) $(DEFINES) -c $< -o $@'
    ),
    # Fallback: %.o: %.S without -D__START=main (some labs may already not have it)
    (
        '%.o: %.S\n\t$(CC) $(CPU_FLAGS) $(OPT_FLAGS) $(DEFINES) -c $< -o $@',
        '# Explicit rule for startup file (overrides case-insensitive pattern matching on Windows)\n$(SDK_DEVICE)/gcc/startup_MCXN236.o: $(SDK_DEVICE)/gcc/startup_MCXN236.S\n\t$(CC) $(CPU_FLAGS) $(OPT_FLAGS) $(DEFINES) -c $< -o $@\n\n%.o: %.S\n\t$(CC) $(CPU_FLAGS) $(OPT_FLAGS) $(DEFINES) -c $< -o $@'
    ),
]

for path in MAKEFILES:
    label = '/'.join(path.split('\\')[-3:])
    with open(path, 'rb') as f:
        raw = f.read()
    crlf = b'\r\n' in raw
    text = raw.replace(b'\r\n', b'\n').decode('utf-8')
    original = text

    for old, new in REPLACEMENTS:
        if old in text:
            # Skip explicit startup rule if already present
            if '$(SDK_DEVICE)/gcc/startup_MCXN236.o' in text and '$(SDK_DEVICE)/gcc/startup_MCXN236.o' in new:
                continue
            text = text.replace(old, new)

    if text != original:
        out = text.encode('utf-8')
        if crlf:
            out = out.replace(b'\n', b'\r\n')
        with open(path, 'wb') as f:
            f.write(out)
        print(f'OK: patched {label}')
    else:
        print(f'WARN: no changes: {label}')
