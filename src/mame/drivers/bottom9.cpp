// license:BSD-3-Clause
// copyright-holders:Nicola Salmoria
/***************************************************************************

    Bottom of the Ninth (c) 1989 Konami

    Similar to S.P.Y.

    driver by Nicola Salmoria

    2008-07
    Dip locations and factory settings verified with US manual.
    To be verified for Main Stadium

***************************************************************************/

#include "emu.h"
#include "includes/bottom9.h"
#include "includes/konamipt.h"

#include "cpu/z80/z80.h"
#include "cpu/m6809/hd6309.h"
#include "machine/gen_latch.h"
#include "machine/watchdog.h"
#include "speaker.h"


uint8_t bottom9_state::k052109_051960_r(offs_t offset)
{
	if (m_k052109->get_rmrd_line() == CLEAR_LINE)
	{
		if (offset >= 0x3800 && offset < 0x3808)
			return m_k051960->k051937_r(offset - 0x3800);
		else if (offset < 0x3c00)
			return m_k052109->read(offset);
		else
			return m_k051960->k051960_r(offset - 0x3c00);
	}
	else
		return m_k052109->read(offset);
}

void bottom9_state::k052109_051960_w(offs_t offset, uint8_t data)
{
	if (offset >= 0x3800 && offset < 0x3808)
		m_k051960->k051937_w(offset - 0x3800, data);
	else if (offset < 0x3c00)
		m_k052109->write(offset, data);
	else
		m_k051960->k051960_w(offset - 0x3c00, data);
}

uint8_t bottom9_state::bottom9_bankedram1_r(offs_t offset)
{
	if (m_k052109_selected)
		return k052109_051960_r(offset);
	else
	{
		if (m_zoomreadroms)
			return m_k051316->rom_r(offset);
		else
			return m_k051316->read(offset);
	}
}

void bottom9_state::bottom9_bankedram1_w(offs_t offset, uint8_t data)
{
	if (m_k052109_selected)
		k052109_051960_w(offset, data);
	else
		m_k051316->write(offset, data);
}

uint8_t bottom9_state::bottom9_bankedram2_r(offs_t offset)
{
	if (m_k052109_selected)
		return k052109_051960_r(offset + 0x2000);
	else
		return m_palette->basemem().read8(offset);
}

void bottom9_state::bottom9_bankedram2_w(offs_t offset, uint8_t data)
{
	if (m_k052109_selected)
		k052109_051960_w(offset + 0x2000, data);
	else
		m_palette->write8(offset, data);
}

void bottom9_state::bankswitch_w(uint8_t data)
{
	int bank;

	/* bit 0 = RAM bank */
	if ((data & 1) == 0)
		popmessage("bankswitch RAM bank 0");

	/* bit 1-4 = ROM bank */
	if (data & 0x10)
		bank = 8 + ((data & 0x06) >> 1);
	else
		bank = ((data & 0x0e) >> 1);

	membank("bank1")->set_entry(bank);
}

void bottom9_state::bottom9_1f90_w(uint8_t data)
{
	/* bits 0/1 = coin counters */
	machine().bookkeeping().coin_counter_w(0, data & 0x01);
	machine().bookkeeping().coin_counter_w(1, data & 0x02);

	/* bit 2 = enable char ROM reading through the video RAM */
	m_k052109->set_rmrd_line((data & 0x04) ? ASSERT_LINE : CLEAR_LINE);

	/* bit 3 = disable video */
	m_video_enable = ~data & 0x08;

	/* bit 4 = enable 051316 ROM reading */
	m_zoomreadroms = data & 0x10;

	/* bit 5 = RAM bank */
	m_k052109_selected = data & 0x20;
}

void bottom9_state::bottom9_sh_irqtrigger_w(uint8_t data)
{
	m_audiocpu->set_input_line_and_vector(0, HOLD_LINE, 0xff); // Z80
}

INTERRUPT_GEN_MEMBER(bottom9_state::bottom9_sound_interrupt)
{
	if (m_nmienable)
		device.execute().pulse_input_line(INPUT_LINE_NMI, attotime::zero);
}

void bottom9_state::nmi_enable_w(uint8_t data)
{
	m_nmienable = data;
}

void bottom9_state::sound_bank_w(uint8_t data)
{
	int bank_A, bank_B;

	bank_A = ((data >> 0) & 0x03);
	bank_B = ((data >> 2) & 0x03);
	m_k007232_1->set_bank(bank_A, bank_B);

	bank_A = ((data >> 4) & 0x03);
	bank_B = ((data >> 6) & 0x03);
	m_k007232_2->set_bank(bank_A, bank_B);
}


