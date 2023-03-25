/* SPDX-License-Identifier: MIT */
#pragma once

#include <infos/drivers/device.h>
#include <infos/kernel/log.h>
#include <infos/util/lock.h>

namespace infos
{
	namespace kernel
	{
		class IRQ;
	}
	
	namespace drivers
	{
		namespace ata
		{
			struct ATAControllerConfiguration
			{
				uint32_t BAR[5];
			};
			
			class ATADevice;
			class ATAController : public Device
			{
				friend class ATADevice;
				
			public:
				static DeviceClass ATAControllerDeviceClass;
				const DeviceClass& device_class() const override { return ATAControllerDeviceClass; }

				ATAController(kernel::IRQ& irq, const ATAControllerConfiguration& cfg);
				
				bool init(kernel::DeviceManager& dm) override;
				
			private:
				kernel::IRQ& _irq;
				util::Mutex _mtx[2];
				
				uint8_t ata_read(int channel, int reg);
				void ata_read_buffer(int channel, int reg, void *buffer, size_t size);
				void ata_write(int channel, int reg, uint8_t data);
				int ata_poll(int channel, bool error_check=false);
				
				bool probe_channel(kernel::DeviceManager& dm, int channel);
				bool probe_device(kernel::DeviceManager& dm, int channel, int device);
				
				/**
				 * As written in wiki.osdev.org/PCI_IDE_Controller : 
				 * There are 4 Serial IDE Ports... Each pair of ports form one channel. 
				 * 
				 * Hence 2 channels, I assume? 
				 */
				struct ChannelRegisters {
					/**
					 * @brief
					 * I/O base -- 8 offset ports
					 */
					uint16_t base;
					/**
					 * @brief
					 * Control base -- 4 offset ports BUT only 1 used (this is not your concern)
					 */
					uint16_t ctrl;
					/**
					 * @brief
					 * Bus Master IDE -- 8 offset ports for controlling this channel's bus master 
					 * IDE. 
					 */
					uint16_t bmide;
					/**
					 * @brief 
					 * No Interrupt (ENable?) -- i.e., whether IRQ should NOT be triggered. 
					 */
					uint8_t nIEN;
				} channels[2];
			};
			
			extern kernel::ComponentLog ata_log;
		}
	}
}

/* [Status Port]
 * Bitmask referring to status of a channel at read time. 
 */
#define ATA_SR_BSY     0x80    // Busy
#define ATA_SR_DRDY    0x40    // Drive ready
#define ATA_SR_DF      0x20    // Drive write fault
#define ATA_SR_DSC     0x10    // Drive seek complete
#define ATA_SR_DRQ     0x08    // Data request ready
#define ATA_SR_CORR    0x04    // Corrected data
#define ATA_SR_IDX     0x02    // Inlex
#define ATA_SR_ERR     0x01    // Error

/* [Error Port]
 * Bitmask referring to most recent error at read time. 
 */
#define ATA_ER_BBK      0x80    // Bad sector
#define ATA_ER_UNC      0x40    // Uncorrectable data
#define ATA_ER_MC       0x20    // No media
#define ATA_ER_IDNF     0x10    // ID mark not found
#define ATA_ER_MCR      0x08    // No media
#define ATA_ER_ABRT     0x04    // Command aborted
#define ATA_ER_TK0NF    0x02    // Track 0 not found
#define ATA_ER_AMNF     0x01    // No address mark

/* [Command Port]
 * Opcode referring to the operation you wish to perform on the ATA device at this channel. 
 */
#define ATA_CMD_READ_PIO          0x20
#define ATA_CMD_READ_PIO_EXT      0x24
#define ATA_CMD_READ_DMA          0xC8
#define ATA_CMD_READ_DMA_EXT      0x25
#define ATA_CMD_WRITE_PIO         0x30
#define ATA_CMD_WRITE_PIO_EXT     0x34
#define ATA_CMD_WRITE_DMA         0xCA
#define ATA_CMD_WRITE_DMA_EXT     0x35
#define ATA_CMD_CACHE_FLUSH       0xE7
#define ATA_CMD_CACHE_FLUSH_EXT   0xEA
#define ATA_CMD_PACKET            0xA0
/* Has relevant subops */
#define ATA_CMD_IDENTIFY_PACKET   0xA1
#define ATA_CMD_IDENTIFY          0xEC
/* Below, specifically, are for ATAPI devices (e.g., CD/DVD drives) */
#define ATAPI_CMD_READ       0xA8
#define ATAPI_CMD_EJECT      0x1B

/* [CMD -- Identification Subops]
 * Opcodes ATA_CMD_IDENTIFY_PACKET and ATA_CMD_IDENTIFY returns a 512B buffer called the 
 * "identification space". 
 * 
 * Below opcodes are used to perform corresponding reads from this identification space. 
 */
#define ATA_IDENT_DEVICETYPE   0
#define ATA_IDENT_CYLINDERS    2
#define ATA_IDENT_HEADS        6
#define ATA_IDENT_SECTORS      12
#define ATA_IDENT_SERIAL       20
#define ATA_IDENT_MODEL        54
#define ATA_IDENT_CAPABILITIES 98
#define ATA_IDENT_FIELDVALID   106
#define ATA_IDENT_MAX_LBA      120
#define ATA_IDENT_COMMANDSETS  164
#define ATA_IDENT_MAX_LBA_EXT  200

/* [Drive Selection]
 * Use these to tell the ATA controller what kind of drive you are selecting. 
 */
#define IDE_ATA        0x00
#define IDE_ATAPI      0x01

#define ATA_MASTER     0x00
#define ATA_SLAVE      0x01

/* [Channel Registers]
 * Each channel has 13 registers [sic.]. Obv. this should not be the case if below is what's being 
 * defined... 
 */
#define ATA_REG_DATA       0x00 // BAR[0|2] + 0; RW
#define ATA_REG_ERROR      0x01 // BAR[0|2] + 1; R (ref. [Error Port])
#define ATA_REG_FEATURES   0x01 // BAR[0|2] + 1; W (half-duplex)
#define ATA_REG_SECCOUNT0  0x02 // BAR[0|2] + 2; RW
#define ATA_REG_LBA0       0x03 // BAR[0|2] + 3; RW
#define ATA_REG_LBA1       0x04 // BAR[0|2] + 4; RW
#define ATA_REG_LBA2       0x05 // BAR[0|2] + 5; RW
#define ATA_REG_HDDEVSEL   0x06 // BAR[0|2] + 6; RW
#define ATA_REG_COMMAND    0x07 // BAR[0|2] + 7; R (ref. [Command Port])
#define ATA_REG_STATUS     0x07 // BAR[0|2] + 7; W (half-duplex)
#define ATA_REG_SECCOUNT1  0x08 
#define ATA_REG_LBA3       0x09 
#define ATA_REG_LBA4       0x0A
#define ATA_REG_LBA5       0x0B
#define ATA_REG_CONTROL    0x0C // BAR[1|3] + 2; W (half-duplex)
#define ATA_REG_ALTSTATUS  0x0C // BAR[1|3] + 2; R (ref. wiki.osdev.org -- redundancy maybe?)
#define ATA_REG_DEVADDRESS 0x0D // BAR[1|3] + 3

/* [Channels] */
#define ATA_PRIMARY      0x00
#define ATA_SECONDARY    0x01

/* [Directions] */
#define ATA_READ      0x00
#define ATA_WRITE     0x01
