#include "task.h"
#include "controll.h"
#include "encoder.h"
#include "button.h"
#include "clock.h"
#include "oled_software_i2c.h"
#include "camera_uart.h"
#include "stepper.h"
#include "ball_ctrl.h"
#include <stdio.h>

/* ====== 任务计时器 ====== */
static uint32_t g_task_start_ms = 0;
static char     g_time_buf[16];

static void task_timer_start(void)
{
    g_task_start_ms = tick_ms;
}

static void task_timer_show(void)
{
    uint32_t elapsed = tick_ms - g_task_start_ms;
    uint32_t ms  = elapsed % 1000;
    uint32_t sec = (elapsed / 1000) % 60;
    uint32_t min = elapsed / 60000;

    if (g_start_flag) {
        sprintf(g_time_buf, "Time: %02lu:%02lu.%03lu",
                (unsigned long)min, (unsigned long)sec, (unsigned long)ms);
    } else {
        sprintf(g_time_buf, "End:  %02lu:%02lu.%03lu",
                (unsigned long)min, (unsigned long)sec, (unsigned long)ms);
    }
    OLED_ShowString(0, 7, (uint8_t *)g_time_buf, 8);
}

/* ====== 斜坡加减速工具函数 ====== */
#define ACCEL_STEP  0.005f      /* 每帧速度变化量 */
#define DECEL_DIST  8.0f        /* 开始减速距离 (cm) */
#define BRAKE_DIST  2.0f        /* 最后急刹距离 (cm) */

/* 斜坡逼近: cur 以 step 速率向 target 逼近，返回更新后的速度 */
static inline float ramp_move(float cur, float target, float step)
{
    if (cur < target) {
        cur += step;
        if (cur > target) cur = target;
    } else if (cur > target) {
        cur -= step;
        if (cur < target) cur = target;
    }
    return cur;
}

/* ====== 任务一： ====== */

void task_1(void)
{
        g_start_flag = 0;

}


#define TASK2_SPEED     0.31f
/* ====== 任务二：巡线 → 检测停车标志停下 ====== */
void task_2(void)
{
    static uint8_t running = 0;



    if (!g_start_flag) running = 0;

    if (!running) {
        task_timer_start();
        reset_position();           /* 里程计清零 → 赛道起点对齐 */
        set_target_speed(TASK2_SPEED);
        running = 1;
    }

    if (detect_stop()) {
        set_target_speed(0.0f);
        running = 0;
        g_start_flag = 0;
    }

    task_timer_show();
}



void task_3(void)
{
    
}


/* ====== 任务四：位置环行驶 1.5m（缓慢加速，过线后较快减速） ====== */
#define TASK4_SPEED         0.26f
#define TASK4_ACCEL_STEP    0.002f      /* 加速步进：越小加速越缓慢 */

void task_4(void)
{
    static uint8_t  running = 0;
    static float    cur_speed = 0.0f;
    static uint8_t  time_frozen = 0;
    static uint32_t frozen_ms = 0;

    if (!g_start_flag) { running = 0; cur_speed = 0.0f; time_frozen = 0; }

    if (!running) {
        task_timer_start();
        reset_position();
        cur_speed = 0.0f;
        time_frozen = 0;
        running = 1;
    }

    float dist = (pos_left + pos_right) * 0.5f;

    if (dist >= 200.0f) {//200
        /* 到1.8m：开始缓慢减速停车 */
        cur_speed = ramp_move(cur_speed, 0.0f, TASK4_ACCEL_STEP);
        set_target_speed(cur_speed);
        if (cur_speed == 0.0f) {
            middle_speed = 0;
            running = 0;
            g_start_flag = 0;
        }
    }
    else if (dist >= 150.0f) {
        /* 通过1.5m：冻结显示时间，继续行驶到1.8m */
        if (!time_frozen) {
            frozen_ms = tick_ms - g_task_start_ms;
            time_frozen = 1;
        }
        cur_speed = ramp_move(cur_speed, TASK4_SPEED, TASK4_ACCEL_STEP);
        set_target_speed(cur_speed);
    }
    else {
        /* 缓慢加速到目标速度，然后匀速 */
        cur_speed = ramp_move(cur_speed, TASK4_SPEED, TASK4_ACCEL_STEP);
        set_target_speed(cur_speed);
    }

    /* 显示：过线后显示冻结时间，否则实时显示 */
    if (time_frozen) {
        uint32_t ms  = frozen_ms % 1000;
        uint32_t sec = (frozen_ms / 1000) % 60;
        uint32_t min = frozen_ms / 60000;
        sprintf(g_time_buf, "End:  %02lu:%02lu.%03lu",
                (unsigned long)min, (unsigned long)sec, (unsigned long)ms);
        OLED_ShowString(0, 7, (uint8_t *)g_time_buf, 8);
    } else {
        task_timer_show();
    }

    CameraUART_SendCmd(4);
/*---------------------------------------------------------------------*/
}


/* ====== 任务五：巡线 → 检测停车标志后缓慢停车 ====== */
#define TASK5_SPEED         0.22f
#define TASK5_ACCEL_STEP    0.002f      /* 加速步进：越小加速越缓慢 */

