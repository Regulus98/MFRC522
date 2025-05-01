/*
 * MFRC522.c
 *
 *  Created on: Apr 10, 2025
 *      Author: Islam Bedirşah
 */

#include "MFRC522.h"
#include "main.h"

#define MFRC522_CS_PORT    GPIOB
#define MFRC522_CS_PIN     GPIO_PIN_12
#define MFRC522_RST_PORT   GPIOC
#define MFRC522_RST_PIN    GPIO_PIN_6

#define CS_LOW()     HAL_GPIO_WritePin(MFRC522_CS_PORT, MFRC522_CS_PIN, GPIO_PIN_RESET)
#define CS_HIGH()    HAL_GPIO_WritePin(MFRC522_CS_PORT, MFRC522_CS_PIN, GPIO_PIN_SET)


static void MFRC522_WriteRegister(uint8_t, uint8_t);
static uint8_t MFRC522_ReadRegister(uint8_t);
static uint8_t MFRC522_ToCard(uint8_t, uint8_t*, uint8_t, uint8_t*, uint32_t*);
static uint8_t MFRC522_Authenticate(uint8_t, uint8_t, uint8_t*, uint8_t*);


/*
 * Function Name: MFRC522_WriteRegister
 * Function Description: To a certain MFRC522 register to write a byte of data
 * Input Parameters: addr - register address; val - the value to be written
 * Return value: None
 */
static void MFRC522_WriteRegister(uint8_t addr, uint8_t val)
{
	uint8_t addr_bits = (addr << 1) & 0x7E;

	/* Select slave */
	CS_LOW();

	/* Send address */
	HAL_SPI_Transmit(&hspi2, &addr_bits, 1, 500);

	/* Send data */
	HAL_SPI_Transmit(&hspi2, &val, 1, 500);

	/* Release slave again */
	CS_HIGH();
}


/*
 * Function Name: MFRC522_ReadRegister
 * Description: From a certain MFRC522 read a byte of data register
 * Input Parameters: addr - register address
 * Returns: a byte of data read from the
 */
static uint8_t MFRC522_ReadRegister(uint8_t addr)
{
	uint8_t rx_bits;
	uint8_t addr_bits = ((addr << 1) & 0x7E) | 0x80;

	/* Select slave */
	CS_LOW();

	/* Send address */
	HAL_SPI_Transmit(&hspi2, &addr_bits, 1, 500);

	/* Read Data */
	HAL_SPI_Receive(&hspi2, &rx_bits, 1, 500);

	/* Release slave again */
	CS_HIGH();

	return rx_bits;
}


/*
 * Function Name: MFRC522_Init
 * Description: Initialize RC522
 * Input: None
 * Return value: None
*/
void MFRC522_Init(void)
{
	MFRC522_Reset();

	/* Timer Td =  (TPrescaler * 2 + 1) * (TreloadVal + 1) / 13.56MHz */
	MFRC522_WriteRegister(MFRC522_REG_T_MODE, 0x80);
	MFRC522_WriteRegister(MFRC522_REG_T_PRESCALER, 0xA9);
	MFRC522_WriteRegister(MFRC522_REG_T_RELOAD_L, 0x03);
	MFRC522_WriteRegister(MFRC522_REG_T_RELOAD_H, 0xE8);

	MFRC522_WriteRegister(MFRC522_REG_TX_AUTO, 0x40);     // force 100% ASK modulation
	MFRC522_WriteRegister(MFRC522_REG_MODE, 0x3D);        // CRC Initial value 0x6363

	/* Turn antenna on */
	MFRC522_AntennaOn();
}


/*
 * Function Name: MFRC522_Reset
 * Description: Reset RC522
 * Input: None
 * Return value: None
 */
void MFRC522_Reset(void)
{
	MFRC522_WriteRegister(MFRC522_REG_COMMAND, PCD_RESETPHASE);
}


/*
 * Function Name: MFRC522_SetBitMask
 * Description: Set RC522 register bit
 * Input parameters: reg - register address; mask - set value
 * Return value: None
 */
