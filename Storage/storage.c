#include "block_device.h"
#include "stm32f1xx_hal.h"
#include <string.h>
#include <stdlib.h>

#define FLASH_SIZE_REG  0x1FFFF7E0u

static uint8_t *s_ram_disk;
static uint8_t s_page_buffer[BlockDev_FLASH_PAGE_SIZE];
static uint8_t s_cache_valid, s_cache_dirty, s_flash_error, s_ram_error, s_initialized;
static uint32_t s_cache_page, s_ram_sectors, s_flash_sectors;

extern char _end[];
extern char __heap_end[];

static uint32_t compute_flash_sectors(void)
{
    uint32_t total = (uint32_t)(*(volatile const uint16_t *)FLASH_SIZE_REG) * 1024u;
    uint32_t min = BlockDev_FLASH_BASE + BlockDev_FLASH_SAFETY_MARGIN + BlockDev_SECTOR_SIZE;
    return (total > min) ? (total - BlockDev_FLASH_BASE - BlockDev_FLASH_SAFETY_MARGIN) / BlockDev_SECTOR_SIZE : 0u;
}

static uint32_t compute_ram_sectors(void)
{
    uint32_t avail = (uint32_t)((uintptr_t)__heap_end - (uintptr_t)_end);
    return (avail > BlockDev_RAM_SAFETY_MARGIN) ? (avail - BlockDev_RAM_SAFETY_MARGIN) / BlockDev_SECTOR_SIZE : 0u;
}

static uint8_t flash_program_page(uint32_t addr, const uint8_t *data)
{
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t page_error = 0;
    uint32_t limit = BlockDev_FLASH_BASE + s_flash_sectors * BlockDev_SECTOR_SIZE;
    HAL_StatusTypeDef status;

    if (addr < BlockDev_FLASH_BASE || addr + BlockDev_FLASH_PAGE_SIZE > limit || (addr & (BlockDev_FLASH_PAGE_SIZE - 1u))) return 1u;

    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.PageAddress = addr;
    erase.NbPages = 1u;

    HAL_FLASH_Unlock();
    status = HAL_FLASHEx_Erase(&erase, &page_error);
    for (uint32_t i = 0; status == HAL_OK && i < BlockDev_FLASH_PAGE_SIZE; i += 4u) {
        uint32_t word = (uint32_t)data[i] | ((uint32_t)data[i + 1u] << 8) |
                        ((uint32_t)data[i + 2u] << 16) | ((uint32_t)data[i + 3u] << 24);
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + i, word);
    }
    HAL_FLASH_Lock();
    return status != HAL_OK;
}

static void flash_cache_commit(void)
{
    if (!s_cache_valid || !s_cache_dirty) return;
    HAL_NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);
    uint8_t r = flash_program_page(s_cache_page, s_page_buffer);
    HAL_NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
    if (r) s_flash_error = 1u;
    else s_cache_dirty = 0u;
}

static void make_boot_sector(uint8_t *b, const char *label, uint32_t sectors)
{
    memset(b, 0, BlockDev_SECTOR_SIZE);
    b[0] = 0xEB; b[1] = 0x3C; b[2] = 0x90;
    memcpy(&b[3], "MSDOS5.0", 8);
    b[11] = 0x00; b[12] = 0x02;
    b[13] = 0x01;
    b[14] = 0x01; b[15] = 0x00;
    b[16] = 0x01;
    b[17] = 0x10; b[18] = 0x00;
    b[19] = (uint8_t)(sectors & 0xFFu);
    b[20] = (uint8_t)(sectors >> 8);
    b[21] = 0xF8;
    b[22] = 0x01; b[23] = 0x00;
    b[24] = 0x01; b[25] = 0x00;
    b[26] = 0x01; b[27] = 0x00;
    b[36] = 0x80; b[38] = 0x29;
    memset(&b[43], ' ', 11);
    for (uint8_t i = 0; i < 11u && label[i]; ++i) b[43 + i] = (uint8_t)label[i];
    memcpy(&b[54], "FAT12   ", 8);
    b[510] = 0x55; b[511] = 0xAA;
}

static uint8_t flash_is_blank(void)
{
    const uint32_t *p = (const uint32_t *)(uintptr_t)BlockDev_FLASH_BASE;
    uint32_t n = s_flash_sectors * BlockDev_SECTOR_SIZE / 4u;
    for (uint32_t i = 0; i < n; ++i) if (p[i] != 0xFFFFFFFFu) return 0u;
    return 1u;
}

static uint8_t format_flash_disk(void)
{
    uint32_t current_page = 0xFFFFFFFFu;
    memset(s_page_buffer, 0, BlockDev_FLASH_PAGE_SIZE);

    for (uint32_t lba = 0; lba < s_flash_sectors; ++lba) {
        uint32_t addr = BlockDev_FLASH_BASE + lba * BlockDev_SECTOR_SIZE;
        uint32_t page = addr & ~(BlockDev_FLASH_PAGE_SIZE - 1u);
        uint32_t off = addr - page;

        if (page != current_page) {
            if (current_page != 0xFFFFFFFFu && flash_program_page(current_page, s_page_buffer)) return 1u;
            current_page = page;
            memset(s_page_buffer, 0, BlockDev_FLASH_PAGE_SIZE);
        }

        if (lba == 0u) {
            make_boot_sector(&s_page_buffer[off], "FLASHDISK", s_flash_sectors);
        } else if (lba == 1u) {
            s_page_buffer[off] = 0xF8;
            s_page_buffer[off + 1u] = 0xFF;
            s_page_buffer[off + 2u] = 0xFF;
        }
    }
    if (current_page != 0xFFFFFFFFu && flash_program_page(current_page, s_page_buffer)) return 1u;
    return 0u;
}

