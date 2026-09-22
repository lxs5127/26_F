#ifndef __BUTTON_H
#define __BUTTON_H

#include "ti_msp_dl_config.h"

/* ====== 任务模式枚举 ====== */
typedef enum {
    TASK_NONE = 0,          /* 未选择任务 (空闲)         */
    TASK_1,                 /* 任务一                   */
    TASK_2,                 /* 任务二                   */
    TASK_3,                 /* 任务三                   */
    TASK_4,                 /* 任务四                   */
    TASK_5,                 /* 任务五                   */
    TASK_6,                 /* 任务六                   */
    TASK_COUNT              /* 任务总数 (哨兵值，放最后) */
} TaskMode;

extern volatile TaskMode g_task;
extern volatile uint8_t  g_start_flag;   /* 1=任务运行中, 0=空闲 */

void read_button(void);
const char* task_name(TaskMode t);

#endif