void MFRC522_SetBitMask(uint8_t reg, uint8_t mask)
{
    uint8_t tmp = MFRC522_ReadRegister(reg);
    MFRC522_WriteRegister(reg, tmp | mask);  /* set bit mask */
}


/*
 * Function Name: MFRC522_ClearBitMask
 * Description: clear RC522 register bit
 * Input parameters: reg - register address; mask - clear bit value
 * Return value: None
*/
void MFRC522_ClearBitMask(uint8_t reg, uint8_t mask)
{
    uint8_t tmp = MFRC522_ReadRegister(reg);
    MFRC522_WriteRegister(reg, tmp & (~mask));  /* clear bit mask */
}


/*
 * Function Name: MFRC522_AntennaOn
 * Description: Open antennas, each time you start or shut down the natural barrier between the transmitter should be at least 1ms interval
 * Input: None
 * Return value: None
 */
void MFRC522_AntennaOn(void)
{
	MFRC522_SetBitMask(MFRC522_REG_TX_CONTROL, 0x03);
}


/*
 * Function Name: MFRC522_AntennaOff
  * Description: Close antennas, each time you start or shut down the natural barrier between the transmitter should be at least 1ms interval
  * Input: None
  * Return value: None
 */
void MFRC522_AntennaOff(void)
{
	MFRC522_ClearBitMask(MFRC522_REG_TX_CONTROL, 0x03);
}


static uint8_t MFRC522_ToCard(uint8_t command, uint8_t *sendData, uint8_t sendLen, uint8_t *backData, uint32_t *backLen)
{
    uint8_t irqEn = 0, waitIRq = 0, n, lastBits;
    uint32_t i;

    if (command == PCD_AUTHENT) { irqEn = 0x12; waitIRq = 0x10; }
    else if (command == PCD_TRANSCEIVE) { irqEn = 0x77; waitIRq = 0x30; }

    MFRC522_WriteRegister(MFRC522_REG_COMM_IEN, irqEn | 0x80);	// Interrupt request
    MFRC522_ClearBitMask(MFRC522_REG_COMM_IRQ, 0x80);			// Clear all interrupt request bit
    MFRC522_SetBitMask(MFRC522_REG_FIFO_LEVEL, 0x80);			// FlushBuffer=1, FIFO Initialization
    MFRC522_WriteRegister(MFRC522_REG_COMMAND, PCD_IDLE);		// NO action; Cancel the current command

    for (i = 0; i < sendLen; i++) {
    	MFRC522_WriteRegister(MFRC522_REG_FIFO_DATA, sendData[i]);
    }

    MFRC522_WriteRegister(MFRC522_REG_COMMAND, command);
    if (command == PCD_TRANSCEIVE)
    	MFRC522_SetBitMask(MFRC522_REG_BIT_FRAMING, 0x80);		// StartSend = 1, transmission of data starts

    /* i according to the clock frequency adjustment, the operator M1 card maximum waiting time 25ms */
    i = 1000;
    do
    {
        n = MFRC522_ReadRegister(MFRC522_REG_COMM_IRQ);
    } while (--i && !(n & 0x01) && !(n & waitIRq));


    MFRC522_ClearBitMask(MFRC522_REG_BIT_FRAMING, 0x80);

    if (i && !(MFRC522_ReadRegister(MFRC522_REG_ERROR) & 0x1B))
    {
        if (n & irqEn & 0x01) return MI_NOTAGERR;
        if (command == PCD_TRANSCEIVE)
        {
            n = MFRC522_ReadRegister(MFRC522_REG_FIFO_LEVEL);
            lastBits = MFRC522_ReadRegister(MFRC522_REG_CONTROL) & 0x07;
            *backLen = lastBits ? (n - 1) * 8 + lastBits : n * 8;

            if (n > MFRC522_MAX_LEN) n = MFRC522_MAX_LEN;

            for (i = 0; i < n; i++)
            {
                backData[i] = MFRC522_ReadRegister(MFRC522_REG_FIFO_DATA);
            }
        }

        return MI_OK;
    }

    return MI_ERR;
}


