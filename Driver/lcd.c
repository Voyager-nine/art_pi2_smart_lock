/**
 * @file    lcd.c
 * @brief   ST7735S彩色LCD显示驱动程序
 * @details 基于RT-Thread平台的TFT液晶显示驱动
 *          实现功能：
 *          - ST7735S控制器初始化与配置
 *          - SPI通信协议实现
 *          - 基础图形绘制(点、线、矩形、圆形)
 *          - 中文字符显示支持(16x16, 24x24, 32x32)
 *          - 英文字符显示支持(8x16, 16x32)
 *          - 图像显示与颜色填充
 *          - 数字与浮点数显示
 * @author  移植自原项目
 * @date    2025-11-30
 * @version 1.0
 *
 * 硬件配置：
 * LCD控制器：ST7735S
 * 通信接口：SPI5
 * 分辨率：128x128像素
 * 颜色深度：16位RGB565
 *
 * 引脚连接：
 * - CS: PF6 (片选)
 * - DC: 数据/命令选择
 * - RES: 复位控制
 * - BLK: 背光控制
 */

/* 系统头文件 */
#include "lcd.h"                /* LCD驱动头文件 */
#include "drv_spi.h"            /* RT-Thread SPI驱动 */
#include "font_ascii_16x8.h"    /* ASCII字符字库 */

/* ===================== 函数声明与配置 ===================== */

void LCD_Init_Regs(void);  /* ST7735S寄存器初始化函数声明 */

/* SPI设备配置 */
#define LCD_SPI_BUS_NAME  "spi5"    /* ART-Pi2 P1排针对应SPI5总线 */
#define LCD_DEV_NAME      "lcd0"    /* LCD SPI设备名称 */

/* 全局变量 */
static struct rt_spi_device *lcd_spi_dev;  /* SPI设备句柄 */

/* ===================== LCD初始化函数 ===================== */

/**
 * @brief  ST7735S液晶显示器初始化函数
 * @retval RT_EOK: 初始化成功
 * @retval -RT_ERROR: 初始化失败
 * @note   初始化流程：
 *         1. 配置控制GPIO引脚
 *         2. 挂载SPI设备到指定总线
 *         3. 配置SPI通信参数
 *         4. 执行LCD控制器初始化序列
 */
int LCD_Init_RTT(void)
{
    /* ========== 步骤1：配置LCD控制GPIO ========== */
    rt_pin_mode(LCD_DC_PIN, PIN_MODE_OUTPUT);   /* DC引脚：数据/命令选择 */
    rt_pin_mode(LCD_RES_PIN, PIN_MODE_OUTPUT);  /* RES引脚：复位控制 */
    rt_pin_mode(LCD_BLK_PIN, PIN_MODE_OUTPUT);  /* BLK引脚：背光控制 */

    /* ========== 步骤2：挂载SPI设备 ========== */
    /* 将LCD设备挂载到SPI5总线，CS引脚为PF6 */
    rt_hw_spi_device_attach(LCD_SPI_BUS_NAME, LCD_DEV_NAME, GPIOF, GPIO_PIN_6);

    /* ========== 步骤3：查找并获取SPI设备句柄 ========== */
    lcd_spi_dev = (struct rt_spi_device *)rt_device_find(LCD_DEV_NAME);
    if (!lcd_spi_dev)
    {
        /* 设备查找失败，输出错误信息 */
        rt_kprintf("Error: LCD SPI device %s not found!\n", LCD_DEV_NAME);
        rt_kprintf("Did you enable SPI5 in RT-Thread Settings?\n");
        return -RT_ERROR;
    }

    /* ========== 步骤4：配置SPI通信参数 ========== */
    struct rt_spi_configuration cfg;
    cfg.data_width = 8;                                         /* 数据宽度：8位 */
    cfg.mode = RT_SPI_MASTER | RT_SPI_MODE_0 | RT_SPI_MSB;     /* 主模式，模式0，MSB先发 */
    cfg.max_hz = 20 * 1000 * 1000;                            /* 最大频率：20MHz */
    rt_spi_configure(lcd_spi_dev, &cfg);

    /* ========== 步骤5：执行LCD寄存器初始化 ========== */
    LCD_Init_Regs();

    return RT_EOK;
}

