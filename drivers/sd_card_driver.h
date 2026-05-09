#ifndef SD_CARD_DRIVER_H
#define SD_CARD_DRIVER_H

#include "common_types.h"

/* FatFs 集成接口 */
#include "ff.h"
#include "diskio.h"

RetStatus sd_card_hw_init(void);
bool      sd_card_is_mounted(void);
FATFS*    sd_get_filesystem(void);
void      sd_card_get_capacity(u32* total_kb, u32* free_kb);

#endif /* SD_CARD_DRIVER_H */