/*
 * Function Name: MFRC522_Request
 * Description: Find cards, read the card type number
 * Input parameters: reqMode - find cards way
 *   TagType - Return Card Type
 *    0x4400 = Mifare_UltraLight
 *    0x0400 = Mifare_One(S50)
 *    0x0200 = Mifare_One(S70)
 *    0x0800 = Mifare_Pro(X)
 *    0x4403 = Mifare_DESFire
 * Return value: the successful return MI_OK
 */
uint8_t MFRC522_Request(uint8_t reqMode, uint8_t *TagType)
{
	uint8_t status;
	uint32_t backBits = 0; 				/* The received data bits */

	/* Defines the number of bits of the last byte that will be transmitted */
	/* TxLastBists = BitFramingReg[2..0] */
	MFRC522_WriteRegister(MFRC522_REG_BIT_FRAMING, 0x07);

	status = MFRC522_ToCard(PCD_TRANSCEIVE, &reqMode, 1, TagType, &backBits);

	if ( status != MI_OK || backBits != 0x10 ) return MI_ERR;

	return status;
}


/*
 * Function Name: MFRC522_Anticoll
 * Description: Anti-collision detection, reading selected card serial number card
 * Input parameters: serNum - returns 4 bytes card serial number, the first 5 bytes for the checksum byte
 * Return value: the successful return MI_OK
 */
uint8_t MFRC522_Anticoll(uint8_t *serNum)
{
    uint32_t unLen;

    MFRC522_WriteRegister(MFRC522_REG_BIT_FRAMING, 0x00);

    /* Send To Card */
    serNum[0] = PICC_ANTICOLL;
    serNum[1] = 0x20;
    uint8_t status = MFRC522_ToCard(PCD_TRANSCEIVE, serNum, 2, serNum, &unLen);

    if (status == MI_OK)
    {
        uint8_t check = 0;

        for (uint8_t i = 0; i < 4; i++)
        	check ^= serNum[i];

        if (check != serNum[4]) status = MI_ERR;
    }

    return status;
}


/*
 * Function Name: MFRC522_CalculateCRC
 * Description: CRC calculation with MF522
 * Input parameters: pIndata - To read the CRC data, len - the data length, pOutData - CRC calculation results
 * Return value: None
 */
uint8_t MFRC522_CalculateCRC(uint8_t *pIndata, uint8_t len, uint8_t *pOutData)
{
	MFRC522_ClearBitMask(MFRC522_REG_DIV_IRQ, 0x04);			// Clear the CRCIRq interrupt request bit
	MFRC522_SetBitMask(MFRC522_REG_FIFO_LEVEL, 0x80);			// FlushBuffer = 1, FIFO initialization

	/* Write data to the FIFO */
	for (uint16_t i = 0; i < len; i++)
	{
		MFRC522_WriteRegister( MFRC522_REG_FIFO_DATA, *(pIndata + i) );
	}

	/* Start the calculation */
	MFRC522_WriteRegister(MFRC522_REG_COMMAND, PCD_CALCCRC);

	/* Wait CRC calculation is complete */
	uint16_t n, timeout = 0x400;

	while ( timeout-- )
	{
		n = MFRC522_ReadRegister(MFRC522_REG_DIV_IRQ);

		if ( n & 0x04 ) 	// CRCIrq = 1
		{
			/* Stop calculating CRC for new content in the FIFO */
			MFRC522_WriteRegister(MFRC522_REG_COMMAND, PCD_IDLE);

			/* Read CRC calculation result */
			pOutData[0] = MFRC522_ReadRegister(MFRC522_REG_CRC_RESULT_L);
			pOutData[1] = MFRC522_ReadRegister(MFRC522_REG_CRC_RESULT_M);

			return MI_OK;
		}
	}

	return MI_TIMEOUT;
}


