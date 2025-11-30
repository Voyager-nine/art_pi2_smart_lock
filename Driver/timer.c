/**
 * @file    timer.c
 * @brief   舵机PWM控制驱动程序
 * @details 基于RT-Thread平台的舵机控制模块
 *          实现功能：
 *          - PWM信号生成与控制
 *          - 舵机角度精确控制
 *          - 门锁开关状态管理
 *          - 自动化硬件引脚配置
 * @author  移植自原项目
 * @date    2025-11-30
 * @version 1.0
 *
 * 硬件配置：
 * PWM定时器：TIM5
 * 输出通道：Channel 3
 * 控制引脚：PA2
 * PWM频率：50Hz (20ms周期)
 *
 * 舵机控制参数：
 * - 0度(上锁)：脉宽0.5ms
 * - 90度(开锁)：脉宽1.5ms
 */

/* 系统头文件 */
#include "timer.h"              /* 定时器驱动头文件 */
#include <rtdevice.h>           /* RT-Thread设备驱动框架 */
#include <board.h>              /* 板级支持包 */
#include "stm32h7rsxx_hal.h"    /* STM32H7RS HAL库，用于底层GPIO配置 */

/* ===================== 宏定义与配置参数 ===================== */

#define PWM_DEV_NAME        "pwm5"      /* ART-Pi2平台使用TIM5定时器 */
#define PWM_DEV_CHANNEL     3           /* PWM输出通道，对应物理引脚PA2 */

/* PWM参数配置（以纳秒为单位，符合RT-Thread标准） */
#define PWM_PERIOD_NS       20000000    /* PWM周期：20ms = 20,000,000ns (50Hz) */
#define PWM_MIN_NS          500000      /* 最小脉宽：0.5ms = 500,000ns (0度位置) */
#define PWM_90_NS           1500000     /* 90度脉宽：1.5ms = 1,500,000ns (90度位置) */

/* 全局变量 */
static struct rt_device_pwm *servo_dev = RT_NULL;  /* PWM设备句柄 */

/* ===================== PWM初始化函数 ===================== */

/**
 * @brief  舵机PWM控制器初始化函数
 * @note   函数职责：
 *         1. 查找并获取PWM设备句柄
 *         2. 使能指定的PWM输出通道
 *         3. 设置舵机初始状态为上锁位置
 *         4. 输出初始化状态信息
 *
 * @attention 该函数保持原有函数名以确保兼容性
 */
void TIM2_PWM_Init(void)
{
    /* ========== 步骤1：查找PWM设备 ========== */
    servo_dev = (struct rt_device_pwm *)rt_device_find(PWM_DEV_NAME);
    if (servo_dev == RT_NULL)
    {
        /* 设备未找到错误处理 */
        rt_kprintf("Error: Can't find %s! Check settings.\n", PWM_DEV_NAME);
        return;
    }

    /* ========== 步骤2：使能PWM输出通道 ========== */
    rt_pwm_enable(servo_dev, PWM_DEV_CHANNEL);

    /* ========== 步骤3：设置初始状态 ========== */
    lock(1);  /* 默认上锁状态，确保系统安全 */

    /* ========== 步骤4：输出初始化成功信息 ========== */
    rt_kprintf("PWM Init Success! Servo Ready.\n");
}

/* ===================== 门锁控制功能函数 ===================== */

/**
 * @brief  门锁控制函数
 * @param  enable: 门锁状态控制参数
 *                 1 = 上锁状态 (舵机转到0度位置)
 *                 0 = 开锁状态 (舵机转到90度位置)
 * @note   工作原理：
 *         - 通过调节PWM脉宽控制舵机转动角度
 *         - 舵机角度直接对应门锁的物理状态
 *         - 内置设备检查与自动初始化机制
 *         - 提供机械运动完成延时
 */
void lock(rt_uint8_t enable)
{
    /* ========== 设备有效性检查 ========== */
    if (servo_dev == RT_NULL)
    {
        /* 如果设备未初始化，尝试自动初始化 */
        TIM2_PWM_Init();
        if (servo_dev == RT_NULL)
        {
            /* 初始化失败，安全退出 */
            return;
        }
    }

    /* ========== 根据参数执行门锁控制 ========== */
    if(enable == 1)
    {
        /* === 上锁操作：舵机转到0度位置 === */
        rt_pwm_set(servo_dev, PWM_DEV_CHANNEL, PWM_PERIOD_NS, PWM_MIN_NS);
        rt_kprintf("Door Locked (0 deg)\n");  /* 调试信息输出 */
    }
    else
    {
        /* === 开锁操作：舵机转到90度位置 === */
        rt_pwm_set(servo_dev, PWM_DEV_CHANNEL, PWM_PERIOD_NS, PWM_90_NS);
        rt_kprintf("Door Unlocked (90 deg)\n");  /* 调试信息输出 */
    }

    /* ========== 机械运动完成延时 ========== */
    /* 舵机需要时间完成物理转动，延时300ms确保动作完成 */
    rt_thread_mdelay(300);
}

/* ===================== STM32H7平台特殊配置 ===================== */

/**
 * @brief  TIM5 PWM输出引脚配置函数
 * @param  htim: 定时器句柄指针
 * @note   STM32H7RS平台特殊处理：
 *         - RT-Thread配置工具中没有TIM5_CH3的直接选项
 *         - 需要手动配置PA2引脚作为TIM5_CH3的复用功能
 *         - 该函数会在PWM初始化时由HAL库自动调用
 *         - 确保PWM信号能够正确输出到PA2引脚
 *
 * @attention 此函数为STM32 HAL库回调函数，不可删除
 */
void HAL_TIM_MspPostInit(TIM_HandleTypeDef* htim)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};  /* GPIO初始化结构体 */

    /* ========== 检查是否为目标定时器TIM5 ========== */
    if(htim->Instance == TIM5)
    {
        /* ========== 步骤1：使能GPIOA时钟 ========== */
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* ========== 步骤2：配置PA2引脚参数 ========== */
        GPIO_InitStruct.Pin = GPIO_PIN_2;              /* 选择PA2引脚 */
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;        /* 推挽复用输出模式 */
        GPIO_InitStruct.Pull = GPIO_NOPULL;            /* 无内部上下拉 */
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;   /* 低频输出（PWM不需要高频） */

        /* ========== 步骤3：设置复用功能为TIM5 ========== */
        GPIO_InitStruct.Alternate = GPIO_AF2_TIM5;     /* 复用功能选择：AF2对应TIM5 */

        /* ========== 步骤4：应用GPIO配置 ========== */
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}