void bottom9_state::main_map(address_map &map)
{
	map(0x0000, 0x3fff).rw(FUNC(bottom9_state::k052109_051960_r), FUNC(bottom9_state::k052109_051960_w));
	map(0x0000, 0x07ff).rw(FUNC(bottom9_state::bottom9_bankedram1_r), FUNC(bottom9_state::bottom9_bankedram1_w));
	map(0x1f80, 0x1f80).w(FUNC(bottom9_state::bankswitch_w));
	map(0x1f90, 0x1f90).w(FUNC(bottom9_state::bottom9_1f90_w));
	map(0x1fa0, 0x1fa0).w("watchdog", FUNC(watchdog_timer_device::reset_w));
	map(0x1fb0, 0x1fb0).w("soundlatch", FUNC(generic_latch_8_device::write));
	map(0x1fc0, 0x1fc0).w(FUNC(bottom9_state::bottom9_sh_irqtrigger_w));
	map(0x1fd0, 0x1fd0).portr("SYSTEM");
	map(0x1fd1, 0x1fd1).portr("P1");
	map(0x1fd2, 0x1fd2).portr("P2");
	map(0x1fd3, 0x1fd3).portr("DSW1");
	map(0x1fe0, 0x1fe0).portr("DSW2");
	map(0x1ff0, 0x1fff).w(m_k051316, FUNC(k051316_device::ctrl_w));
	map(0x2000, 0x27ff).rw(FUNC(bottom9_state::bottom9_bankedram2_r), FUNC(bottom9_state::bottom9_bankedram2_w)).share("palette");
	map(0x4000, 0x5fff).ram();
	map(0x6000, 0x7fff).bankr("bank1");
	map(0x8000, 0xffff).rom();
}

void bottom9_state::audio_map(address_map &map)
{
	map(0x0000, 0x7fff).rom();
	map(0x8000, 0x87ff).ram();
	map(0x9000, 0x9000).w(FUNC(bottom9_state::sound_bank_w));
	map(0xa000, 0xa00d).rw(m_k007232_1, FUNC(k007232_device::read), FUNC(k007232_device::write));
	map(0xb000, 0xb00d).rw(m_k007232_2, FUNC(k007232_device::read), FUNC(k007232_device::write));
	map(0xd000, 0xd000).r("soundlatch", FUNC(generic_latch_8_device::read));
	map(0xf000, 0xf000).w(FUNC(bottom9_state::nmi_enable_w));
}


