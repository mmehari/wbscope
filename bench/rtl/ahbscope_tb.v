////////////////////////////////////////////////////////////////////////////////
//
// Filename: 	ahbscope_tb.v
// {{{
// Project:	AHBScope, an AHB3-Lite hosted scope
//
// Purpose:	This file is a test bench wrapper around the AHB3-Lite scope,
//		designed to create a "signal" which can then be scoped and
//	proven.  In our case here, the "signal" is a counter.  When we test
//	the scope within a Verilator testbench, we'll know if our test was
//	"correct" if the counter 1) only ever increments by 1, and 2) if the
//	trigger lands on the right data sample.
//
// Creator:	Dan Gisselquist, Ph.D.
//		Gisselquist Technology, LLC
//
////////////////////////////////////////////////////////////////////////////////
// }}}
// Copyright (C) 2015-2024, Gisselquist Technology, LLC
// {{{
// This program is free software (firmware): you can redistribute it and/or
// modify it under the terms of  the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or (at
// your option) any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTIBILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program.  (It's in the $(ROOT)/doc directory.  Run make with no
// target there if the PDF file isn't present.)  If not, see
// <http://www.gnu.org/licenses/> for a copy.
// }}}
// License:	GPL, v3, as defined and found on www.gnu.org,
// {{{
//		http://www.gnu.org/licenses/gpl.html
//
////////////////////////////////////////////////////////////////////////////////
//
`default_nettype	none
// }}}
module	ahbscope_tb (
		// {{{
		input	wire		i_clk,
		// i_reset is required by test infrastructure
					i_reset,
		// The test data.  o_data is internally generated here from a
		// counter, i_trigger is given externally
					i_trigger,
		output	wire	[31:0]	o_data,
		// AHB3-Lite slave interaction
		// {{{
		input	wire		i_hsel,
		input	wire	[31:0]	i_haddr,
		input	wire	[1:0]	i_htrans,
		input	wire		i_hwrite,
		input	wire	[2:0]	i_hsize,
		input	wire	[2:0]	i_hburst,
		input	wire	[3:0]	i_hprot,
		input	wire	[31:0]	i_hwdata,
		input	wire		i_hready,
		output	wire	[31:0]	o_hrdata,
		output	wire		o_hreadyout,
		output	wire		o_hresp,
		// }}}
		// And our output interrupt
		output	wire		o_interrupt
		// }}}
	);

	// Signal declarations
	// {{{
	reg	[30:0]	counter;
	// }}}

	// counter
	// {{{
	initial	counter = 0;
	always @(posedge i_clk)
		counter <= counter + 1'b1;
	// }}}

	assign	o_data = { i_trigger, counter };

	ahbscope #(.LGMEM(5'd6), .BUSW(32), .SYNCHRONOUS(1),
			.DEFAULT_HOLDOFF(1))
	scope(
		.i_data_clk(i_clk),
		.i_ce(1'b1),
		.i_trigger(i_trigger),
		.i_data(o_data),
		.HCLK(i_clk),
		.HRESETn(!i_reset),
		.HSEL(i_hsel),
		.HADDR(i_haddr),
		.HTRANS(i_htrans),
		.HWRITE(i_hwrite),
		.HSIZE(i_hsize),
		.HBURST(i_hburst),
		.HPROT(i_hprot),
		.HWDATA(i_hwdata),
		.HREADY(i_hready),
		.HRDATA(o_hrdata),
		.HREADYOUT(o_hreadyout),
		.HRESP(o_hresp),
		.o_interrupt(o_interrupt));

	// Make Verilator happy
	// {{{
	// verilator lint_off UNUSED
	wire	unused;
	assign	unused = &{ 1'b0 };
	// verilator lint_on UNUSED
	// }}}
endmodule
