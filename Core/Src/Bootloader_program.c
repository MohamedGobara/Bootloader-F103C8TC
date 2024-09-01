/*
 * Bootloader_program.c
 *
 *  Created on: Aug 26, 2024
 *      Author: Gobara
 */

#include "main.h"
#include <stdint.h>
#include <string.h>
#include "Bootloader_interface.h"
#include "Bootloader_private.h"

/* External declarations for the CRC and UART handlers */
extern CRC_HandleTypeDef hcrc;
extern UART_HandleTypeDef huart1;
/* Verifies the CRC of the received data */
static uint8_t u8VerifyCRC(uint8_t *copy_u8DataArr, uint8_t Copy_u8Length,
		uint32_t copy_u32HostCRC) {
	uint32_t local_u32Temp, local_u32CRCAccumlative;
	uint8_t counter;

	/* Accumulate the CRC for each byte in the data array */
	for (counter = 0; counter < Copy_u8Length; counter++) {
		local_u32Temp = copy_u8DataArr[counter];
		local_u32CRCAccumlative = HAL_CRC_Accumulate(&hcrc, &local_u32Temp, 1);
	}

	/* Reset the CRC calculation unit */
	__HAL_CRC_DR_RESET(&hcrc);

	/* Return success or fail based on CRC comparison */
	return local_u32CRCAccumlative == copy_u32HostCRC ? CRC_SUCCESS : CRC_FAIL;
}

/* Sends an ACK (acknowledgement) response with the specified reply length */
static void vSendACK(uint8_t copy_u8ReplyLength) {
	uint8_t Local_u8AckBuffer[2] = { Bootloader_ACK, copy_u8ReplyLength };

	/* Transmit the ACK response */
	HAL_UART_Transmit(&huart1, Local_u8AckBuffer, 2, HAL_MAX_DELAY);
}

/* Sends a NACK (negative acknowledgement) response */
static void vSendNACK(void) {
	uint8_t Local_u8NACK = Bootloader_NACK;

	/* Transmit the NACK response */
	HAL_UART_Transmit(&huart1, &Local_u8NACK, 1, HAL_MAX_DELAY);
}

static uint8_t u8ValidDataAddress(uint32_t Copy_u32Address) {

	uint8_t local_u8AddressStatus = INVALID_ADDRESS_STATUS;

	/* Address is valid if it within : SRAM or FLASH */

	if (Copy_u32Address >= FLASH_BASE && Copy_u32Address <= FLASH_BANK1_END) {

		local_u8AddressStatus = VALID_ADDRESS_STATUS;

	} else if (Copy_u32Address >= SRAM_BASE
			&& Copy_u32Address <= ((uint32_t) SRAM_BASE + 0x5000U)) {

		local_u8AddressStatus = VALID_ADDRESS_STATUS;

	}

	return local_u8AddressStatus;

}


static uint8_t u8ExecuteFlashWrite(uint16_t* local_u16pdata, uint32_t local_u32AddressOfPage, uint8_t local_u8SizeOfData)
{
	uint8_t MyHalStatus= HAL_ERROR ;
	if(u8ValidDataAddress(local_u32AddressOfPage)==VALID_ADDRESS_STATUS){

		MyHalStatus=HAL_OK ;
		uint8_t local_u8ConterInFlash=0 ;


		uint8_t local_u8Conter;
		for(local_u8Conter=0 ; local_u8Conter<(local_u8SizeOfData/2);local_u8Conter++){
			HAL_FLASH_Unlock() ;
			MyHalStatus=HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, local_u32AddressOfPage+local_u8ConterInFlash, (uint64_t)local_u16pdata[local_u8Conter]) ;
			local_u8ConterInFlash+=2;
			HAL_FLASH_Lock();

		}

	}
	return MyHalStatus;


}