static INPUT_PORTS_START( bottom9 )
	PORT_START("P1")
	KONAMI8_ALT_B12(1)

	PORT_START("P2")
	KONAMI8_ALT_B12(2)

	PORT_START("DSW1")
	KONAMI_COINAGE_ALT_LOC(SW1)

	PORT_START("DSW2")
	PORT_DIPNAME( 0x07, 0x04, "Play Time" ) PORT_DIPLOCATION("SW2:1,2,3")
	PORT_DIPSETTING(    0x07, "1'00" )
	PORT_DIPSETTING(    0x06, "1'10" )
	PORT_DIPSETTING(    0x05, "1'20" )
	PORT_DIPSETTING(    0x04, "1'30" )
	PORT_DIPSETTING(    0x03, "1'40" )
	PORT_DIPSETTING(    0x02, "1'50" )
	PORT_DIPSETTING(    0x01, "2'00" )
	PORT_DIPSETTING(    0x00, "2'10" )
	PORT_DIPNAME( 0x18, 0x08, "Bonus Time" ) PORT_DIPLOCATION("SW2:4,5")
	PORT_DIPSETTING(    0x18, "00" )
	PORT_DIPSETTING(    0x10, "20" )
	PORT_DIPSETTING(    0x08, "30" )
	PORT_DIPSETTING(    0x00, "40" )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) ) PORT_DIPLOCATION("SW2:6,7")
	PORT_DIPSETTING(    0x60, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Difficult ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Very_Difficult ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) ) PORT_DIPLOCATION("SW2:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START("SYSTEM")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) ) PORT_DIPLOCATION("SW3:1")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPUNUSED_DIPLOC( 0x20, 0x20, "SW3:2" )    /* According to manual: N/U */
	PORT_SERVICE_DIPLOC( 0x40, IP_ACTIVE_LOW, "SW3:3" )
	PORT_DIPNAME( 0x80, 0x80, "Fielder Control" ) PORT_DIPLOCATION("SW3:4")
	PORT_DIPSETTING(    0x80, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x00, "Auto" )
INPUT_PORTS_END

static INPUT_PORTS_START( mstadium )
	PORT_INCLUDE( bottom9 )

	PORT_MODIFY("P1")
	KONAMI8_ALT_B123(1)

	PORT_MODIFY("P2")
	KONAMI8_ALT_B123(2)

	PORT_MODIFY("DSW2")
	PORT_DIPNAME( 0x03, 0x01, "Play Inning" ) PORT_DIPLOCATION("SW2:1,2")
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x0c, 0x08, "Play Inning Time" ) PORT_DIPLOCATION("SW2:3,4")
	PORT_DIPSETTING(    0x0c, "6 Min" )
	PORT_DIPSETTING(    0x08, "8 Min" )
	PORT_DIPSETTING(    0x04, "10 Min" )
	PORT_DIPSETTING(    0x00, "12 Min" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW2:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END



void bottom9_state::volume_callback0(uint8_t data)
{
	m_k007232_1->set_volume(0, (data >> 4) * 0x11, 0);
	m_k007232_1->set_volume(1, 0, (data & 0x0f) * 0x11);
}

void bottom9_state::volume_callback1(uint8_t data)
{
	m_k007232_2->set_volume(0, (data >> 4) * 0x11, 0);
	m_k007232_2->set_volume(1, 0, (data & 0x0f) * 0x11);
}

void bottom9_state::machine_start()
{
	uint8_t *ROM = memregion("maincpu")->base();

	membank("bank1")->configure_entries(0, 12, &ROM[0x10000], 0x2000);

	save_item(NAME(m_video_enable));
	save_item(NAME(m_zoomreadroms));
	save_item(NAME(m_k052109_selected));
	save_item(NAME(m_nmienable));
}

void bottom9_state::machine_reset()
{
	m_video_enable = 0;
	m_zoomreadroms = 0;
	m_k052109_selected = 0;
	m_nmienable = 0;
}

void bottom9_state::bottom9(machine_config &config)
{
	/* basic machine hardware */
	HD6309E(config, m_maincpu, XTAL(24'000'000) / 8); // 63C09E
	m_maincpu->set_addrmap(AS_PROGRAM, &bottom9_state::main_map);

	Z80(config, m_audiocpu, XTAL(3'579'545));
	m_audiocpu->set_addrmap(AS_PROGRAM, &bottom9_state::audio_map);
	m_audiocpu->set_periodic_int(FUNC(bottom9_state::bottom9_sound_interrupt), attotime::from_hz(8*60));  /* irq is triggered by the main CPU */

	WATCHDOG_TIMER(config, "watchdog");

	/* video hardware */
	screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_RASTER));
	screen.set_refresh_hz(60);
	screen.set_vblank_time(subseconds::from_usec(0));
	screen.set_size(64*8, 32*8);
	screen.set_visarea(14*8, (64-14)*8-1, 2*8, 30*8-1);
	screen.set_screen_update(FUNC(bottom9_state::screen_update_bottom9));
	screen.set_palette(m_palette);

	PALETTE(config, m_palette).set_format(palette_device::xBGR_555, 1024);
	m_palette->enable_shadows();

	K052109(config, m_k052109, 0); // 051961 on schematics
	m_k052109->set_palette(m_palette);
	m_k052109->set_screen("screen");
	m_k052109->set_tile_callback(FUNC(bottom9_state::tile_callback));
	m_k052109->irq_handler().set_inputline(m_maincpu, M6809_IRQ_LINE);

	K051960(config, m_k051960, 0);
	m_k051960->set_palette(m_palette);
	m_k051960->set_screen("screen");
	m_k051960->set_sprite_callback(FUNC(bottom9_state::sprite_callback));

	K051316(config, m_k051316, 0);
	m_k051316->set_palette(m_palette);
	m_k051316->set_zoom_callback(FUNC(bottom9_state::zoom_callback));

	/* sound hardware */
	SPEAKER(config, "mono").front_center();

	GENERIC_LATCH_8(config, "soundlatch");

	K007232(config, m_k007232_1, XTAL(3'579'545));
	m_k007232_1->port_write().set(FUNC(bottom9_state::volume_callback0));
	m_k007232_1->add_route(0, "mono", 0.40);
	m_k007232_1->add_route(1, "mono", 0.40);

	K007232(config, m_k007232_2, XTAL(3'579'545));
	m_k007232_2->port_write().set(FUNC(bottom9_state::volume_callback1));
	m_k007232_2->add_route(0, "mono", 0.40);
	m_k007232_2->add_route(1, "mono", 0.40);
}


/***************************************************************************

  Game ROMs

***************************************************************************/

