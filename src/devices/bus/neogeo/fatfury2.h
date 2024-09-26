// license:BSD-3-Clause
// copyright-holders:S. Smith,David Haywood,Fabio Priuli
#ifndef MAME_BUS_NEOGEO_FATFURY2_H
#define MAME_BUS_NEOGEO_FATFURY2_H

#pragma once

#include "slot.h"
#include "rom.h"
#include "prot_fatfury2.h"

// ======================> neogeo_fatfury2_cart_device

class neogeo_fatfury2_cart_device : public neogeo_rom_device
{
public:
	// construction/destruction
	neogeo_fatfury2_cart_device(const machine_config &mconfig, const char *tag, device_t *owner, uint16_t clock);

	// reading and writing
	virtual uint16_t protection_r(address_space &space, offs_t offset) override { return m_prot->protection_r(offset); }
	virtual void protection_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0) override { m_prot->protection_w(offset, data); }

protected:
	// device-level overrides
	virtual void device_start() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;

	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;

	required_device<fatfury2_prot_device> m_prot;
};



// device type definition
DECLARE_DEVICE_TYPE(NEOGEO_FATFURY2_CART, neogeo_fatfury2_cart_device)


#endif // MAME_BUS_NEOGEO_FATFURY2_H
