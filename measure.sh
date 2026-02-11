YOSYS_PATH="yosys-sta"
MICROBENCH_PATH="am-kernels/benchmarks/microbench"
ROOT_DIR="$(pwd)"
MEASURE_TEMP="${ROOT_DIR}/__temp_measure.txt"
MEASURE_CSV="${ROOT_DIR}/measure.csv"
FREQ_TEMP="${ROOT_DIR}/__temp_freq.txt"
DEVICE_DELAY_FILE="${ROOT_DIR}/rtl/freq_defines.vh"

printf "%s,%s,%s," "$(git rev-parse HEAD)" "$(date +"%Y-%m-%dT%H:%M:%S")" "text" > "$MEASURE_TEMP"

cd "$YOSYS_PATH"
make syn
make sta
python "$ROOT_DIR/scripts/freq.py"  result/cpu-100MHz/cpu.rpt > "$FREQ_TEMP"

printf "%s,%s,%s," \
  "$(cat "$FREQ_TEMP")" \
  "$(python "$ROOT_DIR/scripts/area.py"  result/cpu-100MHz/synth_stat.txt)" \
  "$(python "$ROOT_DIR/scripts/power.py" result/cpu-100MHz/cpu.pwr)" >> "$MEASURE_TEMP"

python -c 'import sys; print("localparam RS = ", round(float(sys.stdin.read())*1000), ";")' <<< "$(cat "$FREQ_TEMP")"  > "$DEVICE_DELAY_FILE"
cd - >/dev/null

cd "$MICROBENCH_PATH"
make ARCH=minirv-npc run verbose=4 cpu=vsoc measure_path="$MEASURE_TEMP" mainargs=train
cd - >/dev/null

cat "$MEASURE_TEMP" >> "$MEASURE_CSV"
rm "$MEASURE_TEMP"
rm "$FREQ_TEMP"