ROM_START( bottom9 )
	ROM_REGION( 0x28000, "maincpu", 0 ) /* code + banked roms */
	ROM_LOAD( "891n03.k17",   0x10000, 0x10000, CRC(8b083ff3) SHA1(045fef944b192e4bb147fa0f28680c0602af7377) )
	ROM_LOAD( "891-t02.k15",  0x20000, 0x08000, CRC(2c10ced2) SHA1(ecd43825a67b495cade94a454c96a19143d87760) )
	ROM_CONTINUE(             0x08000, 0x08000 )

	ROM_REGION( 0x10000, "audiocpu", 0 ) /* Z80 code */
	ROM_LOAD( "891j01.g8",    0x0000, 0x8000, CRC(31b0a0a8) SHA1(8e047f81c19f25de97fa22e70dcfe9e06bfae699) )

	ROM_REGION( 0x080000, "k052109", 0 )    /* tiles */
	ROM_LOAD32_BYTE( "891e10c", 0x00000, 0x10000, CRC(209b0431) SHA1(07f05f63267d5ed5c99b5f786bb66a87045db9e1) )
	ROM_LOAD32_BYTE( "891e10a", 0x00001, 0x10000, CRC(8020a9e8) SHA1(3792794a1b875506089da63cae955668cc61f54b) )
	ROM_LOAD32_BYTE( "891e09c", 0x00002, 0x10000, CRC(9dcaefbf) SHA1(8b61b1627737b959158aa6c7ea5db63f6aec7436) )
	ROM_LOAD32_BYTE( "891e09a", 0x00003, 0x10000, CRC(56b0ead9) SHA1(ef4b00ed0de93f61f4c8661ec0e6049c51a25cf6) )
	ROM_LOAD32_BYTE( "891e10d", 0x40000, 0x10000, CRC(16d5fd7a) SHA1(895a53e41173a70c48337d812466857676908a23) )
	ROM_LOAD32_BYTE( "891e10b", 0x40001, 0x10000, CRC(30121cc0) SHA1(79174d00b79855c00c9c872b8f32946be1bf1d8a) )
	ROM_LOAD32_BYTE( "891e09d", 0x40002, 0x10000, CRC(4e1335e6) SHA1(b892ab40a41978a89658ea2e7aabe9b073430b5d) )
	ROM_LOAD32_BYTE( "891e09b", 0x40003, 0x10000, CRC(b6f914fb) SHA1(e95f3e899c2ead15ef8a529dbc67e8f4a0f88bdd) )

	ROM_REGION( 0x100000, "k051960", 0 ) /* graphics ( don't dispose as the program can read them, 0 ) */
	ROM_LOAD32_BYTE( "891e06e", 0x00000, 0x10000, CRC(0b04db1c) SHA1(0beae7bb8da49379915c0253ce03091eb71a58b5) )    /* sprites */
	ROM_LOAD32_BYTE( "891e06a", 0x00001, 0x10000, CRC(5ee37327) SHA1(f63ddaf63af06ea5421b0361315940582ef57922) )
	ROM_LOAD32_BYTE( "891e05e", 0x00002, 0x10000, CRC(b356e729) SHA1(2cda591415b0f139fdb1f80c349d432bb0579d8e) )
	ROM_LOAD32_BYTE( "891e05a", 0x00003, 0x10000, CRC(bfd5487e) SHA1(24e0de9f12f6df6bde6268d090fe9e1ea827c0dc) )
	ROM_LOAD32_BYTE( "891e06f", 0x40000, 0x10000, CRC(f9ada524) SHA1(2df1fe91f43b95bb4e4a24a0931ab6f540496f65) )
	ROM_LOAD32_BYTE( "891e06b", 0x40001, 0x10000, CRC(2295dfaa) SHA1(96070e1bd07b33b6701e45ee1e200f24532e8630) )
	ROM_LOAD32_BYTE( "891e05f", 0x40002, 0x10000, CRC(ecdd11c5) SHA1(8eac76b3b0f2ab4d59491e10070a62fd9f1eba81) )
	ROM_LOAD32_BYTE( "891e05b", 0x40003, 0x10000, CRC(aba18d24) SHA1(ba8e1fab9537199ece2af26bb3f5c8d85d5213d4) )
	ROM_LOAD32_BYTE( "891e06g", 0x80000, 0x10000, CRC(04abf78f) SHA1(9a21cc71993c3074a8a61c654b998466503b31ef) )
	ROM_LOAD32_BYTE( "891e06c", 0x80001, 0x10000, CRC(dbdb0d55) SHA1(8269b9be8f36116eb6d10efbb6b7050846a9290c) )
	ROM_LOAD32_BYTE( "891e05g", 0x80002, 0x10000, CRC(c315f9ae) SHA1(8e2c8ca1c6dcfe5b7302ea89275b231ffb2e0e84) )
	ROM_LOAD32_BYTE( "891e05c", 0x80003, 0x10000, CRC(21fcbc6f) SHA1(efc65973ea7702a1b5c26a966f452804ad97dbd4) )
	ROM_LOAD32_BYTE( "891e06h", 0xc0000, 0x10000, CRC(5d5ded8c) SHA1(2581aa387c1ba1f2b7c59bae2c59fbf127aa4e86) )
	ROM_LOAD32_BYTE( "891e06d", 0xc0001, 0x10000, CRC(f9ecbd71) SHA1(45e28a8b40159fd0cdcc8ad253ffc7eba6cf3535) )
	ROM_LOAD32_BYTE( "891e05h", 0xc0002, 0x10000, CRC(b0aba53b) SHA1(e76b345ae354533959ed06217b91ce3c93b22a23) )
	ROM_LOAD32_BYTE( "891e05d", 0xc0003, 0x10000, CRC(f6d3f886) SHA1(b8bdcc9470aa93849b8c8a1f03971281cacc6d44) )

	ROM_REGION( 0x020000, "k051316", 0 )
	ROM_LOAD( "891e07a",      0x00000, 0x10000, CRC(b8d8b939) SHA1(ee91fb46d70db2d17f5909c4ea7ee1cf2d317d10) )  /* zoom/rotate */
	ROM_LOAD( "891e07b",      0x10000, 0x10000, CRC(83b2f92d) SHA1(c4972018e1f8109656784fae3e023a5522622c4b) )

	ROM_REGION( 0x0200, "proms", 0 )
	ROM_LOAD( "891b11.f23",   0x0000, 0x0100, CRC(ecb854aa) SHA1(3bd321ca3076d4e0042e0af656d51909fa6a5b3b) )    /* priority encoder (not used) */

	ROM_REGION( 0x40000, "k007232_1", 0 ) /* samples for 007232 #0 */
	ROM_LOAD( "891e08a",      0x00000, 0x10000, CRC(cef667bf) SHA1(e773fc0ced45e01e13cdee18c404d609356d2d0e) )
	ROM_LOAD( "891e08b",      0x10000, 0x10000, CRC(f7c14a7a) SHA1(05261a065de33e158e8d72d74eb657035abb5d03) )
	ROM_LOAD( "891e08c",      0x20000, 0x10000, CRC(756b7f3c) SHA1(6f36f0b4e08db27a8b6e180d12be6427677ad62d) )
	ROM_LOAD( "891e08d",      0x30000, 0x10000, CRC(cd0d7305) SHA1(82403ce1f38014ebf94008a66c98697a572303f9) )

	ROM_REGION( 0x40000, "k007232_2", 0 ) /* samples for 007232 #1 */
	ROM_LOAD( "891e04a",      0x00000, 0x10000, CRC(daebbc74) SHA1(f61daebf80e5e4640c4cea4ea5767e64a49d928d) )
	ROM_LOAD( "891e04b",      0x10000, 0x10000, CRC(5ffb9ad1) SHA1(e8f00c63dc3091aa344e82dc29f41aedd5a764b4) )
	ROM_LOAD( "891e04c",      0x20000, 0x10000, CRC(2dbbf16b) SHA1(84b2005a1fe61a6a0cf1aa6e0fdf7ff8b1f8f82a) )
	ROM_LOAD( "891e04d",      0x30000, 0x10000, CRC(8b0cd2cc) SHA1(e14109c69fa24d309aed4ff3589cc6619e29f97f) )
