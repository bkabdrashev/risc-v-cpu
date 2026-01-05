# --build --runtime-debug -CFLAGS "-g3 -O0 -fno-omit-frame-pointer" -LDFLAGS "-g" \

verilator --trace -cc \
  -IysyxSoC/perip/uart16550/rtl \
  -IysyxSoC/perip/spi/rtl \
  -Isoc \
  $(find ysyxSoC/perip -type f -name '*.v') \
  $(find soc/ -type f -name '*.sv') \
  $(find soc/ -type f -name '*.vh') \
  ysyxSoC/ready-to-run/D-stage/ysyxSoCFull.v \
  --timescale "1ns/1ns" \
  --no-timing \
  --top-module ysyxSoCTop \
  --exe soc/soc_main.cpp soc/c_dpi.cpp \
&& make -C obj_dir -f VysyxSoCTop.mk VysyxSoCTop \
&& ./obj_dir/VysyxSoCTop "$@"
