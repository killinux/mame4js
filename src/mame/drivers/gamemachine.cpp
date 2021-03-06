// license:BSD-3-Clause
// copyright-holders:hap
// thanks-to:Sean Riddle, Couriersud
/******************************************************************************

Waddingtons 2001: The Game Machine

It's a tabletop electronic game machine + calculator.
It was possibly created by VTech, but they didn't distribute it by themselves
until later in 1980 as the Computer Game System. There's also a handheld version
"Mini Game Machine". VTech later made a sequel "Game Machine 2" with 5 games.

Hardware notes:
- Mostek MK3870 MCU, 2KB internal ROM
- 12 digits 7seg VFD panel
- MC1455P(555 timer) + bunch of discrete components for sound

TODO:
- MCU frequency was measured approx 2.1MHz on its XTL2 pin, but considering that
  the MK3870 has an internal /2 divider, this is way too slow when compared to
  video references of the game

BTANB:
- some digit segments get stuck after crashing in the GP game

*******************************************************************************

After boot, press a number to start a game:
0: 4 Function Calculator (not a game)
1: Shooting Gallery
2: Black Jack
3: Code Hunter
4: Grand Prix

Screen and keypad overlays were provided for each game, though the default keypad
labels already show the alternate functions.

keypad reference (mapped to PC keyboard A-row and Z-row by default)

Calculator:
  [RET] [MS ] [MR ] [+/-] [.  ] [+= ] [-= ] [x  ] [/  ] [CL ]
  [0  ] [1  ] [2  ] [3  ] [4  ] [5  ] [6  ] [7  ] [8  ] [9  ]

Shooting Gallery:
  [RET] [Cyc] [Zig] [Rnd] [   ] [   ] [   ] [   ] [   ] [   ] * Cyclic, Zigzag, Random
  [   ] [   ] [   ] [   ] [   ] [   ] [   ] [   ] [   ] [   ] * + any of 20 buttons for shooting target

Black Jack:
  [RET] [Dl ] [   ] [   ] [   ] [   ] [   ] [   ] [Hit] [Stn] * Deal, Hit, Stand
  [   ] [   ] [   ] [   ] [   ] [   ] [   ] [   ] [   ] [   ]

Code Hunter:
  [RET] [Sta] [Dis] [   ] [   ] [Ent] [   ] [Crs] [R< ] [R> ] * Start, Display, Enter, Cursor key, Review back, Review ahead
  [   ] [   ] [   ] [   ] [   ] [   ] [   ] [   ] [   ] [   ]

Grand Prix:
  [RET] [Go ] [   ] [   ] [   ] [   ] [   ] [Up ] [Up ] [Up ]
  [Brk] [Gas] [   ] [   ] [   ] [   ] [   ] [Dwn] [Dwn] [Dwn]

******************************************************************************/

#include "emu.h"
#include "cpu/f8/f8.h"
#include "video/pwm.h"
#include "machine/f3853.h"
#include "speaker.h"
#include "machine/netlist.h"
#include "audio/nl_gamemachine.h"

// internal artwork
#include "tgm.lh"

namespace {

class tgm_state : public driver_device
{
public:
	tgm_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_display(*this, "display"),
		m_audio_pin(*this, "snd_nl:p%02u", 8U),
		m_inputs(*this, "IN.%u", 0)
	{ }

	void tgm(machine_config &config);

protected:
	virtual void machine_start() override;

private:
	// devices/pointers
	required_device<cpu_device> m_maincpu;
	required_device<pwm_display_device> m_display;
	required_device_array<netlist_mame_logic_input_device, 8> m_audio_pin;
	required_ioport_array<10> m_inputs;

	void main_map(address_map &map);
	void main_io(address_map &map);

	void update_display();
	void mux1_w(u8 data);
	void mux2_w(u8 data);
	void digit_w(u8 data);
	u8 input_r();
	void sound_w(u8 data);

