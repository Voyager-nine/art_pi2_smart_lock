/**
 * @file    key.c
 * @brief   4x4矩阵键盘驱动程序
 * @details 基于RT-Thread平台的矩阵键盘扫描驱动
 *          实现功能：
 *          - 4行4列矩阵键盘GPIO配置
 *          - 行列扫描按键检测算法
 *          - 支持16个按键的状态读取
 *          - 标准数字键盘布局支持
 * @author  移植自原项目
 * @date    2025-11-30
 * @version 1.0
 *
 * 硬件连接说明：
 * 行线(输出)：KEY_R1_PIN, KEY_R2_PIN, KEY_R3_PIN, KEY_R4_PIN
 * 列线(输入)：KEY_C1_PIN, KEY_C2_PIN, KEY_C3_PIN, KEY_C4_PIN
 *
 * 按键布局：
 *      C1  C2  C3   C4
 * R1   1   2    3    *
 * R2   4   5    6    *
 * R3   7   8    9    *
 * R4  清空   0   确认     *
 */

#include "key.h"  /* 矩阵键盘驱动头文件 */

/* ===================== 键盘初始化函数 ===================== */

/**
 * @brief  4x4矩阵键盘GPIO初始化函数
 * @note   配置策略：
 *         - 行线配置为推挽输出，默认输出高电平
 *         - 列线配置为上拉输入，默认为高电平
 *         - 扫描时将行线逐个拉低，检测列线状态
 *         - 按键按下时形成行列之间的导通路径
 */
void key_init(void)
{
    /* ========== 配置行线为推挽输出模式 ========== */
    rt_pin_mode(KEY_R1_PIN, PIN_MODE_OUTPUT);  /* 第一行输出线 */
    rt_pin_mode(KEY_R2_PIN, PIN_MODE_OUTPUT);  /* 第二行输出线 */
    rt_pin_mode(KEY_R3_PIN, PIN_MODE_OUTPUT);  /* 第三行输出线 */
    rt_pin_mode(KEY_R4_PIN, PIN_MODE_OUTPUT);  /* 第四行输出线 */

    /* 初始化所有行线为高电平（非激活状态） */
    rt_pin_write(KEY_R1_PIN, PIN_HIGH);
    rt_pin_write(KEY_R2_PIN, PIN_HIGH);
    rt_pin_write(KEY_R3_PIN, PIN_HIGH);
    rt_pin_write(KEY_R4_PIN, PIN_HIGH);

    /* ========== 配置列线为上拉输入模式 ========== */
    rt_pin_mode(KEY_C1_PIN, PIN_MODE_INPUT_PULLUP);  /* 第一列输入线 */
    rt_pin_mode(KEY_C2_PIN, PIN_MODE_INPUT_PULLUP);  /* 第二列输入线 */
    rt_pin_mode(KEY_C3_PIN, PIN_MODE_INPUT_PULLUP);  /* 第三列输入线 */
    rt_pin_mode(KEY_C4_PIN, PIN_MODE_INPUT_PULLUP);  /* 第四列输入线 */
}

/* ===================== 按键扫描函数 ===================== */

/**
 * @brief  4x4矩阵键盘扫描函数
 * @retval 按键值: 1-16对应不同按键，0表示无按键按下
 * @note   扫描算法：
 *         1. 逐行扫描：依次将R1-R4拉低，其他行保持高电平
 *         2. 检测列线：读取C1-C4的电平状态
 *         3. 按键映射：按下按键时对应列线被拉低
 *         4. 返回值计算：根据行列位置计算唯一按键值
 *
 * 按键值映射表：
 * R1C1=1   R1C2=2   R1C3=3   R1C4=4
 * R2C1=5   R2C2=6   R2C3=7   R2C4=8
 * R3C1=9   R3C2=10  R3C3=11  R3C4=12
 * R4C1=13  R4C2=14  R4C3=15  R4C4=16
 */
