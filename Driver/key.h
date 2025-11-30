/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-30     Voyager       the first version
 */
#ifndef DRIVER_KEY_H_
#define DRIVER_KEY_H_

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

/* 行线定义 (输出) */
#define KEY_R1_PIN  GET_PIN(F, 13) // P1-8
#define KEY_R2_PIN  GET_PIN(F, 12) // P1-10
#define KEY_R3_PIN  GET_PIN(C, 3)  // P1-11
#define KEY_R4_PIN  GET_PIN(C, 2)  // P1-13

/* 列线定义 (输入) */
#define KEY_C1_PIN  GET_PIN(D, 3)  // P1-15
#define KEY_C2_PIN  GET_PIN(B, 1)  // P1-16
#define KEY_C3_PIN  GET_PIN(B, 2)  // P1-18
#define KEY_C4_PIN  GET_PIN(F, 4)  // P1-22

void key_init(void);
rt_uint8_t key_read(void);


#endif /* DRIVER_KEY_H_ */
