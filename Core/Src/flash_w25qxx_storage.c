/* Copyright © 2023 Georgy E. All rights reserved. */

#include <flash_w25qxx_storage.h>

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "stm32f4xx_hal.h"

#include "utils.h"
#include "main.h"


typedef enum _flash_w25_command_t {
    FLASH_W25_CMD_WRITE_SR1       = ((uint8_t)0x01),
    FLASH_W25_CMD_PAGE_PROGRAMM   = ((uint8_t)0x02),
    FLASH_W25_CMD_READ            = ((uint8_t)0x03),
    FLASH_W25_CMD_WRITE_DISABLE   = ((uint8_t)0x04),
    FLASH_W25_CMD_READ_SR1        = ((uint8_t)0x05),
    FLASH_W25_CMD_WRITE_ENABLE    = ((uint8_t)0x06),
    FLASH_W25_CMD_ERASE_SECTOR    = ((uint8_t)0x20),
    FLASH_W25_CMD_WRITE_ENABLE_SR = ((uint8_t)0x50),
    FLASH_W25_CMD_ENABLE_RESET    = ((uint8_t)0x66),
    FLASH_W25_CMD_RESET           = ((uint8_t)0x99),
    FLASH_W25_CMD_JEDEC_ID        = ((uint8_t)0x9f)
} flash_w25_command_t;

#define FLASH_W25_JEDEC_ID_SIZE       (sizeof(uint32_t))
#define FLASH_W25_SR1_BUSY            ((uint8_t)0b00000001)
#define FLASH_W25_SR1_WEL             ((uint8_t)0b00000010)
#define FLASH_W25_24BIT_ADDR_SIZE     ((uint16_t)512)
#define FLASH_W25_SR1_UNBLOCK_VALUE   ((uint8_t)0x00)
#define FLASH_W25_SR1_BLOCK_VALUE     ((uint8_t)0x0F)

#define FLASH_SPI_TIMEOUT_MS          ((uint32_t)10000)
#define FLASH_SPI_COMMAND_SIZE_MAX    ((uint8_t)10)


flash_status_t _flash_read_jdec_id(uint32_t* jdec_id);
flash_status_t _flash_read_SR1(uint8_t* SR1);
flash_status_t _flash_write_enable();
flash_status_t _flash_write_disable();
flash_status_t _flash_erase_sector(uint32_t addr);
flash_status_t _flash_set_protect_block(uint8_t value);

flash_status_t _flash_send_data(uint8_t* data, uint16_t len);
flash_status_t _flash_recieve_data(uint8_t* data, uint16_t len);
void           _flash_spi_cs_set();
void           _flash_spi_cs_reset();

bool           _flash_check_FREE();
bool           _flash_check_WEL();

uint32_t       _flash_get_storage_bytes_size();


#ifdef DEBUG
const char* FLASH_TAG = "FLSH";
#endif


#define FLASH_W25_JDEC_ID_BLOCK_COUNT_MASK ((uint16_t)0x4011)
const uint16_t w25qxx_jdec_id_block_count[] = {
    2,   // w25q10
    4,   // w25q20
    8,   // w25q40
    16,  // w25q80
    32,  // w25q16
    64,  // w25q32
    128, // w25q64
    256, // w25q128
    512, // w25q256
    1024 // w25q512
};


flash_w25qxx_info_t flash_w25qxx_info = {
    .initialized      = false,
    .is_24bit_address = false,

    .page_size        = FLASH_W25_PAGE_SIZE,
    .pages_count      = FLASH_W25_SECTOR_SIZE / FLASH_W25_PAGE_SIZE,

    .sector_size      = FLASH_W25_SECTOR_SIZE,
    .sectors_count    = FLASH_W25_SECTORS_COUNT,

    .block_size       = FLASH_W25_SECTORS_COUNT * FLASH_W25_SECTOR_SIZE,
    .blocks_count     = 0
};


flash_status_t flash_w25qxx_init()
{
    _flash_spi_cs_reset();

    uint32_t jdec_id = 0;
    flash_status_t status = _flash_read_jdec_id(&jdec_id);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "flash init error=%02x (read JDEC ID)", status);
