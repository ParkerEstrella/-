#include "spi_flash_driver.h"

/* SPI1 引脚: SCK=PB13, MISO=PB14, MOSI=PB15, CS=PB12 */
#define FLASH_SPI        SPI1
#define FLASH_SPI_RCU    RCU_SPI1
#define FLASH_CS_PORT    GPIOB
#define FLASH_CS_PIN     GPIO_PIN_12
#define FLASH_SCK_PORT   GPIOB
#define FLASH_SCK_PIN    GPIO_PIN_13
#define FLASH_MISO_PORT  GPIOB
#define FLASH_MISO_PIN   GPIO_PIN_14
#define FLASH_MOSI_PORT  GPIOB
#define FLASH_MOSI_PIN   GPIO_PIN_15
#define FLASH_GPIO_RCU   RCU_GPIOB

#define CMD_WREN         0x06
#define CMD_WRDI         0x04
#define CMD_RDSR         0x05
#define CMD_READ         0x03
#define CMD_WRITE        0x02
#define CMD_SECTOR_ERASE 0x20
#define CMD_BULK_ERASE   0xC7
#define CMD_RDID         0x9F

#define CS_LOW()   gpio_bit_reset(FLASH_CS_PORT, FLASH_CS_PIN)
#define CS_HIGH()  gpio_bit_set(FLASH_CS_PORT, FLASH_CS_PIN)

#define SPI_TIMEOUT_TICKS  100000

static RetStatus spi_xfer_byte(u8 tx_data, u8* rx_data)
{
    u32 timeout;

    timeout = SPI_TIMEOUT_TICKS;
    while (spi_i2s_flag_get(FLASH_SPI, SPI_FLAG_TBE) == RESET) {
        if (--timeout == 0) return RET_TIMEOUT;
    }
    spi_i2s_data_transmit(FLASH_SPI, tx_data);

    timeout = SPI_TIMEOUT_TICKS;
    while (spi_i2s_flag_get(FLASH_SPI, SPI_FLAG_RBNE) == RESET) {
        if (--timeout == 0) return RET_TIMEOUT;
    }
    if (rx_data) *rx_data = (u8)spi_i2s_data_receive(FLASH_SPI);
    return RET_OK;
}

static u8 spi_xfer_byte_blocking(u8 tx_data)
{
    u8 rx;
    if (spi_xfer_byte(tx_data, &rx) != RET_OK) return 0xFF;
    return rx;
}

static void flash_write_enable(void)
{
    CS_LOW();
    spi_xfer_byte_blocking(CMD_WREN);
    CS_HIGH();
}

static RetStatus flash_wait_ready(void)
{
    u32 timeout = 100000;

    CS_LOW();
    spi_xfer_byte_blocking(CMD_RDSR);
    u8 status;
    do {
        u8 rx;
        if (spi_xfer_byte(0xFF, &rx) != RET_OK) {
            CS_HIGH();
            return RET_TIMEOUT;
        }
        status = rx;
        if (--timeout == 0) {
            CS_HIGH();
            return RET_TIMEOUT;
        }
    } while (status & 0x01);
    CS_HIGH();
    return RET_OK;
}

