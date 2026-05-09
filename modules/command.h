#ifndef COMMAND_H
#define COMMAND_H

#include "common_types.h"

#define CMD_MAX_LEN 64
#define CMD_ARG_MAX 8

/* 命令处理函数类型 */
typedef RetStatus (*CmdHandler)(const char* args[], u8 arg_count);

/* 命令表项 */
typedef struct {
    const char* name;
    const char* description;
    CmdHandler  handler;
    u8          min_args;
    u8          max_args;
} CmdEntry;

/* 命令解析器状态 */
typedef struct {
    char buffer[CMD_MAX_LEN];
    u16  buf_idx;
    bool cmd_ready;
} CmdParser;

/* 命令路由表（外部可见） */
extern const CmdEntry cmd_table[];
extern const u8 cmd_table_size;

/* 解析器 API */
void      cmd_parser_init(CmdParser* parser);
RetStatus cmd_parser_input_char(CmdParser* parser, char ch);
RetStatus cmd_execute(const char* cmd_str);

/* 辅助函数 */
bool cmd_match(const char* input, const char* cmd_name);
u8   cmd_split_args(char* buffer, char* args[], u8 max_args);

/* 交互式输入等待 */
bool cmd_is_waiting_for_input(void);
void cmd_process_pending_input(const char* input);

#endif /* COMMAND_H */