#endif
        return status;
    }

    flash_w25qxx_info.blocks_count = 0;
    uint16_t jdec_id_2b = (uint16_t)jdec_id;
    for (uint16_t i = 0; i < __sizeof_arr(w25qxx_jdec_id_block_count); i++) {
        if ((uint16_t)(FLASH_W25_JDEC_ID_BLOCK_COUNT_MASK + i) == jdec_id_2b) {
            flash_w25qxx_info.blocks_count = w25qxx_jdec_id_block_count[i];
            break;
        }
    }

    if (!flash_w25qxx_info.blocks_count) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "flash init error - unknown JDEC ID");
#endif
        return FLASH_ERROR;
    }


#if FLASH_BEDUG
    LOG_TAG_BEDUG(FLASH_TAG, "flash JDEC ID found: id=%08x, page_count=%lu", jdec_id, flash_w25qxx_info.pages_count);
#endif

    status = _flash_set_protect_block(FLASH_W25_SR1_BLOCK_VALUE);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "flash init error=%02x (block FLASH error)", status);
#endif
    	return status;
    }

    flash_w25qxx_info.initialized      = true;
    flash_w25qxx_info.is_24bit_address = (flash_w25qxx_info.blocks_count >= FLASH_W25_24BIT_ADDR_SIZE) ? true : false;

    return FLASH_OK;
}

flash_status_t flash_w25qxx_reset()
{
    flash_status_t status = _flash_set_protect_block(FLASH_W25_SR1_UNBLOCK_VALUE);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "flash reset error=%02x (unset block protect)", status);
#endif
        status = FLASH_BUSY;
        goto do_block_protect;
    }

    _flash_spi_cs_set();

    uint8_t spi_cmd[] = { FLASH_W25_CMD_ENABLE_RESET, FLASH_W25_CMD_RESET };

    if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "flash reset error (FLASH busy)");
#endif
        return FLASH_BUSY;
    }

    status = _flash_send_data(spi_cmd, sizeof(spi_cmd));
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "flash reset error=%02x (send command)", status);
#endif
        status = FLASH_BUSY;
    }

    if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "flash reset error (flash is busy)");
#endif
        return FLASH_BUSY;
    }

    _flash_spi_cs_reset();


    flash_status_t tmp_status = FLASH_OK;
do_block_protect:
    tmp_status = _flash_set_protect_block(FLASH_W25_SR1_BLOCK_VALUE);
    if (status == FLASH_OK) {
        status = tmp_status;
    } else {
        return status;
    }

    if (status != FLASH_OK) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "flash reset error=%02x (set block protected)", status);
#endif
        status = FLASH_BUSY;
    }

    return status;
}

flash_status_t flash_w25qxx_read(uint32_t addr, uint8_t* data, uint32_t len)
{
    flash_status_t status = FLASH_OK;
    if (!flash_w25qxx_info.initialized) {
        status = flash_w25qxx_init();
    }

    if (status != FLASH_OK) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "flash read addr=%lu error=%02x (flash was not initialized)", addr, status);
#endif
        return status;
    }

    if (addr + len > _flash_get_storage_bytes_size()) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "flash read addr=%lu error (unacceptable address)", addr);
#endif
        return FLASH_OOM;
    }

    _flash_spi_cs_set();


    uint8_t spi_cmd[FLASH_SPI_COMMAND_SIZE_MAX] = { 0 };
    uint8_t counter = 0;
    spi_cmd[counter++] = FLASH_W25_CMD_READ;
    if (flash_w25qxx_info.is_24bit_address) {
        spi_cmd[counter++] = (addr >> 24) & 0xFF;
    }
    spi_cmd[counter++] = (addr >> 16) & 0xFF;
    spi_cmd[counter++] = (addr >> 8) & 0xFF;
    spi_cmd[counter++] = addr & 0xFF;

    if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "flash read addr=%lu error (FLASH busy)", addr);
#endif
        status = FLASH_BUSY;
        goto do_spi_stop;
    }

    status = _flash_send_data(spi_cmd, counter);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "flash read addr=%lu error=%02x (send command)", addr, status);
#endif
        goto do_spi_stop;
    }

    status = _flash_recieve_data(data, len);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "flash read addr=%lu error=%02x (recieve data)", addr, status);
#endif
        goto do_spi_stop;
    }

do_spi_stop:
    _flash_spi_cs_reset();

    return status;
}

