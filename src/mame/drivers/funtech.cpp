// license:BSD-3-Clause
// copyright-holders:David Haywood
/*

program rom contains the following details

COMPANY:FUN TECH CORPORATION
PRODUCT-NAME:SUPER TWO IN ONE
PROJECTOR:TIEN YUAN CHIEN,NOMA
HARDWARE-DESIGNER:EN YU CHENG
SOFTWARE-DESIGNER:RANG CHANG LI,CHIH HNI HUANG,WEN CHANG LIN
PROGRAM-VERSION:1.0
PROGRAM-DATE:09/23/1993

8x8 tiles and 8x32 reels, likely going to be very similar to skylncr.cpp or goldstar.cpp (which are both very similar anyway)
palette addresses are the same as unkch in goldstar.cpp, but the io stuff is definitely different here

board has an M5255 for sound
and an unpopulated position for a YM2413 or UM3567

*/

#include "emu.h"

#include "cpu/z80/z80.h"
#include "sound/ay8910.h"
#include "machine/nvram.h"
#include "machine/ticket.h"
#include "emupal.h"
#include "screen.h"
#include "speaker.h"
#include "tilemap.h"

#include "fts2in1.lh"



class fun_tech_corp_state : public driver_device
{
public:
	fun_tech_corp_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_fgram(*this, "fgram"),
		m_reel_ram(*this, "reel_ram.%u", 0U),
		m_reel_scroll(*this, "reel_scroll.%u", 0U),
		m_reel1_alt_scroll(*this, "reel1_alt_scroll"),
		m_maincpu(*this, "maincpu"),
		m_hopper(*this, "hopper"),
		m_gfxdecode(*this, "gfxdecode"),
		m_lamps(*this, "lamp%u", 0U)
	{ }

	void funtech(machine_config &config);

protected:
	virtual void machine_start() override;
	virtual void video_start() override;

private:
	required_shared_ptr<uint8_t> m_fgram;
	required_shared_ptr_array<uint8_t, 3> m_reel_ram;
	required_shared_ptr_array<uint8_t, 3> m_reel_scroll;
	required_shared_ptr<uint8_t> m_reel1_alt_scroll;
	required_device<cpu_device> m_maincpu;
	required_device<ticket_dispenser_device> m_hopper;
	required_device<gfxdecode_device> m_gfxdecode;
	output_finder<6> m_lamps;

	uint8_t m_vreg;

	tilemap_t *m_fg_tilemap;

	INTERRUPT_GEN_MEMBER(vblank_interrupt);

	void lamps_w(uint8_t data);
	void coins_w(uint8_t data);
	void vreg_w(uint8_t data);

	void fgram_w(offs_t offset, uint8_t data);

	TILE_GET_INFO_MEMBER(get_fg_tile_info);

	tilemap_t *m_reel_tilemap[3];

	template<uint8_t Reel> void reel_ram_w(offs_t offset, uint8_t data);

	template<uint8_t Reel> TILE_GET_INFO_MEMBER(get_reel_tile_info);

	uint32_t screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	void funtech_io_map(address_map &map);
	void funtech_map(address_map &map);
};


TILE_GET_INFO_MEMBER(fun_tech_corp_state::get_fg_tile_info)
{
	int code = m_fgram[tile_index];
	int attr = m_fgram[tile_index+0x800];

	code |= (attr & 0x0f) << 8;

	if (m_vreg&1) code |= 0x1000;

	tileinfo.set(0,
			code,
			attr>>4,
			0);
}

template<uint8_t Reel>
TILE_GET_INFO_MEMBER(fun_tech_corp_state::get_reel_tile_info)
{
	int code = m_reel_ram[Reel][tile_index];
	if (m_vreg & 0x4) code |= 0x100;
	if (m_vreg & 0x8) code |= 0x200;

	tileinfo.set(1,
			code,
			0,
			0);
}

template<uint8_t Reel>
void fun_tech_corp_state::reel_ram_w(offs_t offset, uint8_t data)
{
	m_reel_ram[Reel][offset] = data;
	m_reel_tilemap[Reel]->mark_tile_dirty(offset);
}