/*
 * Function Name: MFRC522_Write
 * Description: Write block data
 * Input parameters: blockAddr - block address; writeData - to 16-byte data block write
 * Return value: the successful return MI_OK
 */
uint8_t MFRC522_Write(uint8_t blockAddr, uint8_t *writeData)
{
	uint8_t status;
	uint32_t recvBits;
	uint8_t buff[18];

	/* Set Buffer Data */
	buff[0] = PICC_WRITE;
	buff[1] = blockAddr;

	/* CRC Check */
	MFRC522_CalculateCRC(buff, 2, &buff[2]);

	/* Step 1: Tell the PICC we want to write to block blockAddr */
	status = MFRC522_ToCard(PCD_TRANSCEIVE, buff, 4, buff, &recvBits);

	if ( status != MI_OK ) return MI_ERR;

	/* Data to the FIFO write 16Byte */
	memcpy(buff, writeData, 16);

	/* CRC Check */
	MFRC522_CalculateCRC(buff, 16, &buff[16]);

	/* Step 2: Transfer the data */
	status = MFRC522_ToCard(PCD_TRANSCEIVE, buff, 18, buff, &recvBits);

	if ( status != MI_OK ) return MI_ERR;

	return status;
}


/*
 * Function Name: MFRC522_Read
 * Description: Read block data
 * Input parameters: blockAddr - block address; recvData - read block data
 * Return value: the successful return MI_OK
 */
uint8_t MFRC522_Read(uint8_t blockAddr, uint8_t *recvData)
{
	uint8_t status;
	uint32_t unLen;

	/* Set Buffer Data */
	recvData[0] = PICC_READ;
	recvData[1] = blockAddr;

	/* CRC Check */
	MFRC522_CalculateCRC(recvData, 2, &recvData[2]);

	/* Read Data from PCD (Proximity Coupling Device) */
	status = MFRC522_ToCard(PCD_TRANSCEIVE, recvData, 4, recvData, &unLen);

	return (status == MI_OK && unLen == 0x90) ? MI_OK : MI_ERR;
}


/*
 * Function Name: MFRC522_Authenticate
 * Description: Verify card password
 * Input parameters: authMode - Password Authentication Mode
                 0x60 = A key authentication
                 0x61 = Authentication Key B
             BlockAddr--Block address
             Sectorkey--Sector password
             serNum--Card serial number, 4-byte
 * Return value: the successful return MI_OK
 */
static uint8_t MFRC522_Authenticate(uint8_t AuthMode, uint8_t BlockAddr, uint8_t *Sectorkey, uint8_t *SerialNum)
{
	uint8_t ucBuffer[12];

	/* Verify the command + block address + sector password + card serial number */
	ucBuffer[0] = AuthMode;
	ucBuffer[1] = BlockAddr;

	/* Copy sector key */
	memcpy(&ucBuffer[2], Sectorkey, 6);

	/* Copy serial number */
	memcpy(&ucBuffer[8], SerialNum, 4);

	/* Start the authentication */
	uint8_t status = MFRC522_ToCard(PCD_AUTHENT, ucBuffer, sizeof(ucBuffer), NULL, NULL);

	/* Check the register value */
	uint16_t Crypto1On = MFRC522_ReadRegister( MFRC522_REG_STATUS2 ) & 0x08;

	if ( status != MI_OK || !Crypto1On )
	{	status = MI_ERR;	}

	return status;
}


/*
 * Function Name: MFRC522_SelectTag
 * Description: election card, read the card memory capacity
 * Input parameters: SerialNum - Incoming card serial number
 * Return value: the successful return of card capacity
 */