flash_status_t flash_w25qxx_write(uint32_t addr, uint8_t* data, uint32_t len)
{
    flash_status_t status = FLASH_OK;
    if (!flash_w25qxx_info.initialized) {
        status = flash_w25qxx_init();
    }

    if (status != FLASH_OK) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "flash write addr=%lu error=%02x (flash was not initialized)", addr, status);
#endif
        return status;
    }

    if (addr + len > _flash_get_storage_bytes_size()) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "flash write addr=%lu error (unacceptable address)", addr);
#endif
        return FLASH_OOM;
    }

    if (len > flash_w25qxx_info.page_size) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "flash write addr=%lu error (unacceptable data length)", addr);
#endif
        return FLASH_ERROR;
    }

    uint32_t sector_addr = (addr / FLASH_W25_SECTOR_SIZE) * FLASH_W25_SECTOR_SIZE;
    uint8_t  sector_tmp_buf[FLASH_W25_SECTOR_SIZE] = { 0 };
    memset(sector_tmp_buf, 0xFF, sizeof(sector_tmp_buf));

    status = flash_w25qxx_read(sector_addr, sector_tmp_buf, sizeof(sector_tmp_buf));
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        uint32_t block_num = sector_addr / flash_w25qxx_info.block_size;
        uint32_t sector_num = (sector_addr % flash_w25qxx_info.block_size) / flash_w25qxx_info.sector_size;
        LOG_TAG_BEDUG(FLASH_TAG, "flash write addr=%lu error (unable to read block_addr=%lu sector_addr=%lu)", addr, block_num, sector_num);
#endif
        return status;
    }

    uint8_t cmpr_buf[FLASH_W25_PAGE_SIZE] = { 0 };
    uint8_t read_buf[FLASH_W25_PAGE_SIZE] = { 0 };
    memset(cmpr_buf, 0xFF, sizeof(cmpr_buf));
    memset(read_buf, 0xFF, sizeof(read_buf));

    status = flash_w25qxx_read(addr, read_buf, len);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "flash write addr=%lu error (read target page)", addr);
#endif
        return status;
    }

    if (!memcmp(read_buf, data, len)) {
        return FLASH_OK;
    }

    bool need_to_write_sector = false;
    if (memcmp(cmpr_buf, read_buf, flash_w25qxx_info.page_size)) {
        status = _flash_erase_sector(addr);
        need_to_write_sector = true;
    }

    if (status != FLASH_OK) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "flash write addr=%lu error=%02x (erase sector)", addr, status);
#endif
        return FLASH_BUSY;
    }

    uint8_t read_sector_buf[FLASH_W25_SECTOR_SIZE] = { 0 };
    memset(read_sector_buf, 0xFF, sizeof(read_sector_buf));

    if (need_to_write_sector) {
        status = flash_w25qxx_read(sector_addr, read_sector_buf, sizeof(read_sector_buf));
    }

    if (status != FLASH_OK) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "flash write addr=%lu error=%02x (read target sector)", sector_addr, status);
#endif
        return FLASH_BUSY;
    }

    if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "flash write addr=%lu error (flash is busy)", addr);
#endif
        return FLASH_BUSY;
    }

    status = _flash_set_protect_block(FLASH_W25_SR1_UNBLOCK_VALUE);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "flash write addr=%lu error=%02x (unset block protect)", addr, status);
#endif
        goto do_block_protect;
    }

    status = _flash_write_enable();
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "flash write addr=%lu error=%02x (write enable)", addr, status);
#endif
        goto do_block_protect;
    }

    if (!util_wait_event(_flash_check_WEL, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "flash write addr=%lu error=%02x (WEL bit wait time exceeded)", addr, status);
#endif
        status = FLASH_BUSY;
        goto do_spi_stop;
    }

    _flash_spi_cs_set();

    uint8_t spi_cmd[FLASH_SPI_COMMAND_SIZE_MAX] = { 0 };
    uint8_t counter = 0;
    spi_cmd[counter++] = FLASH_W25_CMD_PAGE_PROGRAMM;
    if (flash_w25qxx_info.is_24bit_address) {
        spi_cmd[counter++] = (addr >> 24) & 0xFF;
    }
    spi_cmd[counter++] = (addr >> 16) & 0xFF;
    spi_cmd[counter++] = (addr >> 8) & 0xFF;
    spi_cmd[counter++] = addr & 0xFF;

    if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "flash write addr=%lu error (FLASH busy)", addr);