	u16 m_inp_mux;
	u16 m_digit_select;
	u8 m_digit_data;
};

void tgm_state::machine_start()
{
	// zerofill
	m_inp_mux = 0;
	m_digit_select = 0;
	m_digit_data = 0;

	// register for savestates
	save_item(NAME(m_inp_mux));
	save_item(NAME(m_digit_select));
	save_item(NAME(m_digit_data));
}



/******************************************************************************
    I/O
******************************************************************************/

// MK3870 ports

void tgm_state::update_display()
{
	// output VFD digit data
	m_display->matrix(m_digit_select, m_digit_data);
}

void tgm_state::mux1_w(u8 data)
{
	// P00-P06: input mux part
	m_inp_mux = (m_inp_mux & 7) | (data << 3 & 0x3f8);

	// P00-P07: digit select part
	m_digit_select = (m_digit_select & 0xf) | (data << 4);
	update_display();
}

void tgm_state::mux2_w(u8 data)
{
	// P15-P17: input mux part
	m_inp_mux = (m_inp_mux & 0x3f8) | (data >> 5 & 7);

	// P14-P17: digit select part
	m_digit_select = (m_digit_select & 0xff0) | (data >> 4 & 0xf);
	update_display();
}

void tgm_state::digit_w(u8 data)
{
	// P50-P57: digit 7seg data
	m_digit_data = bitswap<8>(data,0,1,2,3,4,5,6,7);
	update_display();
}

u8 tgm_state::input_r()
{
	u8 data = 0;

	// P12,P13: multiplexed inputs
	for (int i = 0; i < 10; i++)
		if (m_inp_mux >> i & 1)
			data |= m_inputs[i]->read();

	return data << 2;
}

void tgm_state::sound_w(u8 data)
{
	// P40-P47: 555 to speaker (see netlist above)
	for (int i = 0; i < 8; i++)
		m_audio_pin[i]->write_line(BIT(~data, i));
}



/******************************************************************************
    Address Maps
******************************************************************************/

void tgm_state::main_map(address_map &map)
{
	map.global_mask(0x07ff);
	map(0x0000, 0x07ff).rom();
}

void tgm_state::main_io(address_map &map)
{
	map(0x00, 0x00).w(FUNC(tgm_state::mux1_w));
	map(0x01, 0x01).rw(FUNC(tgm_state::input_r), FUNC(tgm_state::mux2_w));
	map(0x04, 0x07).rw("psu", FUNC(f38t56_device::read), FUNC(f38t56_device::write));
}



/******************************************************************************
    Input Ports
******************************************************************************/

static INPUT_PORTS_START( tgm )
	PORT_START("IN.0")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_COLON) PORT_CODE(KEYCODE_DEL) PORT_CODE(KEYCODE_BACKSPACE) PORT_CODE(KEYCODE_RIGHT) PORT_NAME("CL")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_SLASH) PORT_CODE(KEYCODE_9) PORT_CODE(KEYCODE_9_PAD) PORT_NAME("9")

	PORT_START("IN.1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_L) PORT_CODE(KEYCODE_SLASH_PAD) PORT_CODE(KEYCODE_LEFT) PORT_NAME(u8"??")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_STOP) PORT_CODE(KEYCODE_8) PORT_CODE(KEYCODE_8_PAD) PORT_NAME("8")

	PORT_START("IN.2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_K) PORT_CODE(KEYCODE_ASTERISK) PORT_CODE(KEYCODE_UP) PORT_NAME(u8"??")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_COMMA) PORT_CODE(KEYCODE_7) PORT_CODE(KEYCODE_7_PAD) PORT_CODE(KEYCODE_DOWN) PORT_NAME("7")

	PORT_START("IN.3")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_J) PORT_CODE(KEYCODE_MINUS_PAD) PORT_NAME("-=")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_M) PORT_CODE(KEYCODE_6) PORT_CODE(KEYCODE_6_PAD) PORT_NAME("6")

	PORT_START("IN.4")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_H) PORT_CODE(KEYCODE_PLUS_PAD) PORT_CODE(KEYCODE_ENTER) PORT_CODE(KEYCODE_ENTER_PAD) PORT_NAME("+=")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_N) PORT_CODE(KEYCODE_5) PORT_CODE(KEYCODE_5_PAD) PORT_NAME("5")

	PORT_START("IN.5")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_G) PORT_CODE(KEYCODE_DEL_PAD) PORT_NAME(".")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_B) PORT_CODE(KEYCODE_4) PORT_CODE(KEYCODE_4_PAD) PORT_NAME("4")

	PORT_START("IN.6")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_F) PORT_CODE(KEYCODE_MINUS) PORT_NAME("+/-")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_V) PORT_CODE(KEYCODE_3) PORT_CODE(KEYCODE_3_PAD) PORT_NAME("3")

	PORT_START("IN.7")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_D) PORT_CODE(KEYCODE_END) PORT_NAME("MR")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_C) PORT_CODE(KEYCODE_2) PORT_CODE(KEYCODE_2_PAD) PORT_NAME("2")

	PORT_START("IN.8")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_S) PORT_CODE(KEYCODE_HOME) PORT_NAME("MS")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_X) PORT_CODE(KEYCODE_1) PORT_CODE(KEYCODE_1_PAD) PORT_NAME("1")

	PORT_START("IN.9")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_A) PORT_CODE(KEYCODE_R) PORT_NAME("Return")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_Z) PORT_CODE(KEYCODE_0) PORT_CODE(KEYCODE_0_PAD) PORT_NAME("0")
