"""Escape bare underscores inside \\texttt{} contexts in .tex files."""
import os
import re

LABS_DIR = r'C:\Development\RTOS\labs'

tex_files = []
for root, dirs, files in os.walk(LABS_DIR):
    for f in files:
        if f.endswith('_instructions.tex'):
            tex_files.append(os.path.join(root, f))


def escape_underscores_in_texttt(text):
    result = []
    i = 0
    in_listing = False

    while i < len(text):
        # Track lstlisting blocks (verbatim — no escaping needed)
        if text[i:].startswith(r'\begin{lstlisting}'):
            in_listing = True
        if text[i:].startswith(r'\end{lstlisting}'):
            in_listing = False

        if not in_listing and text[i:].startswith(r'\texttt{'):
            # Grab the entire \texttt{...} argument
            start = i + len(r'\texttt{')
            depth = 1
            j = start
            while j < len(text) and depth > 0:
                if text[j] == '{':
                    depth += 1
                elif text[j] == '}':
                    depth -= 1
                j += 1
            content = text[start:j - 1]
            # Replace bare _ (not preceded by \) with \_
            fixed = re.sub('(?<!\\\\)_', r'\\_', content)
            result.append(r'\texttt{' + fixed + '}')
            i = j
        else:
            result.append(text[i])
            i += 1

    return ''.join(result)


changed = []
for path in sorted(tex_files):
    with open(path, 'rb') as f:
        raw = f.read()
    crlf = b'\r\n' in raw
    text = raw.replace(b'\r\n', b'\n').decode('utf-8')
    original = text
    text = escape_underscores_in_texttt(text)
    if text != original:
        out = text.encode('utf-8')
        if crlf:
            out = out.replace(b'\n', b'\r\n')
        with open(path, 'wb') as f:
            f.write(out)
        label = os.path.relpath(path, LABS_DIR)
        changed.append(label)
        print('  fixed: ' + label)
    else:
        label = os.path.relpath(path, LABS_DIR)
        print('  ok:    ' + label)

print('\n' + str(len(changed)) + ' files updated')