static uint8_t u8ExecuteFlashErease(uint8_t copy_u8PageNumber,
		uint8_t copy_u8NumberOfPages) {

	uint8_t local_u8HalFlashStatus;

	if (copy_u8PageNumber> MAX_NUMBER_OF_PAGES && copy_u8PageNumber!=FLASH_MASS_ERASE_PATTERN) {
		local_u8HalFlashStatus = HAL_ERROR;

	} else {

		FLASH_EraseInitTypeDef lcoal_MyFlashErase;
		uint32_t local_u32PageStatus ;

		if (copy_u8PageNumber == FLASH_MASS_ERASE_PATTERN) {

			lcoal_MyFlashErase.TypeErase = FLASH_TYPEERASE_MASSERASE;

		} else {

			lcoal_MyFlashErase.TypeErase = FLASH_TYPEERASE_PAGES;

		}

		uint8_t local_u8MaxNumber = MAX_NUMBER_OF_PAGES - copy_u8PageNumber;

		copy_u8NumberOfPages =
				copy_u8NumberOfPages > local_u8MaxNumber ?
						local_u8MaxNumber : copy_u8NumberOfPages;
		lcoal_MyFlashErase.NbPages = (uint32_t) copy_u8NumberOfPages;
		lcoal_MyFlashErase.Banks = FLASH_BANK_1 ;
		lcoal_MyFlashErase.PageAddress = (((uint32_t)FLASH_BASE )+ ((uint32_t)copy_u8PageNumber*1024));

		HAL_FLASH_Unlock() ;
		  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_SET);
		local_u8HalFlashStatus=HAL_FLASHEx_Erase(&lcoal_MyFlashErase, &local_u32PageStatus) ;
		  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_RESET);

		HAL_FLASH_Lock();

	}

	return local_u8HalFlashStatus;

}

/* Jumps to the user application at the specified address */
void Bootloader_JumpToUserApp(void) {
	/* Pointer to function for holding the address of the user app's reset handler */
	void (*localPtr_userApp)(void);
	/* Variable to hold the stack pointer value of the user application */
	uint32_t local_u32MSPValue;

	/* Get the value of the stack pointer from the user app's memory */
	local_u32MSPValue = *((volatile uint32_t*) USER_APP_ADDRESS);

	/* Set the stack pointer value in the MSP register */
	__asm volatile ("MSR MSP, %0" :: "r"(local_u32MSPValue));

	/* Get the reset handler of the user application */
	localPtr_userApp = ((volatile void*) *((volatile uint32_t*) USER_APP_ADDRESS
			+ 1));

	/* Jump to the user application */
	localPtr_userApp();
}

/* Handles the GET_VER command to return the bootloader version */
void Bootloader_vHandle_GET_VER(uint8_t *localPacketptr) {
	uint8_t local_u8TotalSizeOfPacket = localPacketptr[0] + 1;
	uint32_t local_u32CRCReceived = *((uint32_t*) (localPacketptr
			+ local_u8TotalSizeOfPacket - 4));

	/* Verify the CRC of the received packet */
	if (u8VerifyCRC(localPacketptr, (local_u8TotalSizeOfPacket - 4),
			local_u32CRCReceived) == CRC_SUCCESS) {
		vSendACK(1);

		uint8_t local_u8BLVersion = Bootloader_VERSION;

		/* Send the bootloader version */
		HAL_UART_Transmit(&huart1, &local_u8BLVersion, 1, HAL_MAX_DELAY);
	} else {
		vSendNACK();
	}
}

/* Handles the GET_HELP command */
void Bootloader_vHandle_GET_HELP(uint8_t *localPacketptr) {

	uint8_t local_u8TotalSizeOfPacket = localPacketptr[0] + 1;
	uint32_t local_u32CRCReceived = *((uint32_t*) (localPacketptr
			+ local_u8TotalSizeOfPacket - 4));

	uint8_t local_u8cCommands[] = { BL_GET_VER,
	BL_GET_HELP,
	BL_GET_CID,
	BL_RDP_STATUS,
	BL_GO_TO_ADDR,
	BL_FLASH_EREASE,
	BL_MEM_WRITE,
	BL_EN_RW_PROTECT,
	BL_MEM_READ,
	BL_READ_SECTOR_STATUS,
	BL_OTP_READ,
	BL_DIS_WR_PROTECTION };
	/* Verify the CRC of the received packet */
	if (u8VerifyCRC(localPacketptr, (local_u8TotalSizeOfPacket - 4),
			local_u32CRCReceived) == CRC_SUCCESS) {
		vSendACK(sizeof(local_u8cCommands));
		HAL_UART_Transmit(&huart1, local_u8cCommands,
				(uint16_t) sizeof(local_u8cCommands), HAL_MAX_DELAY);
	} else {
		vSendNACK();
	}

}

