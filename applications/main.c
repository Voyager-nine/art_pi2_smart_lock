/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-30     Voyager      Ported to ART-Pi 2
 */

/**
 * @file    main.c
 * @brief   智能密码门锁主程序
 * @details 基于RT-Thread操作系统的智能门锁控制系统
 *          实现功能：
 *          - 4x4矩阵键盘密码输入
 *          - 彩色LCD显示界面
 *          - 舵机控制门锁开关
 *          - 多线程任务管理
 *          - 密码验证与安全控制
 * @author  Voyager
 * @date    2025-11-30
 * @version 1.0
 *
 * 硬件平台：ART-Pi 2 (STM32H7RS)
 * 操作系统：RT-Thread 5.x
 * 编译环境：MDK-ARM / GCC
 */

/* RT-Thread系统头文件 */
#include <rtthread.h>    /* RT-Thread核心API */
#include <rtdevice.h>    /* RT-Thread设备驱动框架 */
#include <board.h>       /* 板级支持包 */

/* 项目自定义驱动头文件 */
#include "lcd.h"         /* lcd显示驱动 */
#include "key.h"         /* 4x4矩阵键盘驱动 */
#include "timer.h"       /* 舵机PWM控制驱动 */
#include "pic.h"         /* 图像资源数据定义 */

/* ===================== 全局变量定义 ===================== */

/**
 * @brief 系统预设密码数组
 * @note  6位密码：{1, 2, 3, 4, 5, 6}，可根据需求修改
 */
u8 password[6] = {1, 2, 3, 4, 5, 6};

/**
 * @brief 用户输入密码临时存储数组
 * @note  数组大小为7，支持6位密码输入 + 1位结束标志
 */
u8 key_temp[7] = {0};

/**
 * @brief 当前已输入密码位数计数器
 * @note  范围：0-6，超过6位时会被限制
 */
u8 key_index = 0;

/**
 * @brief 上一次密码位数状态记录
 * @note  用于LCD界面刷新判断，初始值255确保首次刷新
 */
u8 key_index_old = 255;

/* ===================== 辅助函数实现 ===================== */

/**
 * @brief  密码字符串比较验证函数
 * @param  string1: 用户输入密码数组指针
 * @param  string2: 系统预设密码数组指针
 * @param  len:     要比较的字节长度
 * @retval 验证结果: 1=密码正确, 0=密码错误
 * @note   从数组末尾开始逆向比较，提高安全性
 */
u8 string_chek(u8* string1, u8* string2, u8 len)
{
    /* 从末尾开始逆向比较每个字节 */
    while(len--)
    {
        /* 如果当前位置字符相同，继续比较下一位 */
        if(string1[len] == string2[len])
            ;
        else
            /* 发现不匹配，立即返回失败 */
            return 0;
    }
    /* 所有位都匹配，返回成功 */
    return 1;
}

/* ===================== RT-Thread线程入口函数 ===================== */

/**
 * @brief  按键扫描与业务逻辑处理线程入口函数
 * @param  parameter: RT-Thread线程参数(未使用)
 * @note   线程功能：
 *         1. 4x4矩阵键盘扫描与按键处理
 *         2. 密码输入逻辑控制
 *         3. 门锁开关控制
 *         4. LCD界面状态切换
 *         线程优先级：20 (中等优先级)
 *         扫描周期：10ms
 */
