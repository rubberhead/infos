/* SPDX-License-Identifier: MIT */

/*
 * drivers/ata/ata-controller.cpp
 * 
 * InfOS
 * Copyright (C) University of Edinburgh 2016.  All Rights Reserved.
 * 
 * Tom Spink <tspink@inf.ed.ac.uk>
 */
#include <infos/drivers/ata/ata-controller.h>
#include <infos/drivers/ata/ata-device.h>
#include <infos/drivers/ata/page-cache.h>
#include <infos/util/lock.h>
#include <infos/kernel/kernel.h>
#include <arch/x86/pio.h>

using namespace infos::drivers;
using namespace infos::drivers::ata;
using namespace infos::kernel;
using namespace infos::arch::x86;
using namespace infos::util;

ComponentLog infos::drivers::ata::ata_log(syslog, "ata");

DeviceClass ATAController::ATAControllerDeviceClass(Device::RootDeviceClass, "atactl");

#define PORT_OR_BASE_ADDRESS(__v, __p) ((__v == 0) ? (__p) : (__v & ~3))

/**
 * Initializes ATA controller over all 2 channels. 
 * 
 * @param irq Interrupt Requests, no idea how InfOS impled that. 
 * @param cfg BAR0...4. Use 0 if want to use default on wiki.osdev.org
 */
ATAController::ATAController(IRQ& irq, const ATAControllerConfiguration& cfg) : _irq(irq)
{
	/* [BAR0] 
	 * Base address of primary channel (data port?) in PCI native mode 
	 * i.e., start of I/O ports used by primary channel
	 */
	channels[ATA_PRIMARY].base = PORT_OR_BASE_ADDRESS(cfg.BAR[0], 0x1F0); 

	/* [BAR1] 
	 * Base address of primary channel control port in PCI native mode 
	 * i.e., start of I/O ports controlling the primary channel
	 */
	channels[ATA_PRIMARY].ctrl = PORT_OR_BASE_ADDRESS(cfg.BAR[1], 0x3F6);

	/* [BAR2] 
	 * Base address of secondary channel (data port?) in PCI native mode 
	 * i.e., start of I/O ports used by secondary channel
	 */
	channels[ATA_SECONDARY].base = PORT_OR_BASE_ADDRESS(cfg.BAR[2], 0x170);

	/* [BAR3] 
	 * Base address of secondary channel control port in PCI native mode 
	 * i.e., start of I/O ports controlling the secondary channel
	 */
	channels[ATA_SECONDARY].ctrl = PORT_OR_BASE_ADDRESS(cfg.BAR[3], 0x376);

	/* [BAR4]
	 * Bus master {IDE|ATA|PATA}, where 8 allocated for each channel in seq. 
	 * i.e., start of the 8 I/O ports controlling the primary channel's bus master IDE
	 * 
	 * No idea what the & ~3 is useful for... Take this as granted. 
	 */
	channels[ATA_PRIMARY].bmide = (cfg.BAR[4] & ~3);
	// Likewise offset for secondary channel. 
	channels[ATA_SECONDARY].bmide = (cfg.BAR[4] & ~3) + 8;

	// Disable interrupts, apparently -- why 2 tho? 
	channels[ATA_PRIMARY].nIEN = 2;
	channels[ATA_SECONDARY].nIEN = 2;
}

bool ATAController::init(kernel::DeviceManager& dm)
{
	ata_log.messagef(LogLevel::INFO, "Initialising ATA storage device Status=%u", ata_read(0, ATA_REG_STATUS));

	// Disable interrupts
	ata_write(ATA_PRIMARY, ATA_REG_CONTROL, 2);
	ata_write(ATA_SECONDARY, ATA_REG_CONTROL, 2);

	//UniqueLock<IRQLock> l(IRQLock::Instance);

	bool success = true;
	for (int channel = 0; channel < 2; channel++) {
		success &= probe_channel(dm, channel);
	}

	return success;
}

bool ATAController::probe_channel(kernel::DeviceManager& dm, int channel)
{
	bool success = true;
	for (int slave = 0; slave < 2; slave++) {
		success &= probe_device(dm, channel, slave);
	}

	return true;
}

bool ATAController::probe_device(kernel::DeviceManager& dm, int channel, int device)
{
	ata_log.messagef(LogLevel::DEBUG, "Probing device %d:%d", channel, device);

	ata_write(channel, ATA_REG_HDDEVSEL, 0xa0 | (device << 4));
	sys.spin_delay(Milliseconds(1));

	ata_write(channel, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
	sys.spin_delay(Milliseconds(1));

	if (ata_read(channel, ATA_REG_STATUS) == 0) return true;

	while (true) {
		uint8_t status = ata_read(channel, ATA_REG_STATUS);
		if (status & ATA_SR_ERR) {
			ata_log.message(LogLevel::ERROR, "ATA device error");
			return false;
		}

		if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) {
			break;
		}
	}

	ata_log.messagef(LogLevel::INFO, "Found ATA device");

	ATADevice *dev = new ATADevice(*this, channel, device);
	if (!dm.register_device(*dev)) {
		delete dev;
		return false;
	}
	
	return true;
}

