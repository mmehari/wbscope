////////////////////////////////////////////////////////////////////////////////
//
// Filename:	ahb_tb.h
// {{{
// Project:	AHBScope, an AHB3-Lite hosted scope
//
// Purpose:	To provide a fairly generic interface wrapper to an AHB3-Lite
//		bus, that can then be used to create a test-bench class.
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
#ifndef	AHB_TB_H
#define	AHB_TB_H

#include <stdio.h>
#include <stdint.h>

#include "testb.h"
#include "devbus.h"

template <class VA>	class	AHB_TB : public TESTB<VA>, public DEVBUS {
	// {{{
#ifdef	INTERRUPTWIRE
	bool	m_interrupt;
#endif
	bool	m_buserr;
	// }}}
public:
	typedef	uint32_t	BUSW;

	bool	m_bomb;

	AHB_TB(void) {
		// {{{
		m_bomb = false;
		m_buserr = false;
#ifdef	INTERRUPTWIRE
		m_interrupt = false;
#endif
		TESTB<VA>::m_core->i_hsel   = 0;
		TESTB<VA>::m_core->i_haddr  = 0;
		TESTB<VA>::m_core->i_htrans = 0;
		TESTB<VA>::m_core->i_hwrite = 0;
		TESTB<VA>::m_core->i_hsize  = 2;
		TESTB<VA>::m_core->i_hburst = 0;
		TESTB<VA>::m_core->i_hprot  = 0;
		TESTB<VA>::m_core->i_hwdata = 0;
		TESTB<VA>::m_core->i_hready = 1;
		// }}}
	}

	virtual	void	close(void) {
		TESTB<VA>::closetrace();
	}

	virtual	void	kill(void) {
		close();
	}

#ifdef	INTERRUPTWIRE
	virtual	void	tick(void) {
		TESTB<VA>::tick();
		if (TESTB<VA>::m_core->INTERRUPTWIRE)
			m_interrupt = true;
	}
#endif
#define	TICK	this->tick

	void	bus_idle(void) {
		TESTB<VA>::m_core->i_hsel   = 0;
		TESTB<VA>::m_core->i_htrans = 0;
		TESTB<VA>::m_core->i_hwrite = 0;
		TESTB<VA>::m_core->i_hsize  = 2;
		TESTB<VA>::m_core->i_hburst = 0;
		TESTB<VA>::m_core->i_hprot  = 0;
		TESTB<VA>::m_core->i_hready = 1;
	}

	void	idle(const unsigned counts = 1) {
		bus_idle();
		for(unsigned k=0; k<counts; k++)
			this->tick();
	}

	BUSW	readio(const BUSW a) {
		BUSW result;

		TESTB<VA>::m_core->i_hsel   = 1;
		TESTB<VA>::m_core->i_haddr  = a;
		TESTB<VA>::m_core->i_htrans = 2;
		TESTB<VA>::m_core->i_hwrite = 0;
		TESTB<VA>::m_core->i_hsize  = 2;
		TESTB<VA>::m_core->i_hburst = 0;
		TESTB<VA>::m_core->i_hprot  = 0;
		TESTB<VA>::m_core->i_hready = 1;
		TICK();

		bus_idle();
		TICK();

		result = TESTB<VA>::m_core->o_hrdata;
		if (TESTB<VA>::m_core->o_hresp) {
			printf("AHB/SR-BOMB: RESPONSE ERROR @ %08x\n", a);
			m_buserr = true;
			m_bomb = true;
		}
		return result;
	}

	void	readv(const BUSW a, const int len, BUSW *buf, const int inc=1) {
		for(int i=0; i<len; i++)
			buf[i] = readio(a + ((inc) ? 4*i : 0));
	}

	void	readi(const BUSW a, const int len, BUSW *buf) {
		return readv(a, len, buf, 1);
	}

	void	readz(const BUSW a, const int len, BUSW *buf) {
		return readv(a, len, buf, 0);
	}

	void	writeio(const BUSW a, const BUSW v) {
		TESTB<VA>::m_core->i_hsel   = 1;
		TESTB<VA>::m_core->i_haddr  = a;
		TESTB<VA>::m_core->i_htrans = 2;
		TESTB<VA>::m_core->i_hwrite = 1;
		TESTB<VA>::m_core->i_hsize  = 2;
		TESTB<VA>::m_core->i_hburst = 0;
		TESTB<VA>::m_core->i_hprot  = 0;
		TESTB<VA>::m_core->i_hwdata = v;
		TESTB<VA>::m_core->i_hready = 1;
		TICK();

		bus_idle();
		TICK();

		if (TESTB<VA>::m_core->o_hresp) {
			printf("AHB/SW-BOMB: RESPONSE ERROR @ %08x <= %08x\n", a, v);
			m_buserr = true;
			m_bomb = true;
		}
	}

	void	writev(const BUSW a, const int len, const BUSW *buf, const int inc=1) {
		for(int i=0; i<len; i++)
			writeio(a + ((inc) ? 4*i : 0), buf[i]);
	}

	void	writei(const BUSW a, const int len, const BUSW *buf) {
		return writev(a, len, buf, 1);
	}

	void	writez(const BUSW a, const int len, const BUSW *buf) {
		return writev(a, len, buf, 0);
	}

	bool	bombed(void) const { return m_bomb; }

	bool	poll(void) {
#ifdef	INTERRUPTWIRE
		return (m_interrupt)||(TESTB<VA>::m_core->INTERRUPTWIRE != 0);
#else
		return false;
#endif
	}

	bool	bus_err(void) const {
		return m_buserr;
	}

	void	reset_err(void) {
		m_buserr = false;
	}

	void	usleep(unsigned msec) {
		unsigned count = 1000*100 * msec;
		while(count-- != 0)
#ifdef	INTERRUPTWIRE
			if (poll()) return; else
#endif
			TICK();
	}

	void	clear(void) {
#ifdef	INTERRUPTWIRE
		m_interrupt = false;
#endif
	}

	void	wait(void) {
#ifdef	INTERRUPTWIRE
		while(!poll())
			TICK();
#else
		assert(("No interrupt defined",0));
#endif
	}
	// }}}
};

#endif