void fun_tech_corp_state::video_start()
{
	m_fg_tilemap = &machine().tilemap().create(*m_gfxdecode, tilemap_get_info_delegate(*this, FUNC(fun_tech_corp_state::get_fg_tile_info)), TILEMAP_SCAN_ROWS, 8, 8, 64, 32);
	m_fg_tilemap->set_transparent_pen(0);

	m_reel_tilemap[0] = &machine().tilemap().create(*m_gfxdecode, tilemap_get_info_delegate(*this, FUNC(fun_tech_corp_state::get_reel_tile_info<0>)), TILEMAP_SCAN_ROWS, 8, 32, 64, 8);
	m_reel_tilemap[1] = &machine().tilemap().create(*m_gfxdecode, tilemap_get_info_delegate(*this, FUNC(fun_tech_corp_state::get_reel_tile_info<1>)), TILEMAP_SCAN_ROWS, 8, 32, 64, 8);
	m_reel_tilemap[2] = &machine().tilemap().create(*m_gfxdecode, tilemap_get_info_delegate(*this, FUNC(fun_tech_corp_state::get_reel_tile_info<2>)), TILEMAP_SCAN_ROWS, 8, 32, 64, 8);

	m_reel_tilemap[0]->set_scroll_cols(64);
	m_reel_tilemap[1]->set_scroll_cols(64);
	m_reel_tilemap[2]->set_scroll_cols(64);

}

void fun_tech_corp_state::fgram_w(offs_t offset, uint8_t data)
{
	m_fgram[offset] = data;
	m_fg_tilemap->mark_tile_dirty(offset&0x7ff);
}


uint32_t fun_tech_corp_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	bitmap.fill(0, cliprect);

	if (!(m_vreg & 0x40))
	{
		for (uint8_t reel = 0; reel < 3; reel++)
		{
			for (int i = 0; i < 64; i++)
			{
				m_reel_tilemap[reel]->set_scrolly(i, m_reel_scroll[reel][i]);
			}
		}

		const rectangle visible1(0 * 8, (14 + 48) * 8 - 1, 4 * 8, (4 + 7) * 8 - 1);
		const rectangle visible2(0 * 8, (14 + 48) * 8 - 1, 12 * 8, (12 + 7) * 8 - 1);
		const rectangle visible3(0 * 8, (14 + 48) * 8 - 1, 18 * 8, (18 + 7) * 8 - 1);

		m_reel_tilemap[0]->draw(screen, bitmap, visible1, 0, 0);
		m_reel_tilemap[1]->draw(screen, bitmap, visible2, 0, 0);
		m_reel_tilemap[2]->draw(screen, bitmap, visible3, 0, 0);
	}
	else
	{
		// this mode seems to draw reel1 as fullscreen using a different set of scroll regs
		for (int i = 0; i < 64; i++)
		{
			m_reel_tilemap[0]->set_scrolly(i, m_reel1_alt_scroll[i]);
		}

		m_reel_tilemap[0]->draw(screen, bitmap, cliprect, 0, 0);
	}


	m_fg_tilemap->draw(screen, bitmap, cliprect, 0, 0);
	return 0;
}



INTERRUPT_GEN_MEMBER(fun_tech_corp_state::vblank_interrupt)
{
//  if (m_nmi_enable)
		device.execute().pulse_input_line(INPUT_LINE_NMI, attotime::zero);
}



void fun_tech_corp_state::funtech_map(address_map &map)
{
	map(0x0000, 0xbfff).rom();

	map(0xc000, 0xc1ff).ram().w("palette", FUNC(palette_device::write8)).share("palette");
	map(0xc800, 0xc9ff).ram().w("palette", FUNC(palette_device::write8_ext)).share("palette_ext");

	map(0xd000, 0xd7ff).rom(); // maybe

	map(0xd800, 0xdfff).ram().share("nvram");

	map(0xe000, 0xefff).ram().w(FUNC(fun_tech_corp_state::fgram_w)).share(m_fgram);
	map(0xf000, 0xf1ff).ram().w(FUNC(fun_tech_corp_state::reel_ram_w<0>)).share(m_reel_ram[0]);
	map(0xf200, 0xf3ff).ram().w(FUNC(fun_tech_corp_state::reel_ram_w<1>)).share(m_reel_ram[1]);
	map(0xf400, 0xf5ff).ram().w(FUNC(fun_tech_corp_state::reel_ram_w<2>)).share(m_reel_ram[2]);
	map(0xf600, 0xf7ff).ram();

	map(0xf840, 0xf87f).ram().share(m_reel_scroll[0]);
	map(0xf880, 0xf8bf).ram().share(m_reel_scroll[1]);
	map(0xf900, 0xf93f).ram().share(m_reel_scroll[2]);

	map(0xf9c0, 0xf9ff).ram().share(m_reel1_alt_scroll); // or a mirror, gets used in 'full screen' mode.
}


void fun_tech_corp_state::lamps_w(uint8_t data)
{
	for (int i = 0; i < 6; i++)
		m_lamps[i] = BIT(data, i);

	// bit 6 (0x40) is set when displaying a hand in poker, but there are only six lamp outputs
	// bit 7 (0x80) is always set
}

