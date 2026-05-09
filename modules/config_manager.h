#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "common_types.h"

void      cfgmgr_load_defaults(AppConfig* cfg);
void      cfgmgr_save_to_flash(const AppConfig* cfg);
RetStatus cfgmgr_load_from_flash(AppConfig* cfg);
RetStatus cfgmgr_load_from_tf_card(AppConfig* cfg, const char* filepath);
void      cfgmgr_apply(const AppConfig* cfg);

/* 获取当前运行时配置 */
const AppConfig* cfgmgr_get_current(void);

#endif /* CONFIG_MANAGER_H */
