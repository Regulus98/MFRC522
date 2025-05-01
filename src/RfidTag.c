/*
 * RfidTag.c
 *
 *  Created on: Apr 10, 2025
 *      Author: 26081178
 */

#include "RfidTag.h"
#include "MFRC522.h"

void RfidTag_Init(RfidTag* tag)
{
    memset(tag->uid, 0, UID_SIZE);
    tag->type = UNKNOWN_TYPE;
    tag->isAuthenticated = false;
    memset(tag->currentKey, 0xFF, KEY_SIZE); // Default key
    memset(tag->lastReadData, 0, DATA_BLOCK_SIZE);
    tag->lastReadBlock = 0;
}

void RfidTag_SetUID(RfidTag* tag, const uint8_t* uid)
{
    memcpy(tag->uid, uid, UID_SIZE);
}

void RfidTag_SetType(RfidTag* tag, RfidTagType type)
{
    tag->type = type;
}

void RfidTag_SetAuthenticated(RfidTag* tag, bool authenticated)
{
    tag->isAuthenticated = authenticated;
}

void RfidTag_SetKey(RfidTag* tag, const uint8_t* key)
{
    memcpy(tag->currentKey, key, KEY_SIZE);
}

void RfidTag_StoreReadData(RfidTag* tag, uint8_t block, const uint8_t* data)
{
    memcpy(tag->lastReadData, data, DATA_BLOCK_SIZE);
    tag->lastReadBlock = block;
}

const char* RfidTag_GetTypeString(RfidTag* tag)
{
    switch(tag->type)
    {
        case MIFARE_ULTRALIGHT: return "MIFARE Ultralight";
        case MIFARE_1K: return "MIFARE Classic 1K";
        case MIFARE_4K: return "MIFARE Classic 4K";
        case MIFARE_DESFIRE: return "MIFARE DESFire";
        default: return "Unknown";
    }
}

void RfidTag_PrintInfo(RfidTag* tag)
{
    printf("Rfid Tag Information:\n");
    printf("UID: ");
    for(int i = 0; i < UID_SIZE; i++) {
        printf("%02X ", tag->uid[i]);
    }
    printf("\nType: %s\n", RfidTag_GetTypeString(tag));
    printf("Authenticated: %s\n", tag->isAuthenticated ? "Yes" : "No");

    if(tag->isAuthenticated) {
        printf("Last read block: %d\n", tag->lastReadBlock);
        printf("Data: ");
        for(int i = 0; i < DATA_BLOCK_SIZE; i++) {
            printf("%02X ", tag->lastReadData[i]);
        }
        printf("\nString Data: %s\n", tag->lastReadData);
        printf("\n");
    }
}
