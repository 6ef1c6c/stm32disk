#ifndef BLOCK_DEVICE_H
#define BLOCK_DEVICE_H
#include <stdint.h>

#define BlockDev_SECTOR_SIZE             512u
#define BlockDev_FLASH_BASE              0x0800A000u
#define BlockDev_FLASH_APP_SIZE          0x0000A000u
#define BlockDev_FLASH_PAGE_SIZE         1024u
#define BlockDev_FLASH_SAFETY_MARGIN     1024u
#define BlockDev_RAM_SAFETY_MARGIN       3072u
#define BlockDev_MAX_LUN                 1u

void BlockDev_Init(void);
uint8_t BlockDev_GetMaxLun(void);
uint8_t BlockDev_IsReady(uint8_t lun);
uint8_t BlockDev_IsWriteProtected(uint8_t lun);
uint8_t BlockDev_Read(uint8_t lun, uint32_t lba, uint8_t *buf, uint16_t blocks);
uint8_t BlockDev_Write(uint8_t lun, uint32_t lba, const uint8_t *buf, uint16_t blocks);
uint8_t BlockDev_Flush(uint8_t lun);
void BlockDev_Process(void);
uint32_t BlockDev_GetRamSectorCount(void);
uint32_t BlockDev_GetFlashSectorCount(void);

#endif
