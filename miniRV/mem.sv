module mem (
  input  logic        clk,
  input  logic        reset,
  input  logic        wen,
  input  logic [31:0] wdata,
  input  logic [3:0]  wstrb,
  input  logic [31:0] ram_addr,    
  input  logic [31:0] rom_addr,    

  output logic [31:0] ram_read_data,
  output logic [31:0] rom_read_data
);
  // export "DPI-C" function mem_get;
  // function byte mem_get(input int unsigned get_addr);
  //   return mem[get_addr];
  // endfunction

  import "DPI-C" context task mem_write(input int unsigned address, input int unsigned write, input byte wstrb);
  import "DPI-C" context task mem_read(input int unsigned address, output int unsigned read);
  import "DPI-C" context task mem_reset();

  always_ff @(posedge clk or posedge reset) begin
    if (reset) begin
      mem_reset();
    end else begin
      if (wen) mem_write(ram_addr, wdata, {4'b0, wstrb});
    end
  end

  always_comb begin
    mem_read(ram_addr, ram_read_data);
    mem_read(rom_addr, rom_read_data);
  end

endmodule