uint8_t ATAController::ata_read(int channel, int reg)
{
	/* result stores read byte from block device? */
	uint8_t result = 0;

	/* <1>
	 * If reg is one of:
	 * - ATA_REG_SECCOUNT1 <=> 0x08
	 * - ATA_REG_LBA3	   <=> 0x09
	 * - ATA_REG_LBA4	   <=> 0x0A
	 * - ATA_REG_LBA5      <=> 0x0B
	 * then write to ATA_REG_CONTROL with ((1 << 7) | nIEN) (whatever this means...)
	 */
	if (reg > 0x07 && reg < 0x0C) {
		ata_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
	}

	if (reg < 0x08) {
		/* =>
		 * If reg is in range [0x00, 0x07] i.e., if reg is for data I/O, 
		 * read exactly one byte from port of this reg's pin. 
		 */
		result = __inb(channels[channel].base + reg - 0x00);
	} else if (reg < 0x0C) {
		/* => <1>
		 * Else if reg is in range [0x08, 0x0B] which is if <1> runs, 
		 * read byte from port of this reg's pin with offset (-0d6). 
		 * (equiv. to reg in range [0x02, 0x05] which is SECCOUNT0...LBA2)
		 */
		result = __inb(channels[channel].base + reg - 0x06);
	} else if (reg < 0x0E) {
		/* =>
		 * Else if reg is in range [0x0C, 0x0D] i.e., if reg is for ctrl I/O, 
		 * read byte from port of this reg's pin with offset (-0d10). 
		 * (equiv. to reg in range [0x02, 0x03] which is SECCOUNT0...LBA0)
		 */
		result = __inb(channels[channel].ctrl + reg - 0x0A);
	} else if (reg < 0x16) {
		/* => 
		 * Else if reg is in range [0x0E, 0x15] (out-of-range... where can this happen?), 
		 * read byte from port of this reg's pin with offset (-0d14). 
		 * (equiv. to doing modular arithmetics to pin number)
		 */
		result = __inb(channels[channel].bmide + reg - 0x0E);
	}

	/* 
	 * If reg is in range [0x08, 0x0B] which is if <1> runs, 
	 * then write nIEN (instead of nIEN + (1 << 7)) to ATA_REG_CONTROL again. 
	 */
	if (reg > 0x07 && reg < 0x0C) {
		ata_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
	}

	/* Return pin out byte */
	return result;
}

void ATAController::ata_read_buffer(int channel, int reg, void* buffer, size_t size)
{
	if (reg > 0x07 && reg < 0x0C) {
		ata_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
	}
	
	if (reg < 0x08) {
		__insl(channels[channel].base + reg - 0x00, (uintptr_t)buffer, size >> 2);
	} else if (reg < 0x0C) {
		__insl(channels[channel].base + reg - 0x06, (uintptr_t)buffer, size >> 2);
	} else if (reg < 0x0E) {
		__insl(channels[channel].ctrl + reg - 0x0A, (uintptr_t)buffer, size >> 2);
	} else if (reg < 0x16) {
		__insl(channels[channel].bmide + reg - 0x0E, (uintptr_t)buffer, size >> 2);
	}
	
	if (reg > 0x07 && reg < 0x0C) {
		ata_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
	}
}

void ATAController::ata_write(int channel, int reg, uint8_t data)
{
	if (reg > 0x07 && reg < 0x0C) {
		ata_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
	}

	if (reg < 0x08) {
		__outb(channels[channel].base + reg - 0x00, data);
	} else if (reg < 0x0C) {
		__outb(channels[channel].base + reg - 0x06, data);
	} else if (reg < 0x0E) {
		__outb(channels[channel].ctrl + reg - 0x0A, data);
	} else if (reg < 0x16) {
		__outb(channels[channel].bmide + reg - 0x0E, data);
	}

	if (reg > 0x07 && reg < 0x0C) {
		ata_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
	}
}

int ATAController::ata_poll(int channel, bool error_check)
{
	sys.spin_delay(Nanoseconds(400));
	
	while (ata_read(channel, ATA_REG_STATUS) & ATA_SR_BSY) asm volatile("pause");

	if (error_check) {
		uint8_t status = ata_read(channel, ATA_REG_STATUS);

		if (status & ATA_SR_ERR)
		   return 2;

		if (status & ATA_SR_DF)
		   return 1; // Device Fault.

		if ((status & ATA_SR_DRQ) == 0)
		   return 3;
	}

	return 0; // No Error.
}