/* 2. 替换底层的 SPI 发送函数 */
void LCD_Writ_Bus(u8 dat)
{
    rt_spi_send(lcd_spi_dev, &dat, 1);
}

void LCD_WR_DATA8(u8 dat)
{
    LCD_DC_Set();
    LCD_Writ_Bus(dat);
}

void LCD_WR_DATA(u16 dat)
{
    LCD_DC_Set();
    u8 buf[2];
    buf[0] = dat >> 8;
    buf[1] = dat & 0xff;
    rt_spi_send(lcd_spi_dev, buf, 2);
}

void LCD_WR_REG(u8 dat)
{
    LCD_DC_Clr();
    LCD_Writ_Bus(dat);
}

void LCD_Address_Set(u16 x1,u16 y1,u16 x2,u16 y2)
{
    if(USE_HORIZONTAL==0)
    {
        LCD_WR_REG(0x2a); LCD_WR_DATA(x1+2); LCD_WR_DATA(x2+2);
        LCD_WR_REG(0x2b); LCD_WR_DATA(y1+1); LCD_WR_DATA(y2+1);
        LCD_WR_REG(0x2c);
    }
    else if(USE_HORIZONTAL==1)
    {
        LCD_WR_REG(0x2a); LCD_WR_DATA(x1+2); LCD_WR_DATA(x2+2);
        LCD_WR_REG(0x2b); LCD_WR_DATA(y1+3); LCD_WR_DATA(y2+3);
        LCD_WR_REG(0x2c);
    }
    else if(USE_HORIZONTAL==2)
    {
        LCD_WR_REG(0x2a); LCD_WR_DATA(x1+1); LCD_WR_DATA(x2+1);
        LCD_WR_REG(0x2b); LCD_WR_DATA(y1+2); LCD_WR_DATA(y2+2);
        LCD_WR_REG(0x2c);
    }
    else
    {
        LCD_WR_REG(0x2a); LCD_WR_DATA(x1+3); LCD_WR_DATA(x2+3);
        LCD_WR_REG(0x2b); LCD_WR_DATA(y1+2); LCD_WR_DATA(y2+2);
        LCD_WR_REG(0x2c);
    }
}

/* 3. 屏幕初始化序列 (移植自原代码) */
void LCD_Init_Regs(void)
{
    LCD_RES_Clr();
    rt_thread_mdelay(100);
    LCD_RES_Set();
    rt_thread_mdelay(100);

    LCD_BLK_Set();
    rt_thread_mdelay(100);

    LCD_WR_REG(0x11); // Sleep out
    rt_thread_mdelay(120);

    // ... ST7735S 初始化参数 (完全保持你原代码不变) ...
    LCD_WR_REG(0xB1); LCD_WR_DATA8(0x05); LCD_WR_DATA8(0x3C); LCD_WR_DATA8(0x3C);
    LCD_WR_REG(0xB2); LCD_WR_DATA8(0x05); LCD_WR_DATA8(0x3C); LCD_WR_DATA8(0x3C);
    LCD_WR_REG(0xB3); LCD_WR_DATA8(0x05); LCD_WR_DATA8(0x3C); LCD_WR_DATA8(0x3C); LCD_WR_DATA8(0x05); LCD_WR_DATA8(0x3C); LCD_WR_DATA8(0x3C);
    LCD_WR_REG(0xB4); LCD_WR_DATA8(0x03);
    LCD_WR_REG(0x3a); LCD_WR_DATA8(0x05);

    LCD_WR_REG(0xC0); LCD_WR_DATA8(0xA2); LCD_WR_DATA8(0x02); LCD_WR_DATA8(0x84);
    LCD_WR_REG(0xC1); LCD_WR_DATA8(0xC5);
    LCD_WR_REG(0xC2); LCD_WR_DATA8(0x0D); LCD_WR_DATA8(0x00);
    LCD_WR_REG(0xC3); LCD_WR_DATA8(0x8D); LCD_WR_DATA8(0x2A);
    LCD_WR_REG(0xC4); LCD_WR_DATA8(0x8D); LCD_WR_DATA8(0xEE);
    LCD_WR_REG(0xC5); LCD_WR_DATA8(0x0a);

    LCD_WR_REG(0x36);
    if(USE_HORIZONTAL==0)LCD_WR_DATA8(0x08);
    else if(USE_HORIZONTAL==1)LCD_WR_DATA8(0xC8);
    else if(USE_HORIZONTAL==2)LCD_WR_DATA8(0x78);
    else LCD_WR_DATA8(0xA8);

    LCD_WR_REG(0XE0);
    LCD_WR_DATA8(0x12); LCD_WR_DATA8(0x1C); LCD_WR_DATA8(0x10); LCD_WR_DATA8(0x18);
    LCD_WR_DATA8(0x33); LCD_WR_DATA8(0x2C); LCD_WR_DATA8(0x25); LCD_WR_DATA8(0x28);
    LCD_WR_DATA8(0x28); LCD_WR_DATA8(0x27); LCD_WR_DATA8(0x2F); LCD_WR_DATA8(0x3C);
    LCD_WR_DATA8(0x00); LCD_WR_DATA8(0x03); LCD_WR_DATA8(0x03); LCD_WR_DATA8(0x10);

    LCD_WR_REG(0XE1);
    LCD_WR_DATA8(0x12); LCD_WR_DATA8(0x1C); LCD_WR_DATA8(0x10); LCD_WR_DATA8(0x18);
    LCD_WR_DATA8(0x2D); LCD_WR_DATA8(0x28); LCD_WR_DATA8(0x23); LCD_WR_DATA8(0x28);
    LCD_WR_DATA8(0x28); LCD_WR_DATA8(0x26); LCD_WR_DATA8(0x2F); LCD_WR_DATA8(0x3B);
    LCD_WR_DATA8(0x00); LCD_WR_DATA8(0x03); LCD_WR_DATA8(0x03); LCD_WR_DATA8(0x10);

    LCD_WR_REG(0x20);
    LCD_WR_REG(0x13);
    LCD_WR_REG(0x29);
    LCD_WR_REG(0x2C);
}

