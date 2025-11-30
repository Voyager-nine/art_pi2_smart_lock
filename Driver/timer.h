/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-30     Voyager       the first version
 */
#ifndef DRIVER_TIMER_H_
#define DRIVER_TIMER_H_

#include <rtthread.h>

/* 初始化 PWM 功能 */
void TIM2_PWM_Init(void); // 保持你习惯的函数名，虽然底层实际是 TIM5

/* 门锁控制函数 */
/* enable: 1 = 上锁 (0度), 0 = 开锁 (90度) */
void lock(rt_uint8_t enable);

#endif /* DRIVER_TIMER_H_ */