rt_uint8_t key_read(void)
{
    rt_uint8_t temp = 0;  /* 返回值变量，0表示无按键按下 */

    /* =============== 第一行扫描 (R1=Low) =============== */
    /* 设置扫描状态：R1拉低，其他行保持高电平 */
    rt_pin_write(KEY_R1_PIN, PIN_LOW);
    rt_pin_write(KEY_R2_PIN, PIN_HIGH);
    rt_pin_write(KEY_R3_PIN, PIN_HIGH);
    rt_pin_write(KEY_R4_PIN, PIN_HIGH);

    /* 等待电平稳定，消除GPIO切换时的毛刺干扰 */
    rt_hw_us_delay(10);

    /* 检测列线状态，按下按键时对应列线被拉低 */
    if (rt_pin_read(KEY_C4_PIN) == PIN_LOW) temp = 1;   /* R1C4 -> 键值1 */
    if (rt_pin_read(KEY_C3_PIN) == PIN_LOW) temp = 2;   /* R1C3 -> 键值2 */
    if (rt_pin_read(KEY_C2_PIN) == PIN_LOW) temp = 3;   /* R1C2 -> 键值3 */
    if (rt_pin_read(KEY_C1_PIN) == PIN_LOW) temp = 4;   /* R1C1 -> 键值4 */

    /* =============== 第二行扫描 (R2=Low) =============== */
    /* 切换扫描行：R2拉低，其他行保持高电平 */
    rt_pin_write(KEY_R1_PIN, PIN_HIGH);
    rt_pin_write(KEY_R2_PIN, PIN_LOW);
    rt_hw_us_delay(10);  /* 等待电平稳定 */

    /* 检测第二行各列按键状态 */
    if (rt_pin_read(KEY_C4_PIN) == PIN_LOW) temp = 5;   /* R2C4 -> 键值5 */
    if (rt_pin_read(KEY_C3_PIN) == PIN_LOW) temp = 6;   /* R2C3 -> 键值6 */
    if (rt_pin_read(KEY_C2_PIN) == PIN_LOW) temp = 7;   /* R2C2 -> 键值7 */
    if (rt_pin_read(KEY_C1_PIN) == PIN_LOW) temp = 8;   /* R2C1 -> 键值8 */

    /* =============== 第三行扫描 (R3=Low) =============== */
    /* 切换扫描行：R3拉低，其他行保持高电平 */
    rt_pin_write(KEY_R2_PIN, PIN_HIGH);
    rt_pin_write(KEY_R3_PIN, PIN_LOW);
    rt_hw_us_delay(10);  /* 等待电平稳定 */

    /* 检测第三行各列按键状态 */
    if (rt_pin_read(KEY_C4_PIN) == PIN_LOW) temp = 9;   /* R3C4 -> 键值9 */
    if (rt_pin_read(KEY_C3_PIN) == PIN_LOW) temp = 10;  /* R3C3 -> 键值10 */
    if (rt_pin_read(KEY_C2_PIN) == PIN_LOW) temp = 11;  /* R3C2 -> 键值11 */
    if (rt_pin_read(KEY_C1_PIN) == PIN_LOW) temp = 12;  /* R3C1 -> 键值12 */

    /* =============== 第四行扫描 (R4=Low) =============== */
    /* 切换扫描行：R4拉低，其他行保持高电平 */
    rt_pin_write(KEY_R3_PIN, PIN_HIGH);
    rt_pin_write(KEY_R4_PIN, PIN_LOW);
    rt_hw_us_delay(10);  /* 等待电平稳定 */

    /* 检测第四行各列按键状态 */
    if (rt_pin_read(KEY_C4_PIN) == PIN_LOW) temp = 13;  /* R4C4 -> 键值13 */
    if (rt_pin_read(KEY_C3_PIN) == PIN_LOW) temp = 14;  /* R4C3 -> 键值14 */
    if (rt_pin_read(KEY_C2_PIN) == PIN_LOW) temp = 15;  /* R4C2 -> 键值15 */
    if (rt_pin_read(KEY_C1_PIN) == PIN_LOW) temp = 16;  /* R4C1 -> 键值16 */

    /* 返回检测到的按键值，0表示无按键按下 */
    return temp;
}