/******************************************************************************
      函数说明：在指定区域填充颜色
      入口数据：xsta,ysta   起始坐标
                xend,yend   终止坐标
                                color       要填充的颜色
      返回值：  无
******************************************************************************/
void LCD_Fill(u16 xsta,u16 ysta,u16 xend,u16 yend,u16 color)
{
    u16 i,j;
    LCD_Address_Set(xsta,ysta,xend-1,yend-1);//设置显示范围
    for(i=ysta;i<yend;i++)
    {
        for(j=xsta;j<xend;j++)
        {
            LCD_WR_DATA(color);
        }
    }
}

/******************************************************************************
      函数说明：在指定位置画点
      入口数据：x,y 画点坐标
                color 点的颜色
      返回值：  无
******************************************************************************/
void LCD_DrawPoint(u16 x,u16 y,u16 color)
{
    LCD_Address_Set(x,y,x,y);//设置光标位置
    LCD_WR_DATA(color);
}


/******************************************************************************
      函数说明：画线
      入口数据：x1,y1   起始坐标
                x2,y2   终止坐标
                color   线的颜色
      返回值：  无
******************************************************************************/
void LCD_DrawLine(u16 x1,u16 y1,u16 x2,u16 y2,u16 color)
{
    u16 t;
    int xerr=0,yerr=0,delta_x,delta_y,distance;
    int incx,incy,uRow,uCol;
    delta_x=x2-x1; //计算坐标增量
    delta_y=y2-y1;
    uRow=x1;//画线起点坐标
    uCol=y1;
    if(delta_x>0)incx=1; //设置单步方向
    else if (delta_x==0)incx=0;//垂直线
    else {incx=-1;delta_x=-delta_x;}
    if(delta_y>0)incy=1;
    else if (delta_y==0)incy=0;//水平线
    else {incy=-1;delta_y=-delta_x;}
    if(delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴
    else distance=delta_y;
    for(t=0;t<distance+1;t++)
    {
        LCD_DrawPoint(uRow,uCol,color);//画点
        xerr+=delta_x;
        yerr+=delta_y;
        if(xerr>distance)
        {
            xerr-=distance;
            uRow+=incx;
        }
        if(yerr>distance)
        {
            yerr-=distance;
            uCol+=incy;
        }
    }
}


/******************************************************************************
      函数说明：画矩形
      入口数据：x1,y1   起始坐标
                x2,y2   终止坐标
                color   矩形的颜色
      返回值：  无
******************************************************************************/
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2,u16 color)
{
    LCD_DrawLine(x1,y1,x2,y1,color);
    LCD_DrawLine(x1,y1,x1,y2,color);
    LCD_DrawLine(x1,y2,x2,y2,color);
    LCD_DrawLine(x2,y1,x2,y2,color);
}