void task_5(void)
{

    static uint8_t  running = 0;
    static float    cur_speed = 0.0f;
    static uint8_t  stopping  = 0;
    static uint8_t  time_frozen = 0;
    static uint32_t frozen_ms = 0;
    static float    stop_dist = 0.0f;

    if (!g_start_flag) { running = 0; cur_speed = 0.0f; stopping = 0; time_frozen = 0; }

    if (!running) {
        task_timer_start();
        reset_position();
        cur_speed = 0.0f;
        stopping = 0;
        time_frozen = 0;
        running = 1;
    }

    float dist = (pos_left + pos_right) * 0.5f;

    if (stopping) {
        if (stopping == 1) {
            /* 检测到停车标志后：再走50cm开始减速 */
            if (dist - stop_dist >= 50.0f) {
                stopping = 2;
            }
        }
        if (stopping == 2) {
            /* 缓慢减速停车 */
            cur_speed = ramp_move(cur_speed, 0.0f, TASK5_ACCEL_STEP);
            set_target_speed(cur_speed);
            if (cur_speed == 0.0f) {
                running = 0;
                g_start_flag = 0;
            }
        }
        else {
            cur_speed = ramp_move(cur_speed, TASK5_SPEED, TASK5_ACCEL_STEP);
            set_target_speed(cur_speed);
        }
    } else {
        cur_speed = ramp_move(cur_speed, TASK5_SPEED, TASK5_ACCEL_STEP);
        set_target_speed(cur_speed);
        if (detect_stop()) {
            /* 检测到停车标志：记录时间与位置 */
            frozen_ms = tick_ms - g_task_start_ms;
            stop_dist = dist;
            time_frozen = 1;
            stopping = 1;
        }
    }

    /* 显示：检测到标志后显示冻结时间，否则实时显示 */
    if (time_frozen) {
        uint32_t ms  = frozen_ms % 1000;
        uint32_t sec = (frozen_ms / 1000) % 60;
        uint32_t min = frozen_ms / 60000;
        sprintf(g_time_buf, "End:  %02lu:%02lu.%03lu",
                (unsigned long)min, (unsigned long)sec, (unsigned long)ms);
        OLED_ShowString(0, 7, (uint8_t *)g_time_buf, 8);
    } else {
        task_timer_show();
    }

    CameraUART_SendCmd(5);
/*---------------------------------------------------------------------*/

}


/* ====== 任务六：巡线 → 检测停车标志后缓慢停车 ====== */
#define TASK6_SPEED         0.25f
#define TASK6_ACCEL_STEP    0.002f      /* 加速步进：越小加速越缓慢 */

void task_6(void)
{
    static uint8_t  running = 0;
    static float    cur_speed = 0.0f;
    static uint8_t  stopping  = 0;
    static uint8_t  time_frozen = 0;
    static uint32_t frozen_ms = 0;
    static float    stop_dist = 0.0f;

    if (!g_start_flag) { running = 0; cur_speed = 0.0f; stopping = 0; time_frozen = 0; }

    if (!running) {
        task_timer_start();
        reset_position();
        cur_speed = 0.0f;
        stopping = 0;
        time_frozen = 0;
        running = 1;
    }

    float dist = (pos_left + pos_right) * 0.5f;

    if (stopping) {
        if (stopping == 1) {
            /* 检测到停车标志后：再走50cm开始减速 */
            if (dist - stop_dist >= 50.0f) {
                stopping = 2;
            }
        }
        if (stopping == 2) {
            /* 缓慢减速停车 */
            cur_speed = ramp_move(cur_speed, 0.0f, TASK6_ACCEL_STEP);
            set_target_speed(cur_speed);
            if (cur_speed == 0.0f) {
                running = 0;
                g_start_flag = 0;
            }
        }
        else {
            cur_speed = ramp_move(cur_speed, TASK6_SPEED, TASK6_ACCEL_STEP);
            set_target_speed(cur_speed);
        }
    } else {
        cur_speed = ramp_move(cur_speed, TASK6_SPEED, TASK6_ACCEL_STEP);
        set_target_speed(cur_speed);
        if (detect_stop()) {
            /* 检测到停车标志：记录时间与位置 */
            frozen_ms = tick_ms - g_task_start_ms;
            stop_dist = dist;
            time_frozen = 1;
            stopping = 1;
        }
    }

    /* 显示：检测到标志后显示冻结时间，否则实时显示 */
    if (time_frozen) {
        uint32_t ms  = frozen_ms % 1000;
        uint32_t sec = (frozen_ms / 1000) % 60;
        uint32_t min = frozen_ms / 60000;
        sprintf(g_time_buf, "End:  %02lu:%02lu.%03lu",
                (unsigned long)min, (unsigned long)sec, (unsigned long)ms);
        OLED_ShowString(0, 7, (uint8_t *)g_time_buf, 8);
    } else {
        task_timer_show();
    }

    CameraUART_SendCmd(6);
/*---------------------------------------------------------------------*/

}