#endif
        goto do_spi_stop;
    }

    status = _flash_send_data(spi_cmd, counter);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "flash write addr=%lu error=%02x (send command)", addr, status);
#endif
        goto do_spi_stop;
    }

    if (need_to_write_sector) {
        uint8_t empty_page[FLASH_W25_PAGE_SIZE] = { 0 };
        memset(empty_page, 0xFF, sizeof(empty_page));
        memcpy(&read_sector_buf[addr % flash_w25qxx_info.sector_size], empty_page, sizeof(empty_page));
        memcpy(&read_sector_buf[addr % flash_w25qxx_info.sector_size], data, len);
        status = _flash_send_data(read_buf, sizeof(read_buf));
    } else {
        status = _flash_send_data(data, len);
    }

    if (status != FLASH_OK) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "flash write addr=%lu error=%02x (wait write data timeout)", addr, status);
#endif
        goto do_spi_stop;
    }

    if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "flash write addr=%lu error (flash is busy)", addr);
#endif
        return FLASH_BUSY;
    }

    status = _flash_write_disable();
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "flash write addr=%lu error=%02x (write is not disabled)", addr, status);
#endif
        goto do_spi_stop;
    }

    flash_status_t tmp_status = FLASH_OK;
    uint8_t read_page_buf[FLASH_W25_PAGE_SIZE] = { 0 };

do_spi_stop:
    _flash_spi_cs_reset();

do_block_protect:
    tmp_status = _flash_set_protect_block(FLASH_W25_SR1_BLOCK_VALUE);
    if (status == FLASH_OK) {
        status = tmp_status;
    } else {
        return status;
    }

    if (status != FLASH_OK) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "flash write addr=%lu error=%02x (set block protected)", addr, status);
#endif
        return FLASH_BUSY;
    }

    status = flash_w25qxx_read(addr, read_page_buf, sizeof(read_page_buf));
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "flash write addr=%lu error=%02x (read wrtitten page)", addr, status);
#endif
        return status;
    }

    if (memcmp(read_page_buf, data, len)) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "flash write addr=%lu error(check wrtitten page)", addr);
#endif
        return FLASH_ERROR;
    }

    return status;
}

uint32_t flash_w25qxx_get_pages_count()
{
    flash_status_t status = FLASH_OK;
    if (!flash_w25qxx_info.initialized) {
        status = flash_w25qxx_init();
    }
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "get pages count: initializing error");
#endif
        return 0;
    }
    return flash_w25qxx_info.pages_count * flash_w25qxx_info.sectors_count * flash_w25qxx_info.blocks_count;
}

flash_status_t _flash_read_jdec_id(uint32_t* jdec_id)
{
    bool cs_selected = (HAL_GPIO_ReadPin(FLASH_SPI_CS_GPIO_Port, FLASH_SPI_CS_Pin) == GPIO_PIN_RESET);
    if (!cs_selected) {
        _flash_spi_cs_set();
    }

    if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "get JEDEC ID error (FLASH busy)");
#endif
        goto do_spi_stop;
    }

    uint8_t spi_cmd[] = { FLASH_W25_CMD_JEDEC_ID };
    flash_status_t status = _flash_send_data(spi_cmd, sizeof(spi_cmd));
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "get JDEC ID error=%02x (send command)", status);
#endif
        goto do_spi_stop;
    }

    uint8_t data[FLASH_W25_JEDEC_ID_SIZE] = { 0 };
    status = _flash_recieve_data(data, sizeof(data));
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "get JDEC ID error=%02x (recieve data)", status);
#endif
        goto do_spi_stop;
    }

    *jdec_id = ((((uint32_t)data[0]) << 16) | (((uint32_t)data[1]) << 8) | ((uint32_t)data[2]));

do_spi_stop:
    if (!cs_selected) {
        _flash_spi_cs_reset();
    }

    return status;
}