void fun_tech_corp_state::coins_w(uint8_t data)
{
	if (data & 0x01) logerror("coins_w %02x\n", data);

	machine().bookkeeping().coin_counter_w(4, BIT(data, 1)); // COUNTER HOPPER
	machine().bookkeeping().coin_counter_w(0, BIT(data, 2)); // COUNTER A (coin A)
	machine().bookkeeping().coin_counter_w(1, BIT(data, 3)); // COUNTER B (keyin B)
	machine().bookkeeping().coin_counter_w(2, BIT(data, 4)); // COUNTER C (coin C)
	machine().bookkeeping().coin_counter_w(3, BIT(data, 5)); // COUNTER D (coin D)
	machine().bookkeeping().coin_counter_w(5, BIT(data, 6)); // COUNTER CREDIT OUT

	m_hopper->motor_w(BIT(data, 7));
}

void fun_tech_corp_state::vreg_w(uint8_t data)
{
	if (data & 0xb2) logerror("vreg_w %02x\n", data);

	// -x-- rr-t
	// t = text tile bank
	// r = reel tile bank
	// x = show reel 1 full screen?

	m_vreg = data;
	m_fg_tilemap->mark_all_dirty();
	m_reel_tilemap[0]->mark_all_dirty();
}



void fun_tech_corp_state::funtech_io_map(address_map &map)
{
	map.global_mask(0xff);
	// lamps?
	map(0x00, 0x00).w(FUNC(fun_tech_corp_state::lamps_w));
	map(0x01, 0x01).w(FUNC(fun_tech_corp_state::coins_w));

	map(0x03, 0x03).w(FUNC(fun_tech_corp_state::vreg_w));

	map(0x04, 0x04).portr("IN0");
	map(0x05, 0x05).portr("IN1");
	map(0x06, 0x06).portr("DSW4");
	map(0x07, 0x07).portr("DSW3");

	map(0x10, 0x10).r("aysnd", FUNC(ay8910_device::data_r));
	map(0x11, 0x11).w("aysnd", FUNC(ay8910_device::data_w));
	map(0x12, 0x12).w("aysnd", FUNC(ay8910_device::address_w));
}

