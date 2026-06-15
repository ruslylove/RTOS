#!/bin/bash
# Compile all lab instruction .tex files to PDF using pdflatex
LABS_DIR="/c/Development/RTOS/labs"
PDFLATEX="pdflatex"
PASS=(); FAIL=()

run_lab() {
  local dir="$1" tex="$2"
  cd "$dir" || return
  # Run twice for cross-references
  "$PDFLATEX" -interaction=nonstopmode -halt-on-error "$tex" > pdflatex.log 2>&1
  local rc=$?
  if [ $rc -eq 0 ]; then
    "$PDFLATEX" -interaction=nonstopmode -halt-on-error "$tex" >> pdflatex.log 2>&1
    rc=$?
  fi
  cd - > /dev/null
  echo $rc
}

for tex_path in \
  "$LABS_DIR/lab01_setup_verify/lab01_instructions.tex" \
  "$LABS_DIR/lab02_freertos_basics/lab02_instructions.tex" \
  "$LABS_DIR/lab03_aperiodic_server/lab03_instructions.tex" \
  "$LABS_DIR/lab04_priority_inversion/lab04_instructions.tex" \
  "$LABS_DIR/lab05_watchdog/lab05_instructions.tex" \
  "$LABS_DIR/lab06_wcet_dwt/lab06_instructions.tex" \
  "$LABS_DIR/lab07_stack_overflow/lab07_instructions.tex" \
  "$LABS_DIR/lab08_trustzone/lab08_instructions.tex" \
  "$LABS_DIR/lab09_freertos_ipc/lab09_instructions.tex" \
  "$LABS_DIR/lab10_tickless/lab10_instructions.tex" \
  "$LABS_DIR/lab11_zephyr/lab11_instructions.tex"
do
  dir=$(dirname "$tex_path")
  tex=$(basename "$tex_path")
  lab=$(basename "$dir")
  printf "Compiling %-40s ... " "$lab/$tex"
  rc=$(run_lab "$dir" "$tex")
  if [ "$rc" -eq 0 ]; then
    echo "OK"
    PASS+=("$lab")
  else
    echo "FAIL"
    FAIL+=("$lab")
    # Show first error line
    grep -m1 "^!" "$dir/pdflatex.log" 2>/dev/null || true
  fi
done

echo ""
echo "=== Results ==="
echo "PASS: ${PASS[*]}"
[ ${#FAIL[@]} -gt 0 ] && echo "FAIL: ${FAIL[*]}"
