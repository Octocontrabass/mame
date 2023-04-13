// license: BSD-3-Clause
// copyright-holders: Dirk Best
/****************************************************************************

    Liberty Electronics Freedom 220 Video Display Terminal

    VT220 compatible serial terminal

    Hardware:
    - Z8400A Z80A
    - 4 MHz XTAL (next to CPU)
    - 3x 2764 (next to CPU)
    - 6x TMM2016AP-10 or D446C-2 (2k)
    - SCN2674B C4N40
    - SCB2675B C5N40
    - 2x 2732A (next to CRT controller)
    - 2x S68B10P (128 byte SRAM)
    - M5L8253P-5
    - 3x D8251AC
    - 18.432 MHz XTAL

    Keyboard:
    - SCN8050 (8039)
    - 2716 labeled "121"
    - UA555TC
    - Speaker

    External:
    - DB25 connector "Main Port"
    - DB25 connector "Auxialiary Port"
    - Keyboard connector
    - Expansion slot

    TODO:
    - Verify CRT controller hookup
    - Cursor
    - Keyboard

****************************************************************************/

#include "emu.h"
#include "bus/rs232/rs232.h"
#include "cpu/z80/z80.h"
#include "machine/i8251.h"
#include "machine/pit8253.h"
#include "video/scn2674.h"
#include "emupal.h"
#include "screen.h"


namespace {


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

class freedom220_state : public driver_device
{
public:
	freedom220_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_avdc(*this, "avdc"),
		m_screen(*this, "screen"),
		m_palette(*this, "palette"),
		m_usart(*this, "usart%u", 0U),
		m_chargen(*this, "chargen"),
		m_translate(*this, "translate"),
		m_charram(*this, "charram%u", 0U),
		m_attrram(*this, "attrram%u", 0U),
		m_current_line(0),
		m_nmi_enabled(false)
	{ }

	void freedom220(machine_config &config);

protected:
	virtual void machine_start() override;
	virtual void machine_reset() override;

private:
	required_device<z80_device> m_maincpu;
	required_device<scn2674_device> m_avdc;
	required_device<screen_device> m_screen;
	required_device<palette_device> m_palette;
	required_device_array<i8251_device, 3> m_usart;
	required_region_ptr<uint8_t> m_chargen;
	required_region_ptr<uint8_t> m_translate;
	required_shared_ptr_array<uint8_t, 2> m_charram;
	required_shared_ptr_array<uint8_t, 2> m_attrram;

	uint8_t m_current_line;
	bool m_nmi_enabled;

	void mem_map(address_map &map);
	void io_map(address_map &map);

	void row_buffer_map(address_map &map);
	uint8_t mbc_char_r(offs_t offset);
	uint8_t mbc_attr_r(offs_t offset);
	void avdc_intr_w(int state);
	void avdc_breq_w(int state);
	SCN2674_DRAW_CHARACTER_MEMBER(draw_character);

