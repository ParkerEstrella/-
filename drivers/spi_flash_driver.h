#ifndef SPI_FLASH_DRIVER_H
#define SPI_FLASH_DRIVER_H

#include "common_types.h"

#define SPI_FLASH_PAGE_SZ    256
#define SPI_FLASH_SECTOR_SZ  4096

void    spi_flash_hw_init(void);
u32     spi_flash_read_chip_id(void);
void    spi_flash_erase_sector(u32 address);
void    spi_flash_erase_bulk(void);
void    spi_flash_write(u32 address, const u8* data, u32 length);
void    spi_flash_read(u32 address, u8* buffer, u32 length);

#endif /* SPI_FLASH_DRIVER_H */
