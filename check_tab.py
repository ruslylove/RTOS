with open(r'C:\Development\RTOS\labs\lab02_freertos_basics\Makefile', 'rb') as f:
    content = f.read()
idx = content.find(b'startup_MCXN236.o: ')
if idx >= 0:
    chunk = content[idx:idx+200]
    print(repr(chunk))
else:
    print("pattern not found")