void key_process_thread_entry(void *parameter)
{
    /* 局部变量定义 */
    u8 key_val, key_old = 0, key_down;  /* 按键状态变量 */
    u8 i = 0;                           /* 循环计数器 */

    /* -------------------- 外设初始化 -------------------- */
    key_init();      /* 初始化4x4矩阵键盘GPIO */
    TIM2_PWM_Init(); /* 初始化舵机PWM控制(底层使用TIM5) */

    /* -------------------- 主循环 -------------------- */
    while (1)
    {
        /* 读取当前按键状态 */
        key_val = key_read();

        /* 按键消抖与下降沿检测算法 */
        /* key_down = 当前按下 AND (当前状态 XOR 上次状态) */
        /* 只有在按键从释放到按下的瞬间，key_down才为真 */
        key_down = key_val & (key_val ^ key_old);
        key_old = key_val;  /* 保存当前状态供下次比较 */

        /* 检测到有效按键按下 */
        if (key_down)
        {
            /* 根据按键值执行相应操作 */
            switch(key_down)
            {
                /* ========== 数字键 1-9 处理 ========== */
                case 1: case 2: case 3:    /* 第一行：1,2,3 */
                case 5: case 6: case 7:    /* 第二行：4,5,6 */
                case 9: case 10: case 11:  /* 第三行：7,8,9 */
                {
                    u8 num = 0;

                    /* 按键值到数字的映射算法 */
                    if (key_down <= 3)
                        num = key_down;              /* 1,2,3 直接映射 */
                    else if (key_down <= 7)
                        num = key_down - 1;          /* 5,6,7 -> 4,5,6 */
                    else
                        num = key_down - 2;          /* 9,10,11 -> 7,8,9 */

                    /* 密码缓冲区未满时，添加新输入 */
                    if (key_index < 6) {
                        key_temp[key_index] = num;   /* 存储数字 */
                        key_index++;                 /* 递增位数计数 */
                    }
                }
                break;

                /* ========== 数字键 0 处理 ========== */
                case 14:  /* 第四行第二列：0 */
                    if (key_index < 6) {
                        key_temp[key_index] = 0;     /* 存储数字0 */
                        key_index++;                 /* 递增位数计数 */
                    }
                break;

                /* ========== 清除键处理 ========== */
                case 13:  /* 第四行第一列：清除键 */
                   key_index = 0;                    /* 重置输入计数 */
                   /* 清空整个输入缓冲区 */
                   for(i=0; i<7; i++) key_temp[i] = 0;
                break;

                /* ========== 确认键处理 ========== */
                case 15:  /* 第四行第三列：确认键 */
                    key_index = 0;  /* 重置输入计数，准备下次输入 */

                    /* 调用密码验证函数 */
                    if(string_chek(key_temp, password, 6))
                    {
                        /* ===== 密码正确：开锁流程 ===== */
                        lock(0);  /* 舵机转到开锁位置 */
                        LCD_ShowPicture(0, 0, 128, 128, gImage_3);  /* 显示开锁成功图片 */
                        rt_thread_mdelay(5000);  /* 显示5秒钟 */

                        /* 自动关锁并返回主界面 */
                        lock(1);  /* 舵机转到关锁位置 */
                        LCD_ShowPicture(0, 0, 128, 128, gImage_2);  /* 显示主界面背景 */
                        LCD_ShowChinese(0, 0, (u8*)"门已上锁，请输入密码", BLUE, WHITE, 16, 0);
                    }
                    else
                    {
                        /* ===== 密码错误：报警流程 ===== */
                        lock(1);  /* 确保门锁处于关闭状态 */
                        LCD_ShowPicture(0, 0, 128, 128, gImage_4);  /* 显示错误警告图片 */
                        rt_thread_mdelay(1000);  /* 显示1秒钟警告 */

                        /* 返回主界面等待重新输入 */
                        LCD_ShowPicture(0, 0, 128, 128, gImage_2);  /* 显示主界面背景 */
                        LCD_ShowChinese(0, 0, (u8*)"门已上锁，请输入密码", BLUE, WHITE, 16, 0);
                    }
                    /* 清空输入缓存，防止残留数据 */
                    for(i=0; i<7; i++) key_temp[i] = 0;
                break;
            }
        }

        /* 线程休眠10ms，控制按键扫描频率 */
        rt_thread_mdelay(10);
    }
}

/**
 * @brief  LCD界面刷新线程入口函数
 * @param  parameter: RT-Thread线程参数(未使用)
 * @note   线程功能：
 *         1. 监控密码输入状态变化
 *         2. 动态更新LCD显示内容
 *         3. 实时显示当前输入的密码位数
 *         线程优先级：21 (低于按键线程，确保按键响应优先)
 *         刷新周期：100ms
 */
void lcd_refresh_thread_entry(void *parameter)
{
    /* 等待主线程完成LCD初始化后再开始刷新任务 */
    rt_thread_mdelay(500);

    /* -------------------- 主循环 -------------------- */
    while (1)
    {
        /* 检查密码输入状态是否发生变化 */
        if(key_index != key_index_old)
        {
            /* ===== 刷新密码输入显示区域 ===== */

            /* 清除原有的密码输入区域，使用黄色背景 */
            LCD_Fill(16, 45, 112, 60, YELLOW);

            /* 防止数组越界保护 */
            if(key_index > 6) key_index = 6;

            /* 循环显示当前已输入的每一位密码 */
            for(int j=0; j<key_index; j++)
            {
                /* 将数字转换为ASCII字符并显示 */
                /* 位置计算：起始X坐标20，每个字符间隔16像素 */
                /* 颜色：红色字体，黄色背景，字体大小16，非叠加模式 */
                LCD_ShowChar(20 + 16*j, 45, key_temp[j] + 48, RED, YELLOW, 16, 0);
            }

            /* 更新状态记录，避免重复刷新 */
            key_index_old = key_index;
        }

        /* 线程休眠100ms，控制刷新频率 */
        /* 较低的刷新频率可以节省CPU资源，提高整体系统性能 */
        rt_thread_mdelay(100);
    }
}