uint8_t MFRC522_SelectTag(uint8_t *SerialNum)
{
    uint8_t buffer[9] = { PICC_SELECTTAG, 0x70 };  	// NVB - Number of Valid Bits: Seven whole bytes

    /* Copy card serial to buffer */
    memcpy( &buffer[2], SerialNum, 4 );

    /* Calculate BCC - Block Check Character */
    buffer[6] = buffer[2] ^ buffer[3] ^ buffer[4] ^ buffer[5];

    /* Calculate CRC */
    if ( MFRC522_CalculateCRC( buffer, 7, &buffer[7] ) != MI_OK )  return MI_ERR;

    uint32_t recvBits;
    uint8_t status = MFRC522_ToCard( PCD_TRANSCEIVE, buffer, sizeof(buffer), buffer, &recvBits );

    return ( status == MI_OK && recvBits == 0x18 ) ? buffer[0] : 0;
}


/*
 * Function Name: MFRC522_Halt
 * Description: Command card into hibernation
 * Input: None
 * Return value: None
 */
void MFRC522_Halt(void)
{
	uint32_t unLen;
	uint8_t buff[4];

	buff[0] = PICC_HALT;
	buff[1] = 0;
	MFRC522_CalculateCRC( buff, 2, &buff[2] );

	MFRC522_ToCard( PCD_TRANSCEIVE, buff, 4, buff, &unLen );
}


void MFRC522_StopCrypto1(void)
{
	/* Clear MFCrypto1On bit */
	MFRC522_ClearBitMask(MFRC522_REG_STATUS2, 0x08);  /* Status2Reg[7..0] bits are: TempSensClear I2CForceHS reserved reserved   MFCrypto1On ModemState[2:0] */
}


/*
 * Write Specific Block to MIFARE Classic ( RFID Card )
 * Writes 16 bytes to the active PICC
 */
uint8_t MFRC522_WriteBlock( uint8_t ucBlockNumber, uint8_t *pucBuffer, uint8_t *Sectorkey, uint8_t *SerialNum )
{
	/* This makes sure that we only write into data blocks. Every 4th block is a trailer block for the access/security info. */
	uint16_t usTrailerBlock = (ucBlockNumber / 4) * 4 + 3;		/* Determine trailer block for the sector */

	if ( ucBlockNumber > 2 && ( ucBlockNumber + 1 ) % 4 == 0 )
	{	return MI_ERR;	}

	/* Authentication of the desired block for access */
	uint8_t status = MFRC522_Authenticate( PICC_AUTHENT1A, usTrailerBlock, Sectorkey, SerialNum );

	if ( status != MI_OK )
	{	return MI_ERR;	}

	/* Writing the block */
	status = MFRC522_Write(ucBlockNumber, pucBuffer);

	if ( status != MI_OK )
	{	return MI_ERR;	}

	return MI_OK;
}


/*
 * Read Specific Block from MIFARE Classic 1kB ( RFID Card )
 * Read 16 bytes block
 * Buffer size have to be 18 bytes
 */
uint8_t MFRC522_ReadBlock( uint8_t ucBlockNumber, uint8_t *pucBuffer, uint8_t *Sectorkey, uint8_t *SerialNum )
{
	uint16_t usTrailerBlock = (ucBlockNumber / 4) * 4 + 3;		/* Determine trailer block for the sector */

	/* Authentication of the desired block for access */
	uint8_t status = MFRC522_Authenticate( PICC_AUTHENT1A, usTrailerBlock, Sectorkey, SerialNum );

	if ( status != MI_OK )
	{	return MI_ERR;	}

	/* Reading data from the block */
	status = MFRC522_Read(ucBlockNumber, pucBuffer); 		/* Note: pucBuffer have to be 18 byte size */

	/* Stop encrypting */
	MFRC522_StopCrypto1( );

	if ( status != MI_OK )
	{	return MI_ERR;	}

	return MI_OK;
}


uint8_t MFRC522_ReadCardSerial( uint8_t *TagId, uint8_t *TagType )
{
	if ( MFRC522_Request( PICC_REQIDL, TagType ) != MI_OK || MFRC522_Anticoll( TagId ) != MI_OK )
	{	return MI_ERR;	}

	/* Select the card and get its SAK */
	if ( !MFRC522_SelectTag(TagId) )
	{	return MI_ERR;	}

	return MI_OK;
}