/* Handles the GET_CID command */
void Bootloader_vHandle_GET_CID(uint8_t *localPacketptr) {
	uint8_t local_u8TotalSizeOfPacket = localPacketptr[0] + 1;
	uint32_t local_u32CRCReceived = *((uint32_t*) (localPacketptr
			+ local_u8TotalSizeOfPacket - 4));

	/* Verify the CRC of the received packet */
	if (u8VerifyCRC(localPacketptr, (local_u8TotalSizeOfPacket - 4),
			local_u32CRCReceived) == CRC_SUCCESS) {
		vSendACK(2);

		uint16_t Local_u16CID = (DBGMCU_IDCODE_REG & 0xFFF);

		HAL_UART_Transmit(&huart1, (uint8_t*) &Local_u16CID, 2, HAL_MAX_DELAY);
	} else {
		vSendNACK();
	}
}

/* Handles the RDP_STATUS command */
void Bootloader_vHandle_RDP_STATUS(uint8_t *localPacketptr) {
	uint8_t local_u8TotalSizeOfPacket = localPacketptr[0] + 1;
	uint32_t local_u32CRCReceived = *((uint32_t*) (localPacketptr
			+ local_u8TotalSizeOfPacket - 4));

	/* Verify the CRC of the received packet */
	if (u8VerifyCRC(localPacketptr, (local_u8TotalSizeOfPacket - 4),
			local_u32CRCReceived) == CRC_SUCCESS) {
		//vSendACK(2);

		/* Send the bootloader version */
		//HAL_UART_Transmit(&huart1, &, , HAL_MAX_DELAY);
	} else {
		vSendNACK();
	}
}

/* Handles the GO_TO_ADDR command */
void Bootloader_vHandle_GO_TO_ADDR(uint8_t *localPacketptr) {

	uint8_t local_u8TotalSizeOfPacket = localPacketptr[0] + 1;

	uint32_t local_u32CRCReceived = *((uint32_t*) (localPacketptr
			+ local_u8TotalSizeOfPacket - 4));

	/* Verify the CRC of the received packet */
	if (u8VerifyCRC(localPacketptr, (local_u8TotalSizeOfPacket - 4),
			local_u32CRCReceived) == CRC_SUCCESS) {

		vSendACK(1);

		uint32_t local_u32Address = *((uint32_t*) &localPacketptr[2]);

		uint8_t local_u8AddressStatus = u8ValidDataAddress(local_u32Address);
		if (local_u8AddressStatus == VALID_ADDRESS_STATUS) {
			void (*local_PtrFun)(void) = NULL;
			HAL_UART_Transmit(&huart1, (uint8_t*) &local_u8AddressStatus, 1,
			HAL_MAX_DELAY);

			/* Set T-BIT for any address value */
			local_u32Address |= 0x01;

			local_PtrFun = (void*) local_u32Address;

			local_PtrFun();

		} else {
			HAL_UART_Transmit(&huart1, (uint8_t*) &local_u8AddressStatus, 1,
			HAL_MAX_DELAY);

		}
	} else {
		vSendNACK();
	}
}