/******************************************************************************
      函数说明：画圆
      入口数据：x0,y0   圆心坐标
                r       半径
                color   圆的颜色
      返回值：  无
******************************************************************************/
void Draw_Circle(u16 x0,u16 y0,u8 r,u16 color)
{
    int a,b;
    a=0;b=r;
    while(a<=b)
    {
        LCD_DrawPoint(x0-b,y0-a,color);             //3
        LCD_DrawPoint(x0+b,y0-a,color);             //0
        LCD_DrawPoint(x0-a,y0+b,color);             //1
        LCD_DrawPoint(x0-a,y0-b,color);             //2
        LCD_DrawPoint(x0+b,y0+a,color);             //4
        LCD_DrawPoint(x0+a,y0-b,color);             //5
        LCD_DrawPoint(x0+a,y0+b,color);             //6
        LCD_DrawPoint(x0-b,y0+a,color);             //7
        a++;
        if((a*a+b*b)>(r*r))//判断要画的点是否过远
        {
            b--;
        }
    }
}

/******************************************************************************
      函数说明：显示汉字串
      入口数据：x,y显示坐标
                *s 要显示的汉字串
                fc 字的颜色
                bc 字的背景色
                sizey 字号 可选 16 24 32
                mode:  0非叠加模式  1叠加模式
      返回值：  无
******************************************************************************/
void LCD_ShowChinese(u16 x,u16 y,u8 *s,u16 fc,u16 bc,u8 sizey,u8 mode)
{
    u16 temp = x;
    while(*s!=0)
    {
        if(sizey==16) LCD_ShowChinese16x16(x,y,s,fc,bc,sizey,mode);
        else if(sizey==24) LCD_ShowChinese24x24(x,y,s,fc,bc,sizey,mode);
        else if(sizey==32) LCD_ShowChinese32x32(x,y,s,fc,bc,sizey,mode);
        else return;
        s+=2;
        x+=sizey;
        if(x > 120)
        {
            x = temp;
            y+=sizey;
        }
    }
}

/******************************************************************************
      函数说明：显示单个16x16汉字
      入口数据：x,y显示坐标
                *s 要显示的汉字
                fc 字的颜色
                bc 字的背景色
                sizey 字号
                mode:  0非叠加模式  1叠加模式
      返回值：  无
******************************************************************************/
void LCD_ShowChinese16x16(u16 x,u16 y,u8 *s,u16 fc,u16 bc,u8 sizey,u8 mode)
{
    u8 i,j;
    u16 k;
    u16 HZnum;//汉字数目
    u16 TypefaceNum;//一个字符所占字节大小
    u16 x0=x;
    TypefaceNum=sizey/8*sizey;//此算法只适用于字宽等于字高，且字高是8的倍数的字，
                              //也建议用户使用这样大小的字,否则显示容易出问题！
    HZnum=sizeof(tfont16)/sizeof(typFNT_GB16);  //统计汉字数目
    for(k=0;k<HZnum;k++)
    {
        if ((tfont16[k].Index[0]==*(s))&&(tfont16[k].Index[1]==*(s+1)))
        {
            LCD_Address_Set(x,y,x+sizey-1,y+sizey-1);
            for(i=0;i<TypefaceNum;i++)
            {
                for(j=0;j<8;j++)
                {
                    if(!mode)//非叠加方式
                    {
                        if(tfont16[k].Msk[i]&(0x01<<j))LCD_WR_DATA(fc);
                        else LCD_WR_DATA(bc);
                    }
                    else//叠加方式
                    {
                        if(tfont16[k].Msk[i]&(0x01<<j)) LCD_DrawPoint(x,y,fc);//画一个点
                        x++;
                        if((x-x0)==sizey)
                        {
                            x=x0;
                            y++;
                            break;
                        }
                    }
                }
            }
        }
        continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
    }
}