flash_status_t _flash_read_SR1(uint8_t* SR1)
{
    bool cs_selected = (HAL_GPIO_ReadPin(FLASH_SPI_CS_GPIO_Port, FLASH_SPI_CS_Pin) == GPIO_PIN_RESET);
    if (!cs_selected) {
        _flash_spi_cs_set();
    }

    uint8_t spi_cmd[] = { FLASH_W25_CMD_READ_SR1 };

    HAL_StatusTypeDef status = HAL_SPI_Transmit(&FLASH_SPI, spi_cmd, sizeof(spi_cmd), FLASH_SPI_TIMEOUT_MS);
    if (status == HAL_BUSY) {
       return FLASH_BUSY;
    }

    if (status != HAL_OK) {
       return FLASH_ERROR;
    }

    status = HAL_SPI_Receive(&FLASH_SPI, SR1, sizeof(uint8_t), FLASH_SPI_TIMEOUT_MS);
    if (status == HAL_BUSY) {
        return FLASH_BUSY;
    }

    if (status != HAL_OK) {
        return FLASH_ERROR;
    }

    _flash_spi_cs_reset();

    if (cs_selected) {
        _flash_spi_cs_set();
    }

    return FLASH_OK;
}

flash_status_t _flash_write_enable()
{
    if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "write enable error (FLASH busy)");
#endif
        return FLASH_BUSY;
    }

    _flash_spi_cs_set();

    uint8_t spi_cmd[] = { FLASH_W25_CMD_WRITE_ENABLE };
    flash_status_t status = _flash_send_data(spi_cmd, sizeof(spi_cmd));
#if FLASH_BEDUG
    if (status != FLASH_OK) {
        LOG_TAG_BEDUG(FLASH_TAG, "write enable error=%02x", status);
    }
#endif

    _flash_spi_cs_reset();

    return status;
}

flash_status_t _flash_write_disable()
{
    if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "write disable error (FLASH busy)");
#endif
        return FLASH_BUSY;
    }

    _flash_spi_cs_set();

    uint8_t spi_cmd[] = { FLASH_W25_CMD_WRITE_DISABLE };
    flash_status_t status = _flash_send_data(spi_cmd, sizeof(spi_cmd));
#if FLASH_BEDUG
    if (status != FLASH_OK) {
        LOG_TAG_BEDUG(FLASH_TAG, "write disable error=%02x", status);
    }
#endif

    _flash_spi_cs_reset();

    return status;
}

flash_status_t _flash_erase_sector(uint32_t addr) {
    if (addr % flash_w25qxx_info.sector_size > 0) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "erase sector addr=%lu error (unacceptable address)", addr);
#endif
        return FLASH_ERROR;
    }

    if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "erase sector addr=%lu error (flash is busy)", addr);
#endif
        return FLASH_BUSY;
    }

    uint8_t spi_cmd[FLASH_SPI_COMMAND_SIZE_MAX] = { 0 };
    uint8_t counter = 0;
    spi_cmd[counter++] = FLASH_W25_CMD_ERASE_SECTOR;
    if (flash_w25qxx_info.is_24bit_address) {
        spi_cmd[counter++] = (addr >> 24) & 0xFF;
    }
    spi_cmd[counter++] = (addr >> 16) & 0xFF;
    spi_cmd[counter++] = (addr >> 8) & 0xFF;
    spi_cmd[counter++] = addr & 0xFF;

    flash_status_t status = _flash_set_protect_block(FLASH_W25_SR1_UNBLOCK_VALUE);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "erase sector addr=%lu error=%02x (unset block protect)", addr, status);
#endif
        goto do_spi_stop;
    }

    status = _flash_write_enable();
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "erase sector addr=%lu error=%02x (write is not enabled)", addr, status);
#endif
        goto do_spi_stop;
    }

    if (!util_wait_event(_flash_check_WEL, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "erase sector addr=%lu error=%02x (WEL bit wait time exceeded)", addr, status);
#endif
        status = FLASH_BUSY;
        goto do_spi_stop;
    }

    _flash_spi_cs_set();

    status = _flash_send_data(spi_cmd, counter);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "erase sector addr=%lu error=%02x (write is not enabled)", addr, status);
#endif
        goto do_spi_stop;
    }

    status = _flash_write_disable();
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "erase sector addr=%lu error=%02x (write is not disabled)", addr, status);
#endif
        goto do_spi_stop;
    }