/* Handles the FLASH_EREASE command */
void Bootloader_vHandle_FLASH_EREASE(uint8_t *localPacketptr) {
	uint8_t local_u8TotalSizeOfPacket = localPacketptr[0] + 1;
	uint32_t local_u32CRCReceived = *((uint32_t*) (localPacketptr
			+ local_u8TotalSizeOfPacket - 4));

	/* Verify the CRC of the received packet */
	if (u8VerifyCRC(localPacketptr, (local_u8TotalSizeOfPacket - 4),
			local_u32CRCReceived) == CRC_SUCCESS) {
			vSendACK(1);
			uint8_t local_u8FlashEraseStatus ;

			local_u8FlashEraseStatus = u8ExecuteFlashErease(localPacketptr[2], localPacketptr[3]);

		HAL_UART_Transmit(&huart1, &local_u8FlashEraseStatus, 1, HAL_MAX_DELAY);
	} else {
		vSendNACK();
	}
}

/* Handles the MEM_WRITE command */
void Bootloader_vHandle_MEM_WRITE(uint8_t *localPacketptr) {
	uint8_t local_u8TotalSizeOfPacket = localPacketptr[0] + 1;
	uint32_t local_u32CRCReceived = *((uint32_t*) (localPacketptr
			+ local_u8TotalSizeOfPacket - 4));

	/* Verify the CRC of the received packet */
	if (u8VerifyCRC(localPacketptr, (local_u8TotalSizeOfPacket - 4),
			local_u32CRCReceived) == CRC_SUCCESS) {
			vSendACK(1);

			uint8_t local_u8Status = u8ExecuteFlashWrite((uint16_t*)&localPacketptr[7], *(uint32_t*)&localPacketptr[2], localPacketptr[6]) ;

		/* Send the bootloader version */
		HAL_UART_Transmit(&huart1, &local_u8Status, 1, HAL_MAX_DELAY);
	} else {
		vSendNACK();
	}
}

/* Handles the EN_RW_PROTECT command */
void Bootloader_vHandle_EN_RW_PROTECT(uint8_t *localPacketptr) {
	uint8_t local_u8TotalSizeOfPacket = localPacketptr[0] + 1;
	uint32_t local_u32CRCReceived = *((uint32_t*) (localPacketptr
			+ local_u8TotalSizeOfPacket - 4));

	/* Verify the CRC of the received packet */
	if (u8VerifyCRC(localPacketptr, (local_u8TotalSizeOfPacket - 4),
			local_u32CRCReceived) == CRC_SUCCESS) {
		//	vSendACK();

		/* Send the bootloader version */
		//HAL_UART_Transmit(&huart1, &, , HAL_MAX_DELAY);
	} else {
		vSendNACK();
	}
}

/* Handles the MEM_READ command */
void Bootloader_vHandle_MEM_READ(uint8_t *localPacketptr) {
	uint8_t local_u8TotalSizeOfPacket = localPacketptr[0] + 1;
	uint32_t local_u32CRCReceived = *((uint32_t*) (localPacketptr
			+ local_u8TotalSizeOfPacket - 4));

	/* Verify the CRC of the received packet */
	if (u8VerifyCRC(localPacketptr, (local_u8TotalSizeOfPacket - 4),
			local_u32CRCReceived) == CRC_SUCCESS) {
		//	vSendACK();

		/* Send the bootloader version */
		//HAL_UART_Transmit(&huart1, &, , HAL_MAX_DELAY);
	} else {
		vSendNACK();
	}
}

/* Handles the READ_SECTOR_STATUS command */
void Bootloader_vHandle_READ_SECTOR_STATUS(uint8_t *localPacketptr) {
	uint8_t local_u8TotalSizeOfPacket = localPacketptr[0] + 1;
	uint32_t local_u32CRCReceived = *((uint32_t*) (localPacketptr
			+ local_u8TotalSizeOfPacket - 4));

	/* Verify the CRC of the received packet */
	if (u8VerifyCRC(localPacketptr, (local_u8TotalSizeOfPacket - 4),
			local_u32CRCReceived) == CRC_SUCCESS) {
		//	vSendACK();

		/* Send the bootloader version */
		//HAL_UART_Transmit(&huart1, &, , HAL_MAX_DELAY);
	} else {
		vSendNACK();
	}
}

