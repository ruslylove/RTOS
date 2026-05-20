#!/usr/bin/env bash
# Compile every TikZ figure in this folder to SVG for the Slidev decks.
# Requires a LaTeX toolchain (latex + dvisvgm) — e.g. MacTeX / TeX Live.
# Uses the latex -> DVI route: dvisvgm reads DVI natively, so no Ghostscript
# is needed (the --pdf route breaks on newer, unsupported Ghostscript builds).
# --no-fonts draws every glyph as an SVG <path>: Chromium (and the deployed
# site) does not support SVG <font> elements, which otherwise garble text.
set -euo pipefail
cd "$(dirname "$0")"

for tex in *.tex; do
  name="${tex%.tex}"
  echo "==> $name"
  latex -interaction=batchmode -halt-on-error "$tex" > /dev/null
  dvisvgm --no-fonts --output="$name.svg" "$name.dvi" > /dev/null 2>&1
done

rm -f ./*.aux ./*.log ./*.dvi ./*.pdf
echo "done — SVGs written next to their .tex sources in figures/"