do_spi_stop:
    _flash_spi_cs_reset();

    if (status != FLASH_OK) {
        goto do_block_protect;
    }

    uint8_t cmpr_buf[FLASH_W25_SECTOR_SIZE] = { 0 };
    uint8_t read_buf[FLASH_W25_SECTOR_SIZE] = { 0 };
    memset(cmpr_buf, 0xFF, sizeof(cmpr_buf));
    memset(read_buf, 0xFF, sizeof(read_buf));

    status = flash_w25qxx_read(addr, read_buf, flash_w25qxx_info.sector_size);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "erase sector addr=%lu error=%02x (check target sector)", addr, status);
#endif
        goto do_block_protect;
    }

    if (memcmp(cmpr_buf, read_buf, flash_w25qxx_info.sector_size)) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "erase sector addr=%lu error - sector is not erased", addr);
#endif
        status = FLASH_ERROR;
        goto do_block_protect;
    }

    flash_status_t tmp_status = FLASH_OK;
do_block_protect:
    tmp_status = _flash_set_protect_block(FLASH_W25_SR1_BLOCK_VALUE);
    if (status == FLASH_OK) {
        status = tmp_status;
    } else {
        return status;
    }

    if (status != FLASH_OK) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "erase sector addr=%lu error=%02x (set block protected)", addr, status);
#endif
        status = FLASH_BUSY;
    }

    return status;
}

flash_status_t _flash_set_protect_block(uint8_t value)
{
    if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "set protect block value=%02x error (FLASH busy)", value);
#endif
        return FLASH_BUSY;
    }

    uint8_t spi_cmd_01[] = { FLASH_W25_CMD_WRITE_ENABLE_SR };

    _flash_spi_cs_set();

    flash_status_t status = _flash_send_data(spi_cmd_01, sizeof(spi_cmd_01));
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "set protect block value=%02x error=%02x (enable write SR1)", value, status);
#endif
        goto do_spi_stop;
    }

    _flash_spi_cs_reset();


    uint8_t spi_cmd_02[] = { FLASH_W25_CMD_WRITE_SR1, ((value & 0x0F) << 2) };

    _flash_spi_cs_set();

    status = _flash_send_data(spi_cmd_02, sizeof(spi_cmd_02));
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        LOG_TAG_BEDUG(FLASH_TAG, "set protect block value=%02x error=%02x (write SR1)", value, status);
#endif
        goto do_spi_stop;
    }

    goto do_spi_stop;

do_spi_stop:
    _flash_spi_cs_reset();

    return status;
}


flash_status_t _flash_send_data(uint8_t* data, uint16_t len)
{
    HAL_StatusTypeDef status = HAL_SPI_Transmit(&FLASH_SPI, data, len, FLASH_SPI_TIMEOUT_MS);
    if (status == HAL_BUSY) {
        return FLASH_BUSY;
    }
    if (status != HAL_OK) {
        return FLASH_ERROR;
    }
    return FLASH_OK;
}

flash_status_t _flash_recieve_data(uint8_t* data, uint16_t len)
{
    HAL_StatusTypeDef status = HAL_SPI_Receive(&FLASH_SPI, data, len, FLASH_SPI_TIMEOUT_MS);
    if (status == HAL_BUSY) {
        return FLASH_BUSY;
    }
    if (status != HAL_OK) {
        return FLASH_ERROR;
    }
    return FLASH_OK;
}

void _flash_spi_cs_set()
{
    HAL_GPIO_WritePin(FLASH_SPI_CS_GPIO_Port, FLASH_SPI_CS_Pin, GPIO_PIN_RESET);
}

void _flash_spi_cs_reset()
{
    HAL_GPIO_WritePin(FLASH_SPI_CS_GPIO_Port, FLASH_SPI_CS_Pin, GPIO_PIN_SET);
}

bool _flash_check_FREE()
{
    uint8_t SR1 = 0x00;
    flash_status_t status = _flash_read_SR1(&SR1);
    if (status != FLASH_OK) {
        return false;
    }

    return !(SR1 & FLASH_W25_SR1_BUSY);
}

bool _flash_check_WEL()
{
    uint8_t SR1 = 0x00;
    flash_status_t status = _flash_read_SR1(&SR1);
    if (status != FLASH_OK) {
        return false;
    }

    return SR1 & FLASH_W25_SR1_WEL;
}

uint32_t _flash_get_storage_bytes_size()
{
    return flash_w25qxx_info.blocks_count * flash_w25qxx_info.block_size;
}