/* Handles the OTP_READ command */
void Bootloader_vHandle_OTP_READ(uint8_t *localPacketptr) {
	uint8_t local_u8TotalSizeOfPacket = localPacketptr[0] + 1;
	uint32_t local_u32CRCReceived = *((uint32_t*) (localPacketptr
			+ local_u8TotalSizeOfPacket - 4));

	/* Verify the CRC of the received packet */
	if (u8VerifyCRC(localPacketptr, (local_u8TotalSizeOfPacket - 4),
			local_u32CRCReceived) == CRC_SUCCESS) {
		//	vSendACK();

		/* Send the bootloader version */
		//HAL_UART_Transmit(&huart1, &, , HAL_MAX_DELAY);
	} else {
		vSendNACK();
	}
}

/* Handles the DIS_WR_PROTECTION command */
void Bootloader_vHandle_DIS_WR_PROTECTION(uint8_t *localPacketptr) {
	uint8_t local_u8TotalSizeOfPacket = localPacketptr[0] + 1;
	uint32_t local_u32CRCReceived = *((uint32_t*) (localPacketptr
			+ local_u8TotalSizeOfPacket - 4));

	/* Verify the CRC of the received packet */
	if (u8VerifyCRC(localPacketptr, (local_u8TotalSizeOfPacket - 4),
			local_u32CRCReceived) == CRC_SUCCESS) {
		//	vSendACK();

		/* Send the bootloader version */
		//HAL_UART_Transmit(&huart1, &, , HAL_MAX_DELAY);
	} else {
		vSendNACK();
	}
}

/* Receives data packets and processes them based on command ID */
void Bootloader_vReceiveData(void) {
	/* Local array to store the received packet */
	uint8_t local_u8packet[255] = { 0 };

	while (1) {
		/* Clear the packet buffer before each iteration */
		memset(local_u8packet, 0, 255);

		/* Receive the length of the packet (first byte) */
		HAL_UART_Receive(&huart1, local_u8packet, 1, HAL_MAX_DELAY);

		/* Receive the remaining bytes up to the CRC block */
		HAL_UART_Receive(&huart1, &local_u8packet[1], local_u8packet[0],
		HAL_MAX_DELAY);

		uint8_t invalid_ID_string[] = "Invalid ID\r\n";

		/* Process the packet based on the command ID */
		switch (local_u8packet[1]) {
		case BL_GET_VER:
			Bootloader_vHandle_GET_VER(local_u8packet);
			break;
		case BL_GET_HELP:
			Bootloader_vHandle_GET_HELP(local_u8packet);
			break;
		case BL_GET_CID:
			Bootloader_vHandle_GET_CID(local_u8packet);
			break;
		case BL_RDP_STATUS:
			Bootloader_vHandle_RDP_STATUS(local_u8packet);
			break;
		case BL_GO_TO_ADDR:
			Bootloader_vHandle_GO_TO_ADDR(local_u8packet);
			break;
		case BL_FLASH_EREASE:
			Bootloader_vHandle_FLASH_EREASE(local_u8packet);
			break;
		case BL_MEM_WRITE:
			Bootloader_vHandle_MEM_WRITE(local_u8packet);
			break;
		case BL_EN_RW_PROTECT:
			Bootloader_vHandle_EN_RW_PROTECT(local_u8packet);
			break;
		case BL_MEM_READ:
			Bootloader_vHandle_MEM_READ(local_u8packet);
			break;
		case BL_READ_SECTOR_STATUS:
			Bootloader_vHandle_READ_SECTOR_STATUS(local_u8packet);
			break;
		case BL_OTP_READ:
			Bootloader_vHandle_OTP_READ(local_u8packet);
			break;
		case BL_DIS_WR_PROTECTION:
			Bootloader_vHandle_DIS_WR_PROTECTION(local_u8packet);
			break;
		default:
			HAL_UART_Transmit(&huart1, invalid_ID_string,
					sizeof(invalid_ID_string), HAL_MAX_DELAY);
			break;

		}
	}
}
