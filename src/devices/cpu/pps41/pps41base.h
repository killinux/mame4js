// license:BSD-3-Clause
// copyright-holders:hap
/*

  Rockwell PPS-4/1 MCU cores

*/

#ifndef MAME_CPU_PPS41_PPS41BASE_H
#define MAME_CPU_PPS41_PPS41BASE_H

#pragma once

#include "machine/pla.h"


class pps41_base_device : public cpu_device
{
public:
	// configuration helpers
	// I/O ports:

	//..

	// set MCU mask options:

	//..

protected:
	// construction/destruction
	pps41_base_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, u32 clock, int prgwidth, address_map_constructor program, int datawidth, address_map_constructor data);

	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;

	// device_execute_interface overrides
	virtual u32 execute_min_cycles() const noexcept override { return 1; }
	virtual u32 execute_max_cycles() const noexcept override { return 2; }
	virtual void execute_run() override;
	virtual void execute_one() = 0;

	// device_memory_interface overrides
	virtual space_config_vector memory_space_config() const override;

	address_space_config m_program_config;
	address_space_config m_data_config;
	address_space *m_program;
	address_space *m_data;

	int m_icount;

	// fixed settings or mask options
	int m_prgwidth; // ROM/RAM address size
	int m_datawidth; // "
	u16 m_prgmask; // "
	u16 m_datamask; // "

	optional_device<pla_device> m_opla; // segment output PLA

	// i/o handlers
	//..

	// internal state, regs
	u16 m_pc;
	u16 m_prev_pc;
	u8 m_op;
	u8 m_prev_op;
	u8 m_prev2_op;
	int m_stack_levels;
	u16 m_stack[2]; // max 2

	u8 m_a;
	u8 m_b;
	u8 m_prev_b;
	u8 m_prev2_b;
	u8 m_ram_addr;
	bool m_ram_delay;
	bool m_sag;
	int m_c;
	int m_prev_c;
	int m_c_in;
	bool m_c_delay;
	bool m_skip;
	int m_skip_count;

	// misc handlers
	virtual bool op_is_prefix(u8 op) = 0;
	void cycle();
	void increment_pc();
};


#endif // MAME_CPU_PPS41_PPS41BASE_H
