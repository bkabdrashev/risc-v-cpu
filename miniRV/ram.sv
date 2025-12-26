module ram (
  input  logic                  clock,
  input  logic                  reset,
  input  logic                  wen,
  input  logic [REG_END_WORD:0] wdata,
  input  logic [3:0]            wbmask,
  input  logic [REG_END_WORD:0] addr,    

  output logic [REG_END_WORD:0] rdata
);
/* verilator lint_off UNUSEDPARAM */
  `include "defs.vh"
/* verilator lint_on UNUSEDPARAM */

  export "DPI-C" function sv_mem_read;
  function int unsigned sv_mem_read(input int unsigned mem_addr);
    return mem_read(mem_addr);
  endfunction

  export "DPI-C" function sv_mem_ptr;
  function longint unsigned sv_mem_ptr();
    return mem_ptr();
  endfunction

  export "DPI-C" function sv_vga_ptr;
  function longint unsigned sv_vga_ptr();
    return vga_ptr();
  endfunction

  export "DPI-C" function sv_uart_ptr;
  function longint unsigned sv_uart_ptr();
    return uart_ptr();
  endfunction

  export "DPI-C" function sv_time_ptr;
  function longint unsigned sv_time_ptr();
    return time_ptr();
  endfunction

  export "DPI-C" function sv_mem_write;
  function void sv_mem_write(input int unsigned mem_addr, input int unsigned mem_wdata, input byte mem_wbmask);
    mem_write(mem_addr, mem_wdata, mem_wbmask);
  endfunction

  import "DPI-C" context task mem_write(input int unsigned address, input int unsigned write, input byte wbmask);
  import "DPI-C" context function int unsigned mem_read(input int unsigned address);
  import "DPI-C" context task mem_reset();
  import "DPI-C" context function longint unsigned mem_ptr();
  import "DPI-C" context function longint unsigned vga_ptr();
  import "DPI-C" context function longint unsigned uart_ptr();
  import "DPI-C" context function longint unsigned time_ptr();

  always_ff @(posedge clock or posedge reset) begin
    if (reset) begin
      mem_reset();
    end else begin
      rdata <= (!wen) ? mem_read(addr) : 32'b0;
      // $display("rdata: %d, wen: %d", rdata, wen);
      if (wen) mem_write(addr, wdata, {4'b0, wbmask});
    end
  end

  /*
  always_comb begin
    rdata = mem_read(addr);
  end
  */

endmodule


