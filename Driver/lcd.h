/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-30     Voyager       the first version
 */
#ifndef DRIVER_LCD_H_
#define DRIVER_LCD_H_
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

/* === 新增：类型兼容定义 === */
typedef rt_uint8_t  u8;
typedef rt_uint16_t u16;
typedef rt_uint32_t u32;
typedef rt_uint8_t  uint_8; /* 你的代码里有时候用 uint_8 */

#define USE_HORIZONTAL 1  // 0或1为竖屏 2或3为横屏

#if USE_HORIZONTAL==0||USE_HORIZONTAL==1
#define LCD_W 128
#define LCD_H 128
#else
#define LCD_W 128
#define LCD_H 128
#endif

//-----------------LCD引脚定义 (基于 P1 排针) ----------------
#define LCD_RES_PIN  GET_PIN(E, 12) // P1-31
#define LCD_DC_PIN   GET_PIN(E, 13) // P1-29
#define LCD_BLK_PIN  GET_PIN(D, 13) // P1-33
#define LCD_CS_PIN   GET_PIN(F, 6)  // P1-24 (用于传给 SPI 挂载函数)

// 操作宏 (使用 RTT API)
#define LCD_RES_Clr()  rt_pin_write(LCD_RES_PIN, PIN_LOW)
#define LCD_RES_Set()  rt_pin_write(LCD_RES_PIN, PIN_HIGH)

#define LCD_DC_Clr()   rt_pin_write(LCD_DC_PIN, PIN_LOW)
#define LCD_DC_Set()   rt_pin_write(LCD_DC_PIN, PIN_HIGH)

#define LCD_BLK_Clr()  rt_pin_write(LCD_BLK_PIN, PIN_LOW)
#define LCD_BLK_Set()  rt_pin_write(LCD_BLK_PIN, PIN_HIGH)

// CS 由 SPI 驱动自动控制，这里留空防止编译报错
#define LCD_CS_Clr()
#define LCD_CS_Set()

// 颜色定义 (保持不变)
#define WHITE            0xFFFF
#define BLACK            0x0000
#define BLUE             0x001F
#define BRED             0XF81F
#define GRED             0XFFE0
#define GBLUE            0X07FF
#define RED              0xF800
#define MAGENTA          0xF81F
#define GREEN            0x07E0
#define CYAN             0x7FFF
#define YELLOW           0xFFE0
#define BROWN            0XBC40
#define BRRED            0XFC07
#define GRAY             0X8430
#define DARKBLUE         0X01CF
#define LIGHTBLUE        0X7D7C
#define GRAYBLUE         0X5458
#define LIGHTGREEN       0X841F
#define LGRAY            0XC618
#define LGRAYBLUE        0XA651
#define LBBLUE           0X2B12

// 函数声明
int LCD_Init_RTT(void);
void LCD_Init(void);
void LCD_Fill(u16 xsta,u16 ysta,u16 xend,u16 yend,u16 color);
void LCD_DrawPoint(u16 x,u16 y,u16 color);
void LCD_DrawLine(u16 x1,u16 y1,u16 x2,u16 y2,u16 color);
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2,u16 color);
void Draw_Circle(u16 x0,u16 y0,u8 r,u16 color);
void LCD_ShowChinese(u16 x,u16 y,u8 *s,u16 fc,u16 bc,u8 sizey,u8 mode);
void LCD_ShowChinese16x16(u16 x,u16 y,u8 *s,u16 fc,u16 bc,u8 sizey,u8 mode);
void LCD_ShowChinese24x24(u16 x,u16 y,u8 *s,u16 fc,u16 bc,u8 sizey,u8 mode);
void LCD_ShowChinese32x32(u16 x,u16 y,u8 *s,u16 fc,u16 bc,u8 sizey,u8 mode);
void LCD_ShowChar(u16 x,u16 y,u8 num,u16 fc,u16 bc,u8 sizey,u8 mode);
void LCD_ShowString(u16 x,u16 y,const u8 *p,u16 fc,u16 bc,u8 sizey,u8 mode);
u32 mypow(u8 m,u8 n);
void LCD_ShowIntNum(u16 x,u16 y,u16 num,u8 len,u16 fc,u16 bc,u8 sizey);
void LCD_ShowFloatNum1(u16 x,u16 y,float num,u8 len,u16 fc,u16 bc,u8 sizey);
void LCD_ShowPicture(u16 x,u16 y,u16 length,u16 width,const u8 pic[]);

void LCD_WR_DATA8(u8 dat);
void LCD_WR_DATA(u16 dat);
void LCD_WR_REG(u8 dat);
void LCD_Address_Set(u16 x1,u16 y1,u16 x2,u16 y2);



#endif /* DRIVER_LCD_H_ */