/******************************************************************************
      函数说明：显示单个24x24汉字
      入口数据：x,y显示坐标
                *s 要显示的汉字
                fc 字的颜色
                bc 字的背景色
                sizey 字号
                mode:  0非叠加模式  1叠加模式
      返回值：  无
******************************************************************************/
void LCD_ShowChinese24x24(u16 x,u16 y,u8 *s,u16 fc,u16 bc,u8 sizey,u8 mode)
{
    u8 i,j;
    u16 k;
    u16 HZnum;//汉字数目
    u16 TypefaceNum;//一个字符所占字节大小
    u16 x0=x;
    TypefaceNum=sizey/8*sizey;//此算法只适用于字宽等于字高，且字高是8的倍数的字，
                              //也建议用户使用这样大小的字,否则显示容易出问题！
    HZnum=sizeof(tfont24)/sizeof(typFNT_GB24);  //统计汉字数目
    for(k=0;k<HZnum;k++)
    {
        if ((tfont24[k].Index[0]==*(s))&&(tfont24[k].Index[1]==*(s+1)))
        {
            LCD_Address_Set(x,y,x+sizey-1,y+sizey-1);
            for(i=0;i<TypefaceNum;i++)
            {
                for(j=0;j<8;j++)
                {
                    if(!mode)//非叠加方式
                    {
                        if(tfont24[k].Msk[i]&(0x01<<j))LCD_WR_DATA(fc);
                        else LCD_WR_DATA(bc);
                    }
                    else//叠加方式
                    {
                        if(tfont24[k].Msk[i]&(0x01<<j)) LCD_DrawPoint(x,y,fc);//画一个点
                        x++;
                        if((x-x0)==sizey)
                        {
                            x=x0;
                            y++;
                            break;
                        }
                    }
                }
            }
        }
        continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
    }
}

/******************************************************************************
      函数说明：显示单个32x32汉字
      入口数据：x,y显示坐标
                *s 要显示的汉字
                fc 字的颜色
                bc 字的背景色
                sizey 字号
                mode:  0非叠加模式  1叠加模式
      返回值：  无
******************************************************************************/
void LCD_ShowChinese32x32(u16 x,u16 y,u8 *s,u16 fc,u16 bc,u8 sizey,u8 mode)
{
    u8 i,j;
    u16 k;
    u16 HZnum;//汉字数目
    u16 TypefaceNum;//一个字符所占字节大小
    u16 x0=x;
    TypefaceNum=sizey/8*sizey;//此算法只适用于字宽等于字高，且字高是8的倍数的字，
                              //也建议用户使用这样大小的字,否则显示容易出问题！
    HZnum=sizeof(tfont32)/sizeof(typFNT_GB32);  //统计汉字数目
    for(k=0;k<HZnum;k++)
    {
        if ((tfont32[k].Index[0]==*(s))&&(tfont32[k].Index[1]==*(s+1)))
        {
            LCD_Address_Set(x,y,x+sizey-1,y+sizey-1);
            for(i=0;i<TypefaceNum;i++)
            {
                for(j=0;j<8;j++)
                {
                    if(!mode)//非叠加方式
                    {
                        if(tfont32[k].Msk[i]&(0x01<<j))LCD_WR_DATA(fc);
                        else LCD_WR_DATA(bc);
                    }
                    else//叠加方式
                    {
                        if(tfont32[k].Msk[i]&(0x01<<j)) LCD_DrawPoint(x,y,fc);//画一个点
                        x++;
                        if((x-x0)==sizey)
                        {
                            x=x0;
                            y++;
                            break;
                        }
                    }
                }
            }
        }
        continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
    }
}


