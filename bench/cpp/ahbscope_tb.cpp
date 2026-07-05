////////////////////////////////////////////////////////////////////////////////
//
// Filename:	ahbscope_tb.cpp
// {{{
// Project:	AHBScope, an AHB3-Lite hosted scope
//
// Purpose:	A quick test bench to determine if the ahbscope module works.
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
// }}}
#include <stdio.h>

#include <verilated.h>
#include <verilated_vcd_c.h>
#include "Vahbscope_tb.h"
#define	INTERRUPTWIRE	o_interrupt
#include "ahb_tb.h"

class	AHBSCOPE_TB : public AHB_TB<Vahbscope_tb> {
	bool	m_debug;
public:
	AHBSCOPE_TB(void) {
		m_debug = true;
	}

	virtual	void	tick(void) {
		AHB_TB<Vahbscope_tb>::tick();

		bool	writeout = true;
		if ((m_debug)&&(writeout)) {}
	}

	virtual	void	reset(void) {
		bus_idle();
		m_core->i_reset = 1;
		tick();
		m_core->i_reset = 0;
	}

	unsigned	trigger(void) {
		m_core->i_trigger = 1;
		idle();
		m_core->i_trigger = 0;
		return m_core->o_data;
	}

	bool	debug(void) const { return m_debug; }
	bool	debug(bool nxtv) { return m_debug = nxtv; }
};

int main(int  argc, char **argv) {
	Verilated::commandArgs(argc, argv);
	AHBSCOPE_TB	*tb = new AHBSCOPE_TB;
	unsigned	v;
	unsigned *buf = NULL;
	int	trigpt;
	unsigned	trigger_time, expected_first_value;

	tb->opentrace("ahbscope_tb.vcd");
	printf("Giving the core 2 cycles to start up\n");
	tb->reset();
	tb->idle(2);

#define	AHBSCOPE_STATUS	0
#define	AHBSCOPE_DATA	4
#define	AHBSCOPE_PRIMED	0x10000000
#define	AHBSCOPE_TRIGGERED 0x20000000
#define	AHBSCOPE_STOPPED 0x40000000
#define	AHBSCOPE_LGLEN(A)	((A>>20)&0x01f)

	v = tb->readio(AHBSCOPE_STATUS);
	int ln = AHBSCOPE_LGLEN(v);
	printf("V   = %08x\n", v);
	printf("LN  = %d, or %d entries\n", ln, (1<<ln));
	printf("DLY = %d\n", (v&0xfffff));
	if (((1<<ln) < tb->m_tickcount)&&(v&AHBSCOPE_PRIMED)) {
		printf("SCOPE is already triggered! ??\n");
		goto test_failure;
	}
	buf = new unsigned[(1<<ln)];

	tb->idle(1<<ln);

	v = tb->readio(AHBSCOPE_STATUS);
	if ((v&AHBSCOPE_PRIMED)==0) {
		printf("v = %08x\n", v);
		printf("SCOPE hasn't primed! ??\n");
		goto test_failure;
	}

	trigger_time = tb->trigger() & 0x7fffffff;
	printf("TRIGGERED AT %08x\n", trigger_time);

	v = tb->readio(AHBSCOPE_STATUS);
	if ((v&AHBSCOPE_TRIGGERED)==0) {
		printf("v = %08x\n", v);
		printf("SCOPE hasn't triggered! ??\n");
		goto test_failure;
	}

	while((v & AHBSCOPE_STOPPED)==0)
		v = tb->readio(AHBSCOPE_STATUS);
	printf("SCOPE has stopped, reading data\n");

	tb->readz(AHBSCOPE_DATA, (1<<ln), buf);
	for(int i=0; i<(1<<ln); i++) {
		printf("%4d: %08x%s\n", i, buf[i],
			(i == (1<<ln)-1-(v&0x0fffff)) ? " <<--- TRIGGER!":"");
		if ((i>0)&&(((buf[i]&0x7fffffff)-(buf[i-1]&0x7fffffff))!=1)) {
			printf("ERR: Scope data doesn't increment!\n");
			printf("\tIn other words--its not matching the test signal\n");
			goto test_failure;
		}
	}

	trigpt = (1<<ln) - (v&0x0fffff) - 1;
	if ((trigpt >= 0)&&(trigpt < (1<<ln))) {
		printf("Trigger value = %08x\n", buf[trigpt]);
		if (((0x80000000 & buf[trigpt])==0)&&(trigpt>0)) {
			printf("Pre-Trigger value = %08x\n", buf[trigpt-1]);
			if ((buf[trigpt-1]&0x80000000)==0) {
				printf("TRIGGER NOT FOUND\n");
				goto test_failure;
			}
		}
	}

	expected_first_value = trigger_time + (v&0x0fffff) - (1<<ln);
	if (buf[0] != expected_first_value) {
		printf("Initial value = %08x\n", buf[0]);
		printf("Expected:     %08x\n", expected_first_value);
		printf("ERR: WRONG STARTING-VALUE\n");
		goto test_failure;
	}

	printf("SUCCESS!!\n");
	delete[] buf;
	delete tb;
	exit(0);
test_failure:
	printf("FAIL-HERE\n");
	for(int i=0; i<4; i++)
		tb->tick();
	printf("TEST FAILED\n");
	delete[] buf;
	delete tb;
	exit(-1);
}