INPUT_PORTS_END



/******************************************************************************
    Machine Configs
******************************************************************************/

void tgm_state::tgm(machine_config &config)
{
	/* basic machine hardware */
	F8(config, m_maincpu, 4000000/2); // MK3870, frequency is approximate
	m_maincpu->set_addrmap(AS_PROGRAM, &tgm_state::main_map);
	m_maincpu->set_addrmap(AS_IO, &tgm_state::main_io);

	f38t56_device &psu(F38T56(config, "psu", 4000000/2));
	psu.write_a().set(FUNC(tgm_state::sound_w));
	psu.write_b().set(FUNC(tgm_state::digit_w));

	/* video hardware */
	PWM_DISPLAY(config, m_display).set_size(12, 8);
	m_display->set_segmask(0xfff, 0xff);
	config.set_default_layout(layout_tgm);

	/* sound hardware */
	SPEAKER(config, "speaker").front_center();
	NETLIST_SOUND(config, "snd_nl", 48000)
		.set_source(NETLIST_NAME(gamemachine))
		.add_route(ALL_OUTPUTS, "speaker", 1.0);

	NETLIST_STREAM_OUTPUT(config, "snd_nl:cout0", 0, "SPK1.2").set_mult_offset(-10000.0 / 32768.0, 10000.0 * 3.75 / 32768.0);

	NETLIST_LOGIC_INPUT(config, "snd_nl:p08", "P08.IN", 0);
	NETLIST_LOGIC_INPUT(config, "snd_nl:p09", "P09.IN", 0);
	NETLIST_LOGIC_INPUT(config, "snd_nl:p10", "P10.IN", 0);
	NETLIST_LOGIC_INPUT(config, "snd_nl:p11", "P11.IN", 0);
	NETLIST_LOGIC_INPUT(config, "snd_nl:p12", "P12.IN", 0);
	NETLIST_LOGIC_INPUT(config, "snd_nl:p13", "P13.IN", 0);
	NETLIST_LOGIC_INPUT(config, "snd_nl:p14", "P14.IN", 0);
	NETLIST_LOGIC_INPUT(config, "snd_nl:p15", "P15.IN", 0);
}



/******************************************************************************
    ROM Definitions
******************************************************************************/

ROM_START( 2001tgm )
	ROM_REGION( 0x0800, "maincpu", 0 )
	ROM_LOAD("mk14154n_2001", 0x0000, 0x0800, CRC(6d524c32) SHA1(73d84e59952b751c76dff8bf259b98e1f9136b41) )
ROM_END

} // anonymous namespace



/******************************************************************************
    Drivers
******************************************************************************/

//    YEAR  NAME     PARENT CMP MACHINE  INPUT  CLASS      INIT        COMPANY, FULLNAME, FLAGS
CONS( 1978, 2001tgm, 0,      0, tgm,     tgm,   tgm_state, empty_init, "Waddingtons", "2001: The Game Machine", MACHINE_SUPPORTS_SAVE | MACHINE_IMPERFECT_SOUND )
