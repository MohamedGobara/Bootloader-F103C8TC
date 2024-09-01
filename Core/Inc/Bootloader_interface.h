/*
 * Bootloader_interface.h
 *
 *  Created on: Aug 26, 2024
 *      Author: Gobara
 */

#ifndef INC_BOOTLOADER_INTERFACE_H_
#define INC_BOOTLOADER_INTERFACE_H_

/* Define the starting address of the user application */
#define USER_APP_ADDRESS 0x8008800UL  // Address where the user application is stored in flash memory

/* Bootloader command codes sent from the host to the bootloader */
#define BL_GET_VER 						0x51  // Command to get bootloader version
#define BL_GET_HELP 					0x52  // Command to get help and supported commands
#define BL_GET_CID 						0x53  // Command to get chip ID
#define BL_RDP_STATUS					0x54  // Command to get read protection status
#define BL_GO_TO_ADDR					0x55  // Command to jump to a specific address
#define BL_FLASH_EREASE					0x56  // Command to erase flash memory
#define BL_MEM_WRITE					0x57  // Command to write data to memory
#define BL_EN_RW_PROTECT				0x58  // Command to enable read/write protection
#define BL_MEM_READ						0x59  // Command to read data from memory
#define BL_READ_SECTOR_STATUS			0x5A  // Command to read status of memory sectors
#define BL_OTP_READ						0x5B  // Command to read from OTP (One-Time Programmable) memory
#define BL_DIS_WR_PROTECTION			0x5C  // Command to disable write protection

/* Function prototypes */

/* Jumps to the user application at a specific memory address */
void Bootloader_JumpToUserApp(void);

/* Receives data packets sent from the host */
void Bootloader_vReceiveData(void);

/* Handles the command to get the bootloader version */
void Bootloader_vHandle_GET_VER(uint8_t* localPacketptr);

/* Handles the command to get help and supported commands */
void Bootloader_vHandle_GET_HELP(uint8_t* localPacketptr);

/* Handles the command to get the chip ID */
void Bootloader_vHandle_GET_CID(uint8_t* localPacketptr);

/* Handles the command to get the read protection status */
void Bootloader_vHandle_RDP_STATUS(uint8_t* localPacketptr);

/* Handles the command to jump to a specific address */
void Bootloader_vHandle_GO_TO_ADDR(uint8_t* localPacketptr);

/* Handles the command to erase flash memory */
void Bootloader_vHandle_FLASH_EREASE(uint8_t* localPacketptr);

/* Handles the command to write data to memory */
void Bootloader_vHandle_MEM_WRITE(uint8_t* localPacketptr);

/* Handles the command to enable read/write protection */
void Bootloader_vHandle_EN_RW_PROTECT(uint8_t* localPacketptr);

/* Handles the command to read data from memory */
void Bootloader_vHandle_MEM_READ(uint8_t* localPacketptr);

/* Handles the command to read the status of memory sectors */
void Bootloader_vHandle_READ_SECTOR_STATUS(uint8_t* localPacketptr);

/* Handles the command to read from OTP (One-Time Programmable) memory */
void Bootloader_vHandle_OTP_READ(uint8_t* localPacketptr);

/* Handles the command to disable write protection */
void Bootloader_vHandle_DIS_WR_PROTECTION(uint8_t* localPacketptr);

#endif /* INC_BOOTLOADER_INTERFACE_H_ */