ROM_END

ROM_START( bottom9n )
	ROM_REGION( 0x28000, "maincpu", 0 ) /* code + banked roms */
	ROM_LOAD( "891n03.k17",   0x10000, 0x10000, CRC(8b083ff3) SHA1(045fef944b192e4bb147fa0f28680c0602af7377) )
	ROM_LOAD( "891n02.k15",   0x20000, 0x08000, CRC(d44d9ed4) SHA1(2a12bcfba81ab7e074569e2ad2da6a237a1c0ce5) )
	ROM_CONTINUE(             0x08000, 0x08000 )

	ROM_REGION( 0x10000, "audiocpu", 0 ) /* Z80 code */
	ROM_LOAD( "891j01.g8",    0x0000, 0x8000, CRC(31b0a0a8) SHA1(8e047f81c19f25de97fa22e70dcfe9e06bfae699) )

	ROM_REGION( 0x080000, "k052109", 0 )    /* tiles */
	ROM_LOAD32_BYTE( "891e10c", 0x00000, 0x10000, CRC(209b0431) SHA1(07f05f63267d5ed5c99b5f786bb66a87045db9e1) )
	ROM_LOAD32_BYTE( "891e10a", 0x00001, 0x10000, CRC(8020a9e8) SHA1(3792794a1b875506089da63cae955668cc61f54b) )
	ROM_LOAD32_BYTE( "891e09c", 0x00002, 0x10000, CRC(9dcaefbf) SHA1(8b61b1627737b959158aa6c7ea5db63f6aec7436) )
	ROM_LOAD32_BYTE( "891e09a", 0x00003, 0x10000, CRC(56b0ead9) SHA1(ef4b00ed0de93f61f4c8661ec0e6049c51a25cf6) )
	ROM_LOAD32_BYTE( "891e10d", 0x40000, 0x10000, CRC(16d5fd7a) SHA1(895a53e41173a70c48337d812466857676908a23) )
	ROM_LOAD32_BYTE( "891e10b", 0x40001, 0x10000, CRC(30121cc0) SHA1(79174d00b79855c00c9c872b8f32946be1bf1d8a) )
	ROM_LOAD32_BYTE( "891e09d", 0x40002, 0x10000, CRC(4e1335e6) SHA1(b892ab40a41978a89658ea2e7aabe9b073430b5d) )
	ROM_LOAD32_BYTE( "891e09b", 0x40003, 0x10000, CRC(b6f914fb) SHA1(e95f3e899c2ead15ef8a529dbc67e8f4a0f88bdd) )

	ROM_REGION( 0x100000, "k051960", 0 ) /* graphics ( don't dispose as the program can read them, 0 ) */
	ROM_LOAD32_BYTE( "891e06e", 0x00000, 0x10000, CRC(0b04db1c) SHA1(0beae7bb8da49379915c0253ce03091eb71a58b5) )    /* sprites */
	ROM_LOAD32_BYTE( "891e06a", 0x00001, 0x10000, CRC(5ee37327) SHA1(f63ddaf63af06ea5421b0361315940582ef57922) )
	ROM_LOAD32_BYTE( "891e05e", 0x00002, 0x10000, CRC(b356e729) SHA1(2cda591415b0f139fdb1f80c349d432bb0579d8e) )
	ROM_LOAD32_BYTE( "891e05a", 0x00003, 0x10000, CRC(bfd5487e) SHA1(24e0de9f12f6df6bde6268d090fe9e1ea827c0dc) )
	ROM_LOAD32_BYTE( "891e06f", 0x40000, 0x10000, CRC(f9ada524) SHA1(2df1fe91f43b95bb4e4a24a0931ab6f540496f65) )
	ROM_LOAD32_BYTE( "891e06b", 0x40001, 0x10000, CRC(2295dfaa) SHA1(96070e1bd07b33b6701e45ee1e200f24532e8630) )
	ROM_LOAD32_BYTE( "891e05f", 0x40002, 0x10000, CRC(ecdd11c5) SHA1(8eac76b3b0f2ab4d59491e10070a62fd9f1eba81) )
	ROM_LOAD32_BYTE( "891e05b", 0x40003, 0x10000, CRC(aba18d24) SHA1(ba8e1fab9537199ece2af26bb3f5c8d85d5213d4) )
	ROM_LOAD32_BYTE( "891e06g", 0x80000, 0x10000, CRC(04abf78f) SHA1(9a21cc71993c3074a8a61c654b998466503b31ef) )
	ROM_LOAD32_BYTE( "891e06c", 0x80001, 0x10000, CRC(dbdb0d55) SHA1(8269b9be8f36116eb6d10efbb6b7050846a9290c) )
	ROM_LOAD32_BYTE( "891e05g", 0x80002, 0x10000, CRC(c315f9ae) SHA1(8e2c8ca1c6dcfe5b7302ea89275b231ffb2e0e84) )
	ROM_LOAD32_BYTE( "891e05c", 0x80003, 0x10000, CRC(21fcbc6f) SHA1(efc65973ea7702a1b5c26a966f452804ad97dbd4) )
	ROM_LOAD32_BYTE( "891e06h", 0xc0000, 0x10000, CRC(5d5ded8c) SHA1(2581aa387c1ba1f2b7c59bae2c59fbf127aa4e86) )
	ROM_LOAD32_BYTE( "891e06d", 0xc0001, 0x10000, CRC(f9ecbd71) SHA1(45e28a8b40159fd0cdcc8ad253ffc7eba6cf3535) )
	ROM_LOAD32_BYTE( "891e05h", 0xc0002, 0x10000, CRC(b0aba53b) SHA1(e76b345ae354533959ed06217b91ce3c93b22a23) )
	ROM_LOAD32_BYTE( "891e05d", 0xc0003, 0x10000, CRC(f6d3f886) SHA1(b8bdcc9470aa93849b8c8a1f03971281cacc6d44) )


	ROM_REGION( 0x020000, "k051316", 0 )
	ROM_LOAD( "891e07a",      0x00000, 0x10000, CRC(b8d8b939) SHA1(ee91fb46d70db2d17f5909c4ea7ee1cf2d317d10) )  /* zoom/rotate */
	ROM_LOAD( "891e07b",      0x10000, 0x10000, CRC(83b2f92d) SHA1(c4972018e1f8109656784fae3e023a5522622c4b) )

	ROM_REGION( 0x0200, "proms", 0 )
	ROM_LOAD( "891b11.f23",   0x0000, 0x0100, CRC(ecb854aa) SHA1(3bd321ca3076d4e0042e0af656d51909fa6a5b3b) )    /* priority encoder (not used) */

	ROM_REGION( 0x40000, "k007232_1", 0 ) /* samples for 007232 #0 */
	ROM_LOAD( "891e08a",      0x00000, 0x10000, CRC(cef667bf) SHA1(e773fc0ced45e01e13cdee18c404d609356d2d0e) )
	ROM_LOAD( "891e08b",      0x10000, 0x10000, CRC(f7c14a7a) SHA1(05261a065de33e158e8d72d74eb657035abb5d03) )
	ROM_LOAD( "891e08c",      0x20000, 0x10000, CRC(756b7f3c) SHA1(6f36f0b4e08db27a8b6e180d12be6427677ad62d) )
	ROM_LOAD( "891e08d",      0x30000, 0x10000, CRC(cd0d7305) SHA1(82403ce1f38014ebf94008a66c98697a572303f9) )

	ROM_REGION( 0x40000, "k007232_2", 0 ) /* samples for 007232 #1 */
	ROM_LOAD( "891e04a",      0x00000, 0x10000, CRC(daebbc74) SHA1(f61daebf80e5e4640c4cea4ea5767e64a49d928d) )
	ROM_LOAD( "891e04b",      0x10000, 0x10000, CRC(5ffb9ad1) SHA1(e8f00c63dc3091aa344e82dc29f41aedd5a764b4) )
	ROM_LOAD( "891e04c",      0x20000, 0x10000, CRC(2dbbf16b) SHA1(84b2005a1fe61a6a0cf1aa6e0fdf7ff8b1f8f82a) )
	ROM_LOAD( "891e04d",      0x30000, 0x10000, CRC(8b0cd2cc) SHA1(e14109c69fa24d309aed4ff3589cc6619e29f97f) )
