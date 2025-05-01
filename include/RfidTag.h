/*
 * RfidTag.h
 *
 *  Created on: Apr 10, 2025
 *      Author: 26081178
 */

#ifndef SRC_MFRC522_RFIDTAG_H_
#define SRC_MFRC522_RFIDTAG_H_

#include <stdint.h>
#include <stdbool.h>

#define UID_SIZE 4
#define KEY_SIZE 6
#define DATA_BLOCK_SIZE 16

typedef enum
{
    MIFARE_ULTRALIGHT = 0x4400,
    MIFARE_1K = 0x0400,
    MIFARE_4K = 0x0200,
    MIFARE_DESFIRE = 0x4403,
    UNKNOWN_TYPE = 0x0000
} RfidTagType;

typedef struct
{
    uint8_t uid[UID_SIZE];
    RfidTagType type;
    bool isAuthenticated;
    uint8_t currentKey[KEY_SIZE];
    uint8_t lastReadData[DATA_BLOCK_SIZE];
    uint8_t lastReadBlock;
} RfidTag;

void RfidTag_Init(RfidTag* tag);
void RfidTag_SetUID(RfidTag* tag, const uint8_t* uid);
void RfidTag_SetType(RfidTag* tag, RfidTagType type);
void RfidTag_SetAuthenticated(RfidTag* tag, bool authenticated);
void RfidTag_SetKey(RfidTag* tag, const uint8_t* key);
void RfidTag_StoreReadData(RfidTag* tag, uint8_t block, const uint8_t* data);
const char* RfidTag_GetTypeString(RfidTag* tag);
void RfidTag_PrintInfo(RfidTag* tag);

#endif /* SRC_MFRC522_RFIDTAG_H_ */