/* ===================== 系统主函数 ===================== */

/**
 * @brief  系统主函数 - RT-Thread应用程序入口
 * @retval RT_EOK: 系统初始化成功
 * @note   主函数职责：
 *         1. 硬件外设初始化
 *         2. LCD显示系统初始化
 *         3. 启动画面与用户界面初始化
 *         4. 创建并启动多线程任务
 *         5. 系统向量表重映射配置
 */
int main(void)
{
    /* ==================== 阶段1：硬件初始化 ==================== */
    key_init();        /* 初始化4x4矩阵键盘GPIO配置 */
    TIM2_PWM_Init();   /* 初始化舵机PWM控制器(实际使用TIM5) */
    lock(1);           /* 系统启动时默认处于上锁状态，确保安全 */

    /* ==================== 阶段2：LCD显示系统初始化 ==================== */
    LCD_Init_RTT();    /* 初始化ST7735S液晶驱动，配置SPI通信 */
    LCD_Fill(0, 0, 127, 127, WHITE);  /* 清屏：全屏填充白色背景 */

    /* ==================== 阶段3：开机启动动画 ==================== */
    /* 显示启动提示文字 */
    LCD_ShowChinese(20, 50, (u8*)"正在启动", RED, WHITE, 16, 0);

    /* 绘制进度条动画效果 */
    u8 i = 0;
    while(i < 128)  /* 从左到右绘制128像素宽的进度条 */
    {
        /* 绘制垂直红色线条，Y坐标从100到128 */
        LCD_DrawLine(i, 100, i, 128, RED);
        rt_thread_mdelay(10);  /* 每画一条线延时10ms，形成动态效果 */
        i++;
    }

    /* 启动完成提示 */
    LCD_ShowChinese(20, 50, (u8*)"启动成功", RED, WHITE, 16, 0);
    rt_thread_mdelay(500);  /* 显示500ms */

    /* 显示产品Logo */
    LCD_ShowPicture(0, 0, 128, 128, gImage_1);  /* 全屏显示Logo图片 */
    rt_thread_mdelay(1000);  /* Logo显示1秒 */

    /* ==================== 阶段4：进入主界面 ==================== */
    LCD_ShowPicture(0, 0, 128, 128, gImage_2);  /* 显示主界面背景图片 */
    LCD_ShowChinese(0, 0, (u8*)"门已上锁，请输入密码", BLUE, WHITE, 16, 0);  /* 显示提示文字 */
    LCD_Fill(16, 45, 112, 60, YELLOW);  /* 绘制黄色密码输入框 */

    /* ==================== 阶段5：创建多线程任务 ==================== */

    /* 创建按键处理线程 */
    /* 线程名称："key_logic"，入口函数：key_process_thread_entry */
    /* 栈大小：2048字节，优先级：20，时间片：10 */
    rt_thread_t tid_key = rt_thread_create("key_logic", key_process_thread_entry, RT_NULL, 2048, 20, 10);
    if (tid_key != RT_NULL)
    {
        rt_thread_startup(tid_key);  /* 启动按键线程 */
    }

    /* 创建LCD刷新线程 */
    /* 线程名称："lcd_show"，入口函数：lcd_refresh_thread_entry */
    /* 栈大小：2048字节，优先级：21，时间片：10 */
    rt_thread_t tid_lcd = rt_thread_create("lcd_show", lcd_refresh_thread_entry, RT_NULL, 2048, 21, 10);
    if (tid_lcd != RT_NULL)
    {
        rt_thread_startup(tid_lcd);  /* 启动LCD刷新线程 */
    }

    /* 主函数执行完毕，系统转入多线程调度模式 */
    return RT_EOK;
}

/* ===================== STM32H7平台特殊配置 ===================== */

#include "stm32h7rsxx.h"  /* STM32H7RS系列芯片寄存器定义 */

/**
 * @brief  STM32H7RS中断向量表重映射配置函数
 * @retval 0: 配置成功
 * @note   STM32H7RS系列特殊要求：
 *         - 由于芯片特殊的存储器映射机制
 *         - 必须将中断向量表重映射到XSPI2_BASE地址
 *         - 这是ART-Pi 2平台正常运行的必要配置
 *         - 该函数会在板级初始化阶段自动被RT-Thread调用
 */
static int vtor_config(void)
{
    /* 设置中断向量表基址寄存器 */
    /* XSPI2_BASE: 外部SPI存储器映射基地址 */
    SCB->VTOR = XSPI2_BASE;
    return 0;
}

/* 注册为板级初始化函数，系统启动时自动执行 */
INIT_BOARD_EXPORT(vtor_config);