ROM_END

ROM_START( mstadium )
	ROM_REGION( 0x28000, "maincpu", 0 ) /* code + banked roms */
	ROM_LOAD( "891-403.k17",   0x10000, 0x10000, CRC(1c00c4e8) SHA1(8a3400a8df44f21616422e5af3bca84d0f390f63) )
	ROM_LOAD( "891-402.k15",   0x20000, 0x08000, CRC(b850bbce) SHA1(a64300d1b1068e59eb59c427946c9bff164e2da8) )
	ROM_CONTINUE(             0x08000, 0x08000 )

	ROM_REGION( 0x10000, "audiocpu", 0 ) /* Z80 code */
	ROM_LOAD( "891w01.g8",    0x0000, 0x8000, CRC(edec565a) SHA1(69cba0d00c6ef76c4ce2b553e3fd15de8abbbf31) )

	ROM_REGION( 0x080000, "k052109", 0 )    /* tiles */
	ROM_LOAD32_BYTE( "891e10c", 0x00000, 0x10000, CRC(209b0431) SHA1(07f05f63267d5ed5c99b5f786bb66a87045db9e1) )
	ROM_LOAD32_BYTE( "891e10a", 0x00001, 0x10000, CRC(8020a9e8) SHA1(3792794a1b875506089da63cae955668cc61f54b) )
	ROM_LOAD32_BYTE( "891e09c", 0x00002, 0x10000, CRC(9dcaefbf) SHA1(8b61b1627737b959158aa6c7ea5db63f6aec7436) )
	ROM_LOAD32_BYTE( "891e09a", 0x00003, 0x10000, CRC(56b0ead9) SHA1(ef4b00ed0de93f61f4c8661ec0e6049c51a25cf6) )
	ROM_LOAD32_BYTE( "891e10d", 0x40000, 0x10000, CRC(16d5fd7a) SHA1(895a53e41173a70c48337d812466857676908a23) )
	ROM_LOAD32_BYTE( "891e10b", 0x40001, 0x10000, CRC(30121cc0) SHA1(79174d00b79855c00c9c872b8f32946be1bf1d8a) )
	ROM_LOAD32_BYTE( "891e09d", 0x40002, 0x10000, CRC(4e1335e6) SHA1(b892ab40a41978a89658ea2e7aabe9b073430b5d) )
	ROM_LOAD32_BYTE( "891e09b", 0x40003, 0x10000, CRC(b6f914fb) SHA1(e95f3e899c2ead15ef8a529dbc67e8f4a0f88bdd) )

	ROM_REGION( 0x100000, "k051960", 0 )    /* sprites */
	ROM_LOAD32_BYTE( "891e06e", 0x00000, 0x10000, CRC(0b04db1c) SHA1(0beae7bb8da49379915c0253ce03091eb71a58b5) )
	ROM_LOAD32_BYTE( "891e06a", 0x00001, 0x10000, CRC(5ee37327) SHA1(f63ddaf63af06ea5421b0361315940582ef57922) )
	ROM_LOAD32_BYTE( "891e05e", 0x00002, 0x10000, CRC(b356e729) SHA1(2cda591415b0f139fdb1f80c349d432bb0579d8e) )
	ROM_LOAD32_BYTE( "891e05a", 0x00003, 0x10000, CRC(bfd5487e) SHA1(24e0de9f12f6df6bde6268d090fe9e1ea827c0dc) )
	ROM_LOAD32_BYTE( "891e06f", 0x40000, 0x10000, CRC(f9ada524) SHA1(2df1fe91f43b95bb4e4a24a0931ab6f540496f65) )
	ROM_LOAD32_BYTE( "891e06b", 0x40001, 0x10000, CRC(2295dfaa) SHA1(96070e1bd07b33b6701e45ee1e200f24532e8630) )
	ROM_LOAD32_BYTE( "891e05f", 0x40002, 0x10000, CRC(ecdd11c5) SHA1(8eac76b3b0f2ab4d59491e10070a62fd9f1eba81) )
	ROM_LOAD32_BYTE( "891e05b", 0x40003, 0x10000, CRC(aba18d24) SHA1(ba8e1fab9537199ece2af26bb3f5c8d85d5213d4) )
	ROM_LOAD32_BYTE( "891e06g", 0x80000, 0x10000, CRC(04abf78f) SHA1(9a21cc71993c3074a8a61c654b998466503b31ef) )
	ROM_LOAD32_BYTE( "891e06c", 0x80001, 0x10000, CRC(dbdb0d55) SHA1(8269b9be8f36116eb6d10efbb6b7050846a9290c) )
	ROM_LOAD32_BYTE( "891e05g", 0x80002, 0x10000, CRC(c315f9ae) SHA1(8e2c8ca1c6dcfe5b7302ea89275b231ffb2e0e84) )
	ROM_LOAD32_BYTE( "891e05c", 0x80003, 0x10000, CRC(21fcbc6f) SHA1(efc65973ea7702a1b5c26a966f452804ad97dbd4) )
	ROM_LOAD32_BYTE( "891e06h", 0xc0000, 0x10000, CRC(5d5ded8c) SHA1(2581aa387c1ba1f2b7c59bae2c59fbf127aa4e86) )
	ROM_LOAD32_BYTE( "891e06d", 0xc0001, 0x10000, CRC(f9ecbd71) SHA1(45e28a8b40159fd0cdcc8ad253ffc7eba6cf3535) )
	ROM_LOAD32_BYTE( "891e05h", 0xc0002, 0x10000, CRC(b0aba53b) SHA1(e76b345ae354533959ed06217b91ce3c93b22a23) )
	ROM_LOAD32_BYTE( "891e05d", 0xc0003, 0x10000, CRC(f6d3f886) SHA1(b8bdcc9470aa93849b8c8a1f03971281cacc6d44) )

	ROM_REGION( 0x020000, "k051316", 0 )
	ROM_LOAD( "891e07a",      0x00000, 0x10000, CRC(b8d8b939) SHA1(ee91fb46d70db2d17f5909c4ea7ee1cf2d317d10) )  /* zoom/rotate */
	ROM_LOAD( "891e07b",      0x10000, 0x10000, CRC(83b2f92d) SHA1(c4972018e1f8109656784fae3e023a5522622c4b) )

	ROM_REGION( 0x0200, "proms", 0 )
	ROM_LOAD( "891b11.f23",   0x0000, 0x0100, CRC(ecb854aa) SHA1(3bd321ca3076d4e0042e0af656d51909fa6a5b3b) )    /* priority encoder (not used) */

	ROM_REGION( 0x40000, "k007232_1", 0 ) /* samples for 007232 #0 */
	ROM_LOAD( "891e08a",      0x00000, 0x10000, CRC(cef667bf) SHA1(e773fc0ced45e01e13cdee18c404d609356d2d0e) )
	ROM_LOAD( "891e08b",      0x10000, 0x10000, CRC(f7c14a7a) SHA1(05261a065de33e158e8d72d74eb657035abb5d03) )
	ROM_LOAD( "891e08c",      0x20000, 0x10000, CRC(756b7f3c) SHA1(6f36f0b4e08db27a8b6e180d12be6427677ad62d) )
	ROM_LOAD( "891e08d",      0x30000, 0x10000, CRC(cd0d7305) SHA1(82403ce1f38014ebf94008a66c98697a572303f9) )

	ROM_REGION( 0x40000, "k007232_2", 0 ) /* samples for 007232 #1 */
	ROM_LOAD( "891e04a",      0x00000, 0x10000, CRC(daebbc74) SHA1(f61daebf80e5e4640c4cea4ea5767e64a49d928d) )
	ROM_LOAD( "891e04b",      0x10000, 0x10000, CRC(5ffb9ad1) SHA1(e8f00c63dc3091aa344e82dc29f41aedd5a764b4) )
	ROM_LOAD( "891e04c",      0x20000, 0x10000, CRC(2dbbf16b) SHA1(84b2005a1fe61a6a0cf1aa6e0fdf7ff8b1f8f82a) )
	ROM_LOAD( "891e04d",      0x30000, 0x10000, CRC(8b0cd2cc) SHA1(e14109c69fa24d309aed4ff3589cc6619e29f97f) )
ROM_END



GAME( 1989, bottom9,  0,       bottom9, bottom9,  bottom9_state, empty_init, ROT0, "Konami", "Bottom of the Ninth (version T)", MACHINE_SUPPORTS_SAVE )
GAME( 1989, bottom9n, bottom9, bottom9, bottom9,  bottom9_state, empty_init, ROT0, "Konami", "Bottom of the Ninth (version N)", MACHINE_SUPPORTS_SAVE )
GAME( 1989, mstadium, bottom9, bottom9, mstadium, bottom9_state, empty_init, ROT0, "Konami", "Main Stadium (Japan)", MACHINE_SUPPORTS_SAVE )
