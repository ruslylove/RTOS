#!/usr/bin/env bash
# Compile every TikZ figure in this folder to SVG for the Slidev decks.
# Requires a LaTeX toolchain (pdflatex + dvisvgm) — e.g. MacTeX / TeX Live.
set -euo pipefail
cd "$(dirname "$0")"

for tex in *.tex; do
  name="${tex%.tex}"
  echo "==> $name"
  pdflatex -interaction=batchmode -halt-on-error "$tex" > /dev/null
  dvisvgm --pdf --output="$name.svg" "$name.pdf" > /dev/null 2>&1
done

rm -f ./*.aux ./*.log ./*.pdf
echo "done — SVGs written next to their .tex sources in figures/"