/******************************************************************************
      函数说明：显示单个字符
      入口数据：x,y显示坐标
                num 要显示的字符
                fc 字的颜色
                bc 字的背景色
                sizey 字号
                mode:  0非叠加模式  1叠加模式
      返回值：  无
******************************************************************************/
void LCD_ShowChar(u16 x,u16 y,u8 num,u16 fc,u16 bc,u8 sizey,u8 mode)
{
    u8 temp,sizex,t;
    u16 i,TypefaceNum;//一个字符所占字节大小
    u16 x0=x;
    sizex=sizey/2;
    TypefaceNum=sizex/8*sizey;
    num=num-' ';    //得到偏移后的值
    LCD_Address_Set(x,y,x+sizex-1,y+sizey-1);  //设置光标位置
    for(i=0;i<TypefaceNum;i++)
    {
        if(sizey==16)temp=ascii_1608[num][i];              //调用8x16字体
        else if(sizey==32)temp=ascii_3216[num][i];       //调用16x32字体
        else return;
        for(t=0;t<8;t++)
        {
            if(!mode)//非叠加模式
            {
                if(temp&(0x01<<t))LCD_WR_DATA(fc);
                else LCD_WR_DATA(bc);
            }
            else//叠加模式
            {
                if(temp&(0x01<<t))LCD_DrawPoint(x,y,fc);//画一个点
                x++;
                if((x-x0)==sizex)
                {
                    x=x0;
                    y++;
                    break;
                }
            }
        }
    }
}


/******************************************************************************
      函数说明：显示字符串
      入口数据：x,y显示坐标
                *p 要显示的字符串
                fc 字的颜色
                bc 字的背景色
                sizey 字号
                mode:  0非叠加模式  1叠加模式
      返回值：  无
******************************************************************************/
void LCD_ShowString(u16 x,u16 y,const u8 *p,u16 fc,u16 bc,u8 sizey,u8 mode)
{
    u16 temp = x;
    while(*p!='\0')
    {
        LCD_ShowChar(x,y,*p,fc,bc,sizey,mode);
        x+=sizey/2;
        if(x>120)
        {
            x = temp;
            y+=sizey;
        }
        p++;
    }
}


/******************************************************************************
      函数说明：显示数字
      入口数据：m底数，n指数
      返回值：  无
******************************************************************************/
u32 mypow(u8 m,u8 n)
{
    u32 result=1;
    while(n--)result*=m;
    return result;
}


/******************************************************************************
      函数说明：显示整数变量
      入口数据：x,y显示坐标
                num 要显示整数变量
                len 要显示的位数
                fc 字的颜色
                bc 字的背景色
                sizey 字号
      返回值：  无
******************************************************************************/
void LCD_ShowIntNum(u16 x,u16 y,u16 num,u8 len,u16 fc,u16 bc,u8 sizey)
{
    u8 t,temp;
    u8 enshow=0;
    u8 sizex=sizey/2;
    for(t=0;t<len;t++)
    {
        temp=(num/mypow(10,len-t-1))%10;
        if(enshow==0&&t<(len-1))
        {
            if(temp==0)
            {
                LCD_ShowChar(x+t*sizex,y,' ',fc,bc,sizey,0);
                continue;
            }else enshow=1;

        }
        LCD_ShowChar(x+t*sizex,y,temp+48,fc,bc,sizey,0);
    }
}


/******************************************************************************
      函数说明：显示两位小数变量
      入口数据：x,y显示坐标
                num 要显示小数变量
                len 要显示的位数
                fc 字的颜色
                bc 字的背景色
                sizey 字号
      返回值：  无
******************************************************************************/
void LCD_ShowFloatNum1(u16 x,u16 y,float num,u8 len,u16 fc,u16 bc,u8 sizey)
{
    u8 t,temp,sizex;
    u16 num1;
    sizex=sizey/2;
    num1=num*100;
    for(t=0;t<len;t++)
    {
        temp=(num1/mypow(10,len-t-1))%10;
        if(t==(len-2))
        {
            LCD_ShowChar(x+(len-2)*sizex,y,'.',fc,bc,sizey,0);
            t++;
            len+=1;
        }
        LCD_ShowChar(x+t*sizex,y,temp+48,fc,bc,sizey,0);
    }
}


/******************************************************************************
      函数说明：显示图片
      入口数据：x,y起点坐标
                length 图片长度
                width  图片宽度
                pic[]  图片数组
      返回值：  无
******************************************************************************/
void LCD_ShowPicture(u16 x,u16 y,u16 length,u16 width,const u8 pic[])
{
    u16 i,j,k=0;
    LCD_Address_Set(x,y,x+length-1,y+width-1);
    for(i=0;i<length;i++)
    {
        for(j=0;j<width;j++)
        {
            LCD_WR_DATA8(pic[k*2]);
            LCD_WR_DATA8(pic[k*2+1]);
            k++;
        }
    }
}