static INPUT_PORTS_START( funtech )
	PORT_START("IN0")
	// the buttons are all multi-purpose as it's a 2-in-1.
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_POKER_HOLD5 )   PORT_NAME("Hold 5, Bet")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )         PORT_NAME("Coin A")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )         PORT_NAME("Coin C")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )         PORT_NAME("Coin D")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_POKER_HOLD1 )   PORT_NAME("Hold 1, Take Score, Odds, Left Stop") // shows odds in 8-liner game
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_POKER_HOLD4 )   PORT_NAME("Hold 4, Double-Up") // manual suggests this shows odds, but it doesn't
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_POKER_HOLD3 )   PORT_NAME("Hold 3, Small, Right Stop")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_POKER_HOLD2 )   PORT_NAME("Hold 2, Big, Center Stop")

	PORT_START("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_GAMBLE_BOOK )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_GAMBLE_PAYOUT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_CUSTOM ) PORT_READ_LINE_DEVICE_MEMBER("hopper", ticket_dispenser_device, line_r)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_GAMBLE_KEYOUT ) PORT_NAME("Credit Out")
	PORT_DIPNAME( 0x10, 0x10, "IN1-10" ) // prevents start from working in the poker game at least
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE_NO_TOGGLE(   0x20, IP_ACTIVE_LOW )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )        PORT_NAME("Start, All Stop")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_GAMBLE_KEYIN )  PORT_NAME("Keyin B")

	// the board contains 4 banks of 8 dipswitches
	PORT_START("DSW1")
	PORT_DIPNAME( 0x03, 0x03, "Hopper Limit" )                PORT_DIPLOCATION("NO. 1:1,2")
	PORT_DIPSETTING(    0x00, "300" )
	PORT_DIPSETTING(    0x02, "500" )
	PORT_DIPSETTING(    0x01, "1000" )
	PORT_DIPSETTING(    0x03, "Unlimited" )
	PORT_DIPNAME( 0x1c, 0x1c, "Credit Limit" )                PORT_DIPLOCATION("NO. 1:3,4,5")
	PORT_DIPSETTING(    0x00, "5,000" )
	PORT_DIPSETTING(    0x10, "10,000" )
	PORT_DIPSETTING(    0x08, "20,000" )
	PORT_DIPSETTING(    0x18, "30,000" )
	PORT_DIPSETTING(    0x04, "40,000" )
	PORT_DIPSETTING(    0x14, "50,000" )
	PORT_DIPSETTING(    0x0c, "100,000" )
	PORT_DIPSETTING(    0x1c, "Unlimited" )
	PORT_DIPNAME( 0x20, 0x20, "Credit Limit Display" )        PORT_DIPLOCATION("NO. 1:6")
	PORT_DIPSETTING(    0x20, "Not Displayed" )
	PORT_DIPSETTING(    0x00, "Displayed" )
	PORT_DIPNAME( 0xc0, 0xc0, "Coin A Rate" )                 PORT_DIPLOCATION("NO. 1:7,8")
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, "1 Coin/10 Credits" )

	PORT_START("DSW2")
	PORT_DIPNAME( 0x07, 0x07, "Keyin B Rate" )                PORT_DIPLOCATION("NO. 2:1,2,3")
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x06, "1 Coin/10 Credits" )
	PORT_DIPSETTING(    0x01, "1 Coin/20 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/25 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/50 Credits" )
	PORT_DIPSETTING(    0x07, "1 Coin/100 Credits" )
	PORT_DIPNAME( 0x18, 0x18, "Coin C Rate" )                 PORT_DIPLOCATION("NO. 2:4,5")
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x18, "1 Coin/10 Credits" )
	PORT_DIPNAME( 0xe0, 0xe0, "Coin D Rate" )                 PORT_DIPLOCATION("NO. 2:6,7,8")
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, "1 Coin/10 Credits" )
	PORT_DIPSETTING(    0x60, "1 Coin/25 Credits" )
	PORT_DIPSETTING(    0xe0, "1 Coin/50 Credits" )

	PORT_START("DSW3")
	PORT_DIPNAME( 0x0f, 0x0f, "Main Game Pay Rate" )          PORT_DIPLOCATION("NO. 3:1,2,3,4")
	PORT_DIPSETTING(    0x00, "50%" )
	PORT_DIPSETTING(    0x08, "53%" )
	PORT_DIPSETTING(    0x04, "56%" )
	PORT_DIPSETTING(    0x0c, "59%" )
	PORT_DIPSETTING(    0x02, "62%" )
	PORT_DIPSETTING(    0x0a, "65%" )
	PORT_DIPSETTING(    0x06, "68%" )
	PORT_DIPSETTING(    0x0e, "71%" )
	PORT_DIPSETTING(    0x01, "74%" )
	PORT_DIPSETTING(    0x09, "77%" )
	PORT_DIPSETTING(    0x05, "80%" )
	PORT_DIPSETTING(    0x0d, "83%" )
	PORT_DIPSETTING(    0x03, "86%" )
	PORT_DIPSETTING(    0x0b, "89%" )
	PORT_DIPSETTING(    0x07, "92%" )
	PORT_DIPSETTING(    0x0f, "95%" )
	PORT_DIPNAME( 0x10, 0x10, "Double Up Game" )              PORT_DIPLOCATION("NO. 3:5")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Double Up Pay Rate" )          PORT_DIPLOCATION("NO. 3:6")
	PORT_DIPSETTING(    0x00, "80%" )
	PORT_DIPSETTING(    0x20, "90%" )
	PORT_DIPNAME( 0x40, 0x40, "Double Up 7 Player Value" )    PORT_DIPLOCATION("NO. 3:7")
	PORT_DIPSETTING(    0x40, "Loss" )
	PORT_DIPSETTING(    0x00, "Draw" )
	PORT_DIPNAME( 0x80, 0x80, "Double Up Girl Display" )      PORT_DIPLOCATION("NO. 3:8") // if set to No girl remains clothed when winning
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )

	PORT_START("DSW4")
	PORT_DIPNAME( 0x01, 0x01, "Reel Speed" )                  PORT_DIPLOCATION("NO. 4:1")
	PORT_DIPSETTING(    0x01, "Low" )
	PORT_DIPSETTING(    0x00, "High" )
	PORT_DIPNAME( 0x06, 0x06, "Max Bet" )                     PORT_DIPLOCATION("NO. 4:2,3")
	PORT_DIPSETTING(    0x00, "8" )
	PORT_DIPSETTING(    0x04, "16" )
	PORT_DIPSETTING(    0x02, "32" )
	PORT_DIPSETTING(    0x06, "64" )
	PORT_DIPNAME( 0x08, 0x08, "Min Bet for Any Bonus" )       PORT_DIPLOCATION("NO. 4:4")
	PORT_DIPSETTING(    0x00, "8" )
	PORT_DIPSETTING(    0x08, "16" )
	PORT_DIPNAME( 0x30, 0x30, "Bonus Games Entry Condition" ) PORT_DIPLOCATION("NO. 4:5,6")
	PORT_DIPSETTING(    0x00, "1-3-1-5-1" )
	PORT_DIPSETTING(    0x20, "1-4-1-6-1" )
	PORT_DIPSETTING(    0x10, "1-5-1-7-1" )
	PORT_DIPSETTING(    0x30, "1-6-1-8-1" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )             PORT_DIPLOCATION("NO. 4:7") // unused according to manual
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )             PORT_DIPLOCATION("NO. 4:8") // unused according to manual
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