	void nmi_control_w(uint8_t data);
	void nmi_w(int state);
};


//**************************************************************************
//  ADDRESS MAPS
//**************************************************************************

void freedom220_state::mem_map(address_map &map)
{
	map(0x0000, 0x5fff).rom();
	map(0x8000, 0x87ff).ram().share(m_charram[0]);
	map(0x8800, 0x8fff).ram().share(m_attrram[0]);
	map(0x9000, 0x97ff).ram().share(m_charram[1]);
	map(0x9800, 0x9fff).ram().share(m_attrram[1]);
	map(0xa000, 0xafff).ram();
}

void freedom220_state::io_map(address_map &map)
{
	map.global_mask(0xff);
	map(0x00, 0x07).rw(m_avdc, FUNC(scn2674_device::read), FUNC(scn2674_device::write));
	map(0x20, 0x23).rw("pit", FUNC(pit8253_device::read), FUNC(pit8253_device::write));
	map(0x40, 0x41).rw(m_usart[0], FUNC(i8251_device::read), FUNC(i8251_device::write));
	map(0x60, 0x61).rw(m_usart[1], FUNC(i8251_device::read), FUNC(i8251_device::write));
	map(0x80, 0x81).rw(m_usart[2], FUNC(i8251_device::read), FUNC(i8251_device::write));
	map(0x81, 0x81).lr8(NAME([] () { return 0x01; })); // hack
	map(0xa0, 0xa0).w(FUNC(freedom220_state::nmi_control_w));
}


//**************************************************************************
//  VIDEO EMULATION
//**************************************************************************

void freedom220_state::row_buffer_map(address_map &map)
{
	map.global_mask(0xff);
	map(0x00, 0xff).ram();
}

uint8_t freedom220_state::mbc_char_r(offs_t offset)
{
	if (offset >= 0x800)
		logerror("%d read char offset %04x\n", m_current_line, offset);

	return m_charram[0][(m_current_line * 0x50 + offset) & 0x7ff];
}

uint8_t freedom220_state::mbc_attr_r(offs_t offset)
{
	return m_attrram[0][(m_current_line * 0x50 + offset) & 0x7ff];
}

void freedom220_state::avdc_intr_w(int state)
{
	if (state && m_nmi_enabled)
		m_maincpu->pulse_input_line(INPUT_LINE_NMI, attotime::zero);
}

void freedom220_state::avdc_breq_w(int state)
{
	if (state)
	{
		if (m_screen->vblank())
			m_current_line = 0;
		else
			m_current_line++;
	}
}

SCN2674_DRAW_CHARACTER_MEMBER( freedom220_state::draw_character )
{
	// 765-----  unknown
	// ---4----  normal/bold
	// ----3---  underline
	// -----2--  invert
	// ------1-  blink
	// -------0  unknown

	const pen_t *const pen = m_palette->pens();

	// translation table
	const int table = 0; // 0-f
	charcode = m_translate[(table << 8) | charcode];

	uint8_t data = m_chargen[charcode << 4 | linecount];

	if (ul && (BIT(attrcode, 3)))
		data = 0xff;

	if (blink && (BIT(attrcode, 1)))
		data = 0x00;

	if (BIT(attrcode, 2))
		data = ~data;

	// TODO
	if (0 && cursor)
		data = ~data;

	// foreground/background colors
	rgb_t fg = BIT(attrcode, 4) ? pen[1] : pen[2];
	rgb_t bg = pen[0];

	// draw 8 pixels of the character
	for (int i = 0; i < 8; i++)
		bitmap.pix(y, x + i) = BIT(data, 7 - i) ? fg : bg;
}

static const gfx_layout char_layout =
{
	8, 12,
	RGN_FRAC(1, 1),
	1,
	{ 0 },
	{ STEP8(0, 1) },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8 },
	8 * 16
};

static GFXDECODE_START(chars)
	GFXDECODE_ENTRY("chargen", 0, char_layout, 0, 1)
GFXDECODE_END


//**************************************************************************
//  MACHINE EMULATION
//**************************************************************************

void freedom220_state::nmi_control_w(uint8_t data)
{
	logerror("nmi_control_w: %02x\n", data);

	// unknown if a simple write is enough
	m_nmi_enabled = true;
}

void freedom220_state::machine_start()
{
	// register for save states
	save_item(NAME(m_current_line));
	save_item(NAME(m_nmi_enabled));
}

void freedom220_state::machine_reset()
{
	m_nmi_enabled = false;
}


//**************************************************************************
//  MACHINE DEFINTIONS
//**************************************************************************

