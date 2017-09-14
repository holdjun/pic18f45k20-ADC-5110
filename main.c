/* 
 * File:   main.c
 * Author: holdjohnh
 *
 * Created on September 12, 2017, 4:20 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <p18f45k20.h>
#include <delays.h>

#define LCD_RST LATDbits.LATD7 //5110宏定义
#define LCD_SCE LATDbits.LATD6
#define LCD_DC LATDbits.LATD5
#define LCD_SDIN LATDbits.LATD4
#define LCD_SCLK LATDbits.LATD3

//字库
unsigned char font6x8[][6] =
    {
        {0x00, 0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
        {0x00, 0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
        {0x00, 0x42, 0x61, 0x51, 0x49, 0x46}, // 2
        {0x00, 0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
        {0x00, 0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
        {0x00, 0x27, 0x45, 0x45, 0x45, 0x39}, // 5
        {0x00, 0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
        {0x00, 0x01, 0x71, 0x09, 0x05, 0x03}, // 7
        {0x00, 0x36, 0x49, 0x49, 0x49, 0x36}, // 8
        {0x00, 0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
        {0x00, 0x00, 0x36, 0x36, 0x00, 0x00}, // : 10
        {0x00, 0x00, 0x60, 0x60, 0x00, 0x00}, // . 11
        {0x00, 0x7C, 0x12, 0x11, 0x12, 0x7C}, // A 12
        {0x00, 0x3E, 0x41, 0x41, 0x41, 0x22}, // C 13
        {0x00, 0x7F, 0x41, 0x41, 0x22, 0x1C}, // D 14
        {0x00, 0x1C, 0x20, 0x40, 0x20, 0x1C}  // v 15
};

//延时函数
void delay_nms(unsigned int n)
{
    do
    {
        Delay1KTCYx(4);
    } while (--n);
}

//AD初始化
void init_ad()
{
    ADCON1 = 0X0C;
    ADCON2 = 0X94;
    ADCON0bits.ADON = 1;
    ADCON0bits.CHS0 = 0;
}

//AD获取返回值
unsigned int get_result()
{
    unsigned int dat = 0;
    ADCON0bits.GO_DONE = 1;
    while (ADCON0bits.GO_DONE)
        ;
    PIR1bits.ADIF = 0;
    dat = ADRESH;
    dat = dat << 8;
    dat = dat + ADRESL;
    return dat;
}

//5110时许
void LCD_write_byte(unsigned char dt, unsigned char command)
{
    unsigned char i;
    LCD_SCE = 0;
    LCD_DC = command;
    for (i = 0; i < 8; i++)
    {
        if (dt & 0x80)
            LCD_SDIN = 1;
        else
            LCD_SDIN = 0;
        dt = dt << 1;
        LCD_SCLK = 0;
        LCD_SCLK = 1;
    }
    LCD_DC = 1;
    LCD_SCE = 1;
    LCD_SDIN = 1;
}

//LCD清屏函数
void LCD_clear(void)
{
    unsigned int i;

    LCD_write_byte(0x0c, 0);
    LCD_write_byte(0x80, 0);

    for (i = 0; i < 504; i++)
        LCD_write_byte(0, 1);
}

//LCD初始化
void LCD_init(void)
{
    LCD_RST = 0;
    delay_nms(10);
    LCD_RST = 1;
    LCD_write_byte(0x21, 0); // 初始化Lcd,功能设定使用扩充指令
    LCD_write_byte(0xc8, 0); // 设置偏置电压
    LCD_write_byte(0x06, 0); // 温度校正
    LCD_write_byte(0x13, 0); // 1:48
    LCD_write_byte(0x20, 0); // 使用基本命令
    LCD_clear();             // 清屏
    LCD_write_byte(0x0c, 0); // 设定显示模式，正常显示
}

//光标位置XY
void LCD_set_XY(unsigned char X, unsigned char Y)
{
    LCD_write_byte(0x40 | Y, 0); // column
    LCD_write_byte(0x80 | X, 0); // row
}

//LCD显示字库
void LCD_write_char(unsigned char c)
{
    unsigned char line;

    for (line = 0; line < 6; line++)
        LCD_write_byte(font6x8[c][line], 1);
}

int main(int argc, char **argv)
{
    unsigned char i, j;
    unsigned int adc_result = 0, V_value, sum = 0, Now_x = 0;

    int bin_num = 0x00;
    int num = 0x01;

    LATD |= 0xff;
    TRISD = 0x00;

    init_ad();
    LCD_init();
    delay_nms(10);

    LCD_set_XY(0, 4);
    while (1)
    {
        adc_result = get_result();
        V_value = adc_result * 50 / 1024; //AD获取电压

        //显示字库 绘制曲线
        LCD_set_XY(0, 5);
        LCD_write_char(12);
        LCD_write_char(14);
        LCD_write_char(13);
        LCD_write_char(10);
        LCD_write_char(V_value / 10);
        LCD_write_char(11);
        LCD_write_char(V_value % 10);
        LCD_write_char(15);

        if (V_value < 8)
        {
            LCD_set_XY(Now_x, 4);
            if (V_value == 0)
                LCD_write_byte(0x80, 1);
            else if (V_value == 1)
                LCD_write_byte(0x40, 1);
            else if (V_value == 2)
                LCD_write_byte(0x20, 1);
            else if (V_value == 3)
                LCD_write_byte(0x10, 1);
            else if (V_value == 4)
                LCD_write_byte(0x08, 1);
            else if (V_value == 5)
                LCD_write_byte(0x04, 1);
            else if (V_value == 6)
                LCD_write_byte(0x02, 1);
            else if (V_value == 7)
                LCD_write_byte(0x01, 1);
        }
        else if (V_value < 16)
        {
            V_value = V_value - 8;
            LCD_set_XY(Now_x, 3);
            if (V_value == 0)
                LCD_write_byte(0x80, 1);
            else if (V_value == 1)
                LCD_write_byte(0x40, 1);
            else if (V_value == 2)
                LCD_write_byte(0x20, 1);
            else if (V_value == 3)
                LCD_write_byte(0x10, 1);
            else if (V_value == 4)
                LCD_write_byte(0x08, 1);
            else if (V_value == 5)
                LCD_write_byte(0x04, 1);
            else if (V_value == 6)
                LCD_write_byte(0x02, 1);
            else if (V_value == 7)
                LCD_write_byte(0x01, 1);
        }
        else if (V_value < 24)
        {
            V_value = V_value - 16;
            LCD_set_XY(Now_x, 2);
            if (V_value == 0)
                LCD_write_byte(0x80, 1);
            else if (V_value == 1)
                LCD_write_byte(0x40, 1);
            else if (V_value == 2)
                LCD_write_byte(0x20, 1);
            else if (V_value == 3)
                LCD_write_byte(0x10, 1);
            else if (V_value == 4)
                LCD_write_byte(0x08, 1);
            else if (V_value == 5)
                LCD_write_byte(0x04, 1);
            else if (V_value == 6)
                LCD_write_byte(0x02, 1);
            else if (V_value == 7)
                LCD_write_byte(0x01, 1);
        }
        else if (V_value < 32)
        {
            V_value = V_value - 24;
            LCD_set_XY(Now_x, 1);
            if (V_value == 0)
                LCD_write_byte(0x80, 1);
            else if (V_value == 1)
                LCD_write_byte(0x40, 1);
            else if (V_value == 2)
                LCD_write_byte(0x20, 1);
            else if (V_value == 3)
                LCD_write_byte(0x10, 1);
            else if (V_value == 4)
                LCD_write_byte(0x08, 1);
            else if (V_value == 5)
                LCD_write_byte(0x04, 1);
            else if (V_value == 6)
                LCD_write_byte(0x02, 1);
            else if (V_value == 7)
                LCD_write_byte(0x01, 1);
        }
        else if (V_value < 40)
        {
            V_value = V_value - 32;
            LCD_set_XY(Now_x, 0);
            if (V_value == 0)
                LCD_write_byte(0x80, 1);
            else if (V_value == 1)
                LCD_write_byte(0x40, 1);
            else if (V_value == 2)
                LCD_write_byte(0x20, 1);
            else if (V_value == 3)
                LCD_write_byte(0x10, 1);
            else if (V_value == 4)
                LCD_write_byte(0x08, 1);
            else if (V_value == 5)
                LCD_write_byte(0x04, 1);
            else if (V_value == 6)
                LCD_write_byte(0x02, 1);
            else if (V_value == 7)
                LCD_write_byte(0x01, 1);
        }

        Now_x = Now_x + 1;
        if (Now_x == 80)
        {
            Now_x = 0;
            LCD_clear();
        }

        delay_nms(50);
    }

    return 0;
}
