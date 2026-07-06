////////////////////////////////////////////////////////////////////////////////
//
// Filename:	fahb_slave.v
//
// Project:	WBScope / AHBScope formal helpers
//
// Purpose:	Formal properties for a simple AHB3-Lite slave with a
//		single-cycle response and no wait states.
//
////////////////////////////////////////////////////////////////////////////////
`default_nettype none

module	fahb_slave (
		input	wire			i_clk,
		input	wire			i_reset,
		// AHB3-Lite request channel
		input	wire			i_hsel,
		input	wire	[31:0]		i_haddr,
		input	wire	[1:0]		i_htrans,
		input	wire			i_hwrite,
		input	wire	[2:0]		i_hsize,
		input	wire	[2:0]		i_hburst,
		input	wire	[3:0]		i_hprot,
		input	wire	[31:0]		i_hwdata,
		input	wire			i_hready,
		// AHB3-Lite response channel
		input	wire	[31:0]		i_hrdata,
		input	wire			i_hreadyout,
		input	wire			i_hresp,
		// Captured request phase, aligned with the response phase
		output	reg			f_pending,
		output	reg			f_write,
		output	reg			f_addr,
		output	reg			f_error
	);

	wire	addr_phase_stb, addr_phase_err;
	reg	f_past_valid;
	wire	unused;

	assign	addr_phase_stb = i_hsel && i_hready && i_htrans[1];
	assign	addr_phase_err = (i_hsize != 3'b010);

	initial	f_past_valid = 1'b0;
	always @(posedge i_clk)
		f_past_valid <= 1'b1;

	always @(*)
	if (!f_past_valid)
		assume(i_reset);

	always @(*)
	if (i_reset)
	begin
		assume(!i_hsel || !i_hready || !i_htrans[1]);
	end

	initial begin
		f_pending = 1'b0;
		f_write   = 1'b0;
		f_addr    = 1'b0;
		f_error   = 1'b0;
	end

	always @(posedge i_clk)
	if (i_reset)
	begin
		f_pending <= 1'b0;
		f_write   <= 1'b0;
		f_addr    <= 1'b0;
		f_error   <= 1'b0;
	end else begin
		f_pending <= addr_phase_stb;
		f_write   <= i_hwrite;
		f_addr    <= i_haddr[2];
		f_error   <= addr_phase_stb && addr_phase_err;
	end

	always @(*)
		assert(i_hreadyout);

	always @(*)
		assert(i_hresp == f_error);

	assign	unused = &{ 1'b0, i_hrdata, i_hburst, i_hprot, i_hwdata,
				i_haddr[31:3], i_haddr[1:0], i_htrans[0] };
endmodule