void freedom220_state::freedom220(machine_config &config)
{
	Z80(config, m_maincpu, 4_MHz_XTAL);
	m_maincpu->set_addrmap(AS_PROGRAM, &freedom220_state::mem_map);
	m_maincpu->set_addrmap(AS_IO, &freedom220_state::io_map);

	SCREEN(config, m_screen, SCREEN_TYPE_RASTER);
	m_screen->set_color(rgb_t::green());
	m_screen->set_raw(16000000, 768, 0, 640, 321, 0, 300); // clock unverified
	m_screen->set_screen_update(m_avdc, FUNC(scn2674_device::screen_update));

	PALETTE(config, m_palette, palette_device::MONOCHROME);

	GFXDECODE(config, "gfxdecode", m_palette, chars);

	SCN2674(config, m_avdc, 16000000 / 8); // clock unverified
	m_avdc->intr_callback().set(FUNC(freedom220_state::avdc_intr_w));
	m_avdc->breq_callback().set(FUNC(freedom220_state::avdc_breq_w));
	m_avdc->set_screen(m_screen);
	m_avdc->set_character_width(8); // unverified
	m_avdc->set_addrmap(0, &freedom220_state::row_buffer_map);
	m_avdc->set_addrmap(1, &freedom220_state::row_buffer_map);
	m_avdc->set_display_callback(FUNC(freedom220_state::draw_character));
	m_avdc->mbc_char_callback().set(FUNC(freedom220_state::mbc_char_r));
	m_avdc->mbc_attr_callback().set(FUNC(freedom220_state::mbc_attr_r));

	pit8253_device &pit(PIT8253(config, "pit", 0));
	pit.set_clk<0>(18.432_MHz_XTAL / 10);
	pit.set_clk<1>(18.432_MHz_XTAL / 10);
	pit.set_clk<2>(18.432_MHz_XTAL / 10);
	pit.out_handler<0>().set(m_usart[2], FUNC(i8251_device::write_txc));
	pit.out_handler<0>().append(m_usart[2], FUNC(i8251_device::write_rxc));
	pit.out_handler<1>().set(m_usart[1], FUNC(i8251_device::write_txc));
	pit.out_handler<1>().append(m_usart[1], FUNC(i8251_device::write_rxc));
	pit.out_handler<2>().set(m_usart[0], FUNC(i8251_device::write_txc));
	pit.out_handler<2>().append(m_usart[0], FUNC(i8251_device::write_rxc));

	I8251(config, m_usart[0], 0); // unknown clock
	m_usart[0]->rxrdy_handler().set_inputline("maincpu", INPUT_LINE_IRQ0); // ?
	m_usart[0]->txd_handler().set("mainport", FUNC(rs232_port_device::write_txd));
	m_usart[0]->rts_handler().set("mainport", FUNC(rs232_port_device::write_rts));

	I8251(config, m_usart[1], 0); // unknown clock

	I8251(config, m_usart[2], 0); // unknown clock

	rs232_port_device &mainport(RS232_PORT(config, "mainport", default_rs232_devices, nullptr));
	mainport.rxd_handler().set(m_usart[0], FUNC(i8251_device::write_rxd));
	mainport.cts_handler().set(m_usart[0], FUNC(i8251_device::write_cts));
}


//**************************************************************************
//  ROM DEFINITIONS
//**************************************************************************

ROM_START( free220 )
	ROM_REGION(0x6000, "maincpu", 0)
	ROM_LOAD("m122010__8cdd.ic213", 0x0000, 0x2000, CRC(a1181809) SHA1(0ec0fd30c8a55f0bb9e1c6453120ab9a696f9041))
	ROM_LOAD("m222010__04c8.ic212", 0x2000, 0x2000, CRC(ddd1e5eb) SHA1(3e3998035721050cd2019474343f072dade6589d))
	ROM_LOAD("m322010__8121.ic214", 0x4000, 0x2000, CRC(eeaa4b44) SHA1(93402e00205d7220f5e248a902ed92de4bbe6dd8))

	ROM_REGION(0x1000, "chargen", 0)
	ROM_LOAD("g022010__d64e.bin", 0x0000, 0x1000, CRC(a4482adc) SHA1(98479f6396743da6cf23909ff5a0097e9f021e3b))

	ROM_REGION(0x1000, "translate", 0)
	ROM_LOAD("t022010__61f0.bin", 0x0000, 0x1000, CRC(00461116) SHA1(79a53a557ea4386b3e85a312731c6c0763ab46cc))

	ROM_REGION(0x800, "keyboard", 0)
	ROM_LOAD("121.bin", 0x000, 0x800, CRC(ee491f39) SHA1(477eb9f3d3abc89cfc9b5f9a924a794ca48750c4))
ROM_END


} // anonymous namespace


//**************************************************************************
//  SYSTEM DRIVERS
//**************************************************************************

//    YEAR  NAME     PARENT  COMPAT  MACHINE     INPUT  CLASS             INIT        COMPANY                FULLNAME       FLAGS
COMP( 1984, free220, 0,      0,      freedom220, 0,     freedom220_state, empty_init, "Liberty Electronics", "Freedom 220", MACHINE_NOT_WORKING | MACHINE_NO_SOUND | MACHINE_SUPPORTS_SAVE )