void spi_flash_hw_init(void)
{
    rcu_periph_clock_enable(FLASH_GPIO_RCU);
    rcu_periph_clock_enable(FLASH_SPI_RCU);

    /* CS */
    gpio_mode_set(FLASH_CS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, FLASH_CS_PIN);
    gpio_output_options_set(FLASH_CS_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, FLASH_CS_PIN);
    CS_HIGH();

    /* SCK / MISO / MOSI */
    gpio_af_set(FLASH_SCK_PORT, GPIO_AF_5, FLASH_SCK_PIN);
    gpio_mode_set(FLASH_SCK_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, FLASH_SCK_PIN);
    gpio_output_options_set(FLASH_SCK_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, FLASH_SCK_PIN);

    gpio_af_set(FLASH_MISO_PORT, GPIO_AF_5, FLASH_MISO_PIN);
    gpio_mode_set(FLASH_MISO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, FLASH_MISO_PIN);
    gpio_output_options_set(FLASH_MISO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, FLASH_MISO_PIN);

    gpio_af_set(FLASH_MOSI_PORT, GPIO_AF_5, FLASH_MOSI_PIN);
    gpio_mode_set(FLASH_MOSI_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, FLASH_MOSI_PIN);
    gpio_output_options_set(FLASH_MOSI_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, FLASH_MOSI_PIN);

    /* SPI 配置 */
    spi_parameter_struct spi_cfg;
    spi_struct_para_init(&spi_cfg);
    spi_cfg.trans_mode           = SPI_TRANSMODE_FULLDUPLEX;
    spi_cfg.device_mode          = SPI_MASTER;
    spi_cfg.frame_size           = SPI_FRAMESIZE_8BIT;
    spi_cfg.clock_polarity_phase = SPI_CK_PL_LOW_PH_1EDGE;
    spi_cfg.nss                  = SPI_NSS_SOFT;
    spi_cfg.prescale             = SPI_PSC_8;
    spi_cfg.endian               = SPI_ENDIAN_MSB;
    spi_init(FLASH_SPI, &spi_cfg);
    spi_enable(FLASH_SPI);
}

u32 spi_flash_read_chip_id(void)
{
    CS_LOW();
    spi_xfer_byte_blocking(CMD_RDID);
    u32 id = ((u32)spi_xfer_byte_blocking(0xFF) << 16) |
             ((u32)spi_xfer_byte_blocking(0xFF) << 8)  |
             ((u32)spi_xfer_byte_blocking(0xFF));
    CS_HIGH();
    return id;
}

void spi_flash_erase_sector(u32 address)
{
    flash_write_enable();
    if (flash_wait_ready() != RET_OK) return;
    CS_LOW();
    spi_xfer_byte_blocking(CMD_SECTOR_ERASE);
    spi_xfer_byte_blocking((u8)(address >> 16));
    spi_xfer_byte_blocking((u8)(address >> 8));
    spi_xfer_byte_blocking((u8)(address));
    CS_HIGH();
    flash_wait_ready();
}

void spi_flash_erase_bulk(void)
{
    flash_write_enable();
    CS_LOW();
    spi_xfer_byte_blocking(CMD_BULK_ERASE);
    CS_HIGH();
    flash_wait_ready();
}

void spi_flash_write(u32 address, const u8* data, u32 length)
{
    if (data == NULL || length == 0) return;

    u32 remaining = length;
    u32 offset    = 0;

    while (remaining > 0) {
        u32 space_in_page = SPI_FLASH_PAGE_SZ - (address % SPI_FLASH_PAGE_SZ);
        u32 chunk = (remaining < space_in_page) ? remaining : space_in_page;

        flash_write_enable();
        if (flash_wait_ready() != RET_OK) return;

        CS_LOW();
        spi_xfer_byte_blocking(CMD_WRITE);
        spi_xfer_byte_blocking((u8)(address >> 16));
        spi_xfer_byte_blocking((u8)(address >> 8));
        spi_xfer_byte_blocking((u8)(address));
        for (u32 i = 0; i < chunk; i++) {
            spi_xfer_byte_blocking(data[offset + i]);
        }
        CS_HIGH();
        if (flash_wait_ready() != RET_OK) return;

        offset    += chunk;
        address   += chunk;
        remaining -= chunk;
    }
}

void spi_flash_read(u32 address, u8* buffer, u32 length)
{
    if (buffer == NULL || length == 0) return;

    CS_LOW();
    spi_xfer_byte_blocking(CMD_READ);
    spi_xfer_byte_blocking((u8)(address >> 16));
    spi_xfer_byte_blocking((u8)(address >> 8));
    spi_xfer_byte_blocking((u8)(address));
    for (u32 i = 0; i < length; i++) {
        buffer[i] = spi_xfer_byte_blocking(0xFF);
    }
    CS_HIGH();
}
