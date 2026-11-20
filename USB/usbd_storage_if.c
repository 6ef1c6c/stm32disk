#include <stddef.h>
#include "usbd_storage_if.h"
#include "block_device.h"

static int8_t s_inquiry[36] = {
    0x00, 0x80, 0x04, 0x02, 31,
    0x00, 0x00, 0x00,
    'G','e','n','e','r','i','c',' ',
    'M','a','s','s',' ','S','t','o','r','a','g','e',' ',' ',' ',' ',
    '1','.','0',' '
};

static int8_t STORAGE_Init(uint8_t lun)
{
    return (lun <= BlockDev_MAX_LUN) ? 0 : -1;
}

static int8_t STORAGE_GetCapacity(uint8_t lun, uint32_t *block_num, uint16_t *block_size)
{
    if (!block_num || !block_size || lun > BlockDev_MAX_LUN) return -1;
    *block_num = (lun == 0u) ? BlockDev_GetRamSectorCount() : BlockDev_GetFlashSectorCount();
    *block_size = BlockDev_SECTOR_SIZE;
    return 0;
}

static int8_t STORAGE_IsReady(uint8_t lun)
{
    return BlockDev_IsReady(lun) ? -1 : 0;
}

static int8_t STORAGE_IsWriteProtected(uint8_t lun)
{
    return BlockDev_IsWriteProtected(lun) ? 1 : 0;
}

static int8_t STORAGE_Read(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
    return BlockDev_Read(lun, blk_addr, buf, blk_len) ? -1 : 0;
}

static int8_t STORAGE_Write(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
    return BlockDev_Write(lun, blk_addr, buf, blk_len) ? -1 : 0;
}

static int8_t STORAGE_GetMaxLun(void)
{
    return (int8_t)BlockDev_GetMaxLun();
}

USBD_StorageTypeDef USBD_STORAGE_fops = {
    STORAGE_Init,
    STORAGE_GetCapacity,
    STORAGE_IsReady,
    STORAGE_IsWriteProtected,
    STORAGE_Read,
    STORAGE_Write,
    STORAGE_GetMaxLun,
    s_inquiry
};
