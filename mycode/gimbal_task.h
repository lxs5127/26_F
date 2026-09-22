/**
 * @file    gimbal_task.h
 * @brief   云台控制任务 - 专用定时器驱动
 * @note    使用独立的 TIMER_stepper 定时器以固定周期执行云台跟踪
 *          需要在 SysConfig 中新增一个定时器, $name = "TIMER_stepper"
 *          推荐配置: 周期 20ms (50Hz), 使用 TIMG1 或其它空闲定时器
 */

#ifndef __GIMBAL_TASK_H
#define __GIMBAL_TASK_H

#include "ball_ctrl.h"

/**
 * @brief   初始化云台控制任务
 * @note    使能 TIMER_stepper 并启动定时, 中断触发后自动调用 Gimbal_Run()
 *          在 main 中初始化完云台后调用一次即可
 */

#endif 