static const gfx_layout tiles8x32_layout =
{
	8,32,
	RGN_FRAC(1,1),
	8,
	{ 0, 1, 2, 3, 4 ,5, 6, 7 },
	{ 0, 8, 16, 24, 32, 40, 48, 56 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64, 8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64, 16*64, 17*64, 18*64, 19*64, 20*64, 21*64, 22*64, 23*64, 24*64, 25*64, 26*64, 27*64, 28*64, 29*64, 30*64, 31*64 },
	32*64
};


static const gfx_layout tiles8x8_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};


static GFXDECODE_START( gfx_funtech )
	GFXDECODE_ENTRY( "gfx1", 0, tiles8x8_layout, 0, 16 )
	GFXDECODE_ENTRY( "gfx2", 0, tiles8x32_layout, 0x100, 1 )
GFXDECODE_END




void fun_tech_corp_state::machine_start()
{
	m_lamps.resolve();

	save_item(NAME(m_vreg));
}


void fun_tech_corp_state::funtech(machine_config &config)
{
	/* basic machine hardware */
	Z80(config, m_maincpu, 4000000);         /* ? MHz */
	m_maincpu->set_addrmap(AS_PROGRAM, &fun_tech_corp_state::funtech_map);
	m_maincpu->set_addrmap(AS_IO, &fun_tech_corp_state::funtech_io_map);
	m_maincpu->set_vblank_int("screen", FUNC(fun_tech_corp_state::vblank_interrupt));

	/* video hardware */
	screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_RASTER));
	screen.set_refresh_hz(60);
	screen.set_vblank_time(subseconds::from_usec(0));
	screen.set_size(512, 256);
	screen.set_visarea(0, 512-1, 8, 256-8-1);
	screen.set_screen_update(FUNC(fun_tech_corp_state::screen_update));
	screen.set_palette("palette");

	GFXDECODE(config, m_gfxdecode, "palette", gfx_funtech);
	PALETTE(config, "palette").set_format(palette_device::xBGR_555, 0x200);

	NVRAM(config, "nvram", nvram_device::DEFAULT_ALL_1);

	TICKET_DISPENSER(config, m_hopper, attotime::from_msec(50), TICKET_MOTOR_ACTIVE_HIGH, TICKET_STATUS_ACTIVE_HIGH);

	/* sound hardware */
	SPEAKER(config, "mono").front_center();

	ay8910_device &aysnd(AY8910(config, "aysnd", 1500000)); /* M5255, ? MHz */
	aysnd.port_a_read_callback().set_ioport("DSW1");
	aysnd.port_b_read_callback().set_ioport("DSW2");
	aysnd.add_route(ALL_OUTPUTS, "mono", 1.00);
}



ROM_START( fts2in1 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "u5.bin", 0x00000, 0x10000, CRC(ab19fd28) SHA1(a65ff732e0aaaec256cc63beff5f24419e691645) )

	ROM_REGION( 0x80000, "gfx1", 0 ) // crc printed on label matches half the data, even if chip was double size. Seen on a second PCB with correct sized ROM.
	ROM_LOAD( "u18.bin", 0x00000, 0x80000, CRC(d1154aac) SHA1(dc03c4b7a4dfda2a30bfabaeb0ce053660961663) ) // 1ST AND 2ND HALF IDENTICAL

	ROM_REGION( 0x40000, "gfx2", 0 )
	ROM_LOAD16_BYTE( "u29.bin", 0x00000, 0x20000, CRC(ed6a1e2f) SHA1(2c72e764c7c8091a8fa1dfc257a84d61e2da0e4b) )
	ROM_LOAD16_BYTE( "u30.bin", 0x00001, 0x20000, CRC(d572bddc) SHA1(06499aeb47085a02af9eb4987ed987f9a3a397f7) )
ROM_END

GAMEL( 1993, fts2in1, 0, funtech, funtech, fun_tech_corp_state, empty_init, ROT0, "Fun Tech Corporation", "Super Two In One", MACHINE_SUPPORTS_SAVE, layout_fts2in1 )
