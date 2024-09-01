/*
 * Bootloader_private.h
 *
 *  Created on: Aug 26, 2024
 *      Author: Gobara
 */

#ifndef INC_BOOTLOADER_PRIVATE_H_
#define INC_BOOTLOADER_PRIVATE_H_

/* Define constants for CRC status */
#define CRC_SUCCESS 0x01  // CRC check successful
#define CRC_FAIL   0x02   // CRC check failed

/* Define constants for bootloader acknowledgment responses */
#define Bootloader_ACK    0xA5  // ACK response to the host
#define Bootloader_NACK   0x7F  // NACK response to the host


#define VALID_ADDRESS_STATUS      0x0
#define INVALID_ADDRESS_STATUS    0x1



#define VALID_WRITING_STATUS 	0x0
#define INVALID_WRITING_STATUS  0x1

/* Define the version number of the bootloader */
#define Bootloader_VERSION 0x02  // Bootloader version 2.3


#define FLASH_MASS_ERASE_PATTERN 0xFF


#define DBGMCU_IDCODE_REG     *((volatile uint16_t*)0xE0042000)


#define MAX_NUMBER_OF_PAGES 128


/* Function prototypes for private bootloader functions */

/* Sends an ACK response to the host with a specific reply length */
static void vSendACK(uint8_t copy_u8ReplyLength);

/* Sends a NACK response to the host */
static void vSendNACK(void);

/* Verifies the CRC of the received data array */
static uint8_t u8VerifyCRC(uint8_t* copy_u8DataArr, uint8_t Copy_u8Length, uint32_t copy_u32HostCRC);

static uint8_t u8ValidDataAddress(uint32_t Copy_u32Address) ;

static uint8_t u8ExecuteFlashErease(uint8_t copy_u8PageNumber,
		uint8_t copy_u8NumberOfPages) ;


static uint8_t u8ExecuteFlashWrite(uint16_t* local_u16pdata, uint32_t local_u32AddressOfPage, uint8_t local_u8SizeOfData);
#endif /* INC_BOOTLOADER_PRIVATE_H_ */