void BlockDev_Init(void)
{
    if (s_initialized) return;
    s_cache_valid = s_cache_dirty = s_flash_error = s_ram_error = 0u;
    s_flash_sectors = compute_flash_sectors();
    if (!s_flash_sectors) s_flash_error = 1u;
    s_ram_sectors = compute_ram_sectors();
    if (s_ram_sectors) {
        s_ram_disk = (uint8_t *)malloc(s_ram_sectors * BlockDev_SECTOR_SIZE);
        if (!s_ram_disk) { s_ram_sectors = 0u; s_ram_error = 1u; }
    } else s_ram_error = 1u;
    if (s_ram_disk && s_ram_sectors >= 2u) {
        memset(s_ram_disk, 0, s_ram_sectors * BlockDev_SECTOR_SIZE);
        make_boot_sector(s_ram_disk, "RAMDISK", s_ram_sectors);
        s_ram_disk[BlockDev_SECTOR_SIZE] = 0xF8;
        s_ram_disk[BlockDev_SECTOR_SIZE + 1u] = 0xFF;
        s_ram_disk[BlockDev_SECTOR_SIZE + 2u] = 0xFF;
    }
    if (!s_flash_error && flash_is_blank() && format_flash_disk()) s_flash_error = 1u;
    s_initialized = 1u;
}

uint32_t BlockDev_GetRamSectorCount(void) { return s_ram_sectors; }
uint32_t BlockDev_GetFlashSectorCount(void) { return s_flash_sectors; }

uint8_t BlockDev_IsReady(uint8_t lun)
{
    if (lun > BlockDev_MAX_LUN) return 1u;
    return (lun == 0u) ? s_ram_error : s_flash_error;
}

void BlockDev_Process(void) { flash_cache_commit(); }

uint8_t BlockDev_Read(uint8_t lun, uint32_t lba, uint8_t *buf, uint16_t blocks)
{
    uint32_t limit = (lun == 0u) ? s_ram_sectors : (lun == 1u) ? s_flash_sectors : 0u;
    if (!buf || !blocks || lba >= limit || (uint32_t)blocks > limit - lba || (lun == 1u && s_flash_error)) return 1u;
    for (uint16_t i = 0; i < blocks; ++i) {
        uint8_t *dst = &buf[i * BlockDev_SECTOR_SIZE];
        if (lun == 0u) {
            memcpy(dst, &s_ram_disk[(lba + i) * BlockDev_SECTOR_SIZE], BlockDev_SECTOR_SIZE);
        } else {
            uint32_t addr = BlockDev_FLASH_BASE + (lba + i) * BlockDev_SECTOR_SIZE;
            uint32_t page = addr & ~(BlockDev_FLASH_PAGE_SIZE - 1u);
            uint32_t off = addr - page;
            if (s_cache_valid && s_cache_dirty && page == s_cache_page)
                memcpy(dst, &s_page_buffer[off], BlockDev_SECTOR_SIZE);
            else
                memcpy(dst, (const void *)(uintptr_t)addr, BlockDev_SECTOR_SIZE);
        }
    }
    return 0u;
}

uint8_t BlockDev_Write(uint8_t lun, uint32_t lba, const uint8_t *buf, uint16_t blocks)
{
    uint32_t limit = (lun == 0u) ? s_ram_sectors : (lun == 1u) ? s_flash_sectors : 0u;
    if (!buf || !blocks || lba >= limit || (uint32_t)blocks > limit - lba || (lun == 1u && s_flash_error)) return 1u;
    if (lun == 1u && blocks > 1u) {
        uint32_t p0 = (BlockDev_FLASH_BASE + lba * BlockDev_SECTOR_SIZE) & ~(BlockDev_FLASH_PAGE_SIZE - 1u);
        uint32_t p1 = (BlockDev_FLASH_BASE + (lba + blocks - 1u) * BlockDev_SECTOR_SIZE) & ~(BlockDev_FLASH_PAGE_SIZE - 1u);
        if (p0 != p1) return 1u;
    }
    for (uint16_t i = 0; i < blocks; ++i) {
        const uint8_t *src = &buf[i * BlockDev_SECTOR_SIZE];
        if (lun == 0u) {
            memcpy(&s_ram_disk[(lba + i) * BlockDev_SECTOR_SIZE], src, BlockDev_SECTOR_SIZE);
        } else {
            uint32_t addr = BlockDev_FLASH_BASE + (lba + i) * BlockDev_SECTOR_SIZE;
            uint32_t page = addr & ~(BlockDev_FLASH_PAGE_SIZE - 1u);
            uint32_t off = addr - page;
            if (s_cache_valid && s_cache_dirty && page != s_cache_page) return 1u;
            if (!s_cache_valid || page != s_cache_page) {
                memcpy(s_page_buffer, (const void *)(uintptr_t)page, BlockDev_FLASH_PAGE_SIZE);
                s_cache_page = page;
                s_cache_valid = 1u;
            }
            memcpy(&s_page_buffer[off], src, BlockDev_SECTOR_SIZE);
            s_cache_dirty = 1u;
        }
    }
    return 0u;
}
