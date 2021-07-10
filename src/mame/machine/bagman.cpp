// license:BSD-3-Clause
// copyright-holders:Nicola Salmoria
/***************************************************************************

  bagman.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "emu.h"
#include "includes/bagman.h"


/*Creation date: 98-02-18 */
/*  A few words of comment:
**
**   What's inside of this file is a PAL16R6 emulator. Maybe someday we will
**need to use it for some other game too. We will need to make it more exact
**then (some of the functionality of this chip IS NOT implemented). However I
**have bought a book about PALs and I'm able to improve it. Just LMK.
**  Jarek Burczynski
**  bujar at mame dot net
*/


/*      64 rows x 32 columns
**  1 - fuse blown: disconnected from input (equal to 1)
**  0 - fuse not blown: connected to input (ie. x, not x, q, not q accordingly)
*/
static const uint8_t fusemap[64*32]=
{
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,0,1,1,1,1,0,1,1,0,1,1,1,1,0,1,1,0,1,1,1,0,1,1,1,1,1,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,1,0,1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,0,1,1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,0,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,0,1,1,1,1,1,1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,0,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,0,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,
1,1,0,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,
1,1,0,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,
1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,
1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,1,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,1,1,
1,1,0,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,
1,1,0,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,
1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,
1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,
1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,0,1,1,1,1,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};


void bagman_state::update_pal()
{
uint16_t rowoffs;
uint8_t row, column, val;

/*calculate all rows ANDs*/
	for (row = 0; row < 64; row++)
	{
		rowoffs = row*32;
		val = 1; /*prepare for AND */
		for (column = 0; column < 32; column++)
		{
			if ( fusemap[ rowoffs + column ] == 0 )
				val &= m_columnvalue[column];
		}
		m_andmap[row] = val;
	}

/* I/O pin #19 */
	val = 0; /*prepare for OR*/
	for (row = 1; row < 8; row++)
		val |= m_andmap[row];
	if (m_andmap[0] == 1)
	{
		m_columnvalue[2] = 1-val;
		m_columnvalue[3] = val;
		m_outvalue[0]    = 1-val;
	}
	else
	{
		/*pin is in INPUT configuration so it doesn't create output...*/
		m_columnvalue[2] = 0;
		m_columnvalue[3] = 1;
	}

/* O pin #18 (D1) */
	val = 0; /*prepare for OR*/
	for (row = 8; row < 16; row++)
		val |= m_andmap[row];
	m_columnvalue[6] = 1-val;
	m_columnvalue[7] = val;
	m_outvalue[1]    = 1-val;

/* O pin #17 (D2) */
	val = 0; /*prepare for OR*/
	for (row = 16; row < 24; row++)
		val |= m_andmap[row];
	m_columnvalue[10] = 1-val;
	m_columnvalue[11] = val;
	m_outvalue[2]     = 1-val;

/* O pin #16 (D3) */
	val = 0; /*prepare for OR*/
	for (row = 24; row < 32; row++)
		val |= m_andmap[row];
	m_columnvalue[14] = 1-val;
	m_columnvalue[15] = val;
	m_outvalue[3]     = 1-val;

/* O pin #15 (D4) */
	val = 0; /*prepare for OR*/
	for (row = 32; row < 40; row++)
		val |= m_andmap[row];
	m_columnvalue[18] = 1-val;
	m_columnvalue[19] = val;
	m_outvalue[4]     = 1-val;

/* O pin #14 (D5) */
	val = 0; /*prepare for OR*/
	for (row = 40; row < 48; row++)
		val |= m_andmap[row];
	m_columnvalue[22] = 1-val;
	m_columnvalue[23] = val;
	m_outvalue[5]     = 1-val;

/* O pin #13 (D6) */
	val = 0; /*prepare for OR*/
	for (row = 48; row < 56; row++)
		val |= m_andmap[row];
	m_columnvalue[26] = 1-val;
	m_columnvalue[27] = val;
	m_outvalue[6]     = 1-val;

/* I/O pin #12 */
	val = 0; /*prepare for OR*/
	for (row = 57; row < 64; row++)
		val |= m_andmap[row];
	if (m_andmap[56] == 1)
	{
		m_columnvalue[30] = 1-val;
		m_columnvalue[31] = val;
		m_outvalue[7]     = 1-val;
	}
	else
	{
		/*pin is in INPUT configuration so it doesn't create output...*/
		m_columnvalue[30] = 0;
		m_columnvalue[31] = 1;
	}

}


void bagman_state::pal16r6_w(offs_t offset, uint8_t data)
{
	uint8_t line = offset * 4;
	m_columnvalue[line    ] = data & 1;
	m_columnvalue[line + 1] = 1 - (data & 1);
}

void bagman_state::machine_reset()
{
	pal16r6_w(0, 1);  /*pin 2*/
	pal16r6_w(1, 1);  /*pin 3*/
	pal16r6_w(2, 1);  /*pin 4*/
	pal16r6_w(3, 1);  /*pin 5*/
	pal16r6_w(4, 1);  /*pin 6*/
	pal16r6_w(5, 1);  /*pin 7*/
	pal16r6_w(6, 1);  /*pin 8*/
	pal16r6_w(7, 1);  /*pin 9*/
	update_pal();
}

uint8_t bagman_state::pal16r6_r()
{
	update_pal();
	return  (m_outvalue[6]) + (m_outvalue[5] << 1) + (m_outvalue[4] << 2) +
		(m_outvalue[3] << 3) + (m_outvalue[2] << 4) + (m_outvalue[1] << 5);

/* Bagman schematics show that this is right mapping order of PAL outputs to bits.
** This is the PAL 16R6 shown almost in the middle of the schematics.
** The /RD4 line goes low (active) whenever CPU reads from memory address a000.
*/
}