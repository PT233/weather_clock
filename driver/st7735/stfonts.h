#ifndef __ST_FONTS_H__
#define __ST_FONTS_H__


#include <stdint.h>


/*
 * ST7735 显示屏字体定义头文件
 * 该文件定义了用于ST7735系列屏幕的字体数据结构及外部声明
 */

// 字体描述结构体 - 用于描述字模的基本属性和数据存储
typedef struct
{
    const uint16_t width;     // 单个字符的像素宽度（单位：像素）
    const uint16_t height;    // 单个字符的像素高度（单位：像素）
    const uint8_t *data;      // 指向字模数据数组的指针（按像素存储的字形数据）
    const uint32_t count;     // 该字体包含的字符总数（ASCII字符集通常为128或256）
} st_fonts_t;

/* 外部声明字体对象 - 实际字模数据在对应的.c文件中定义 */

// 标准ASCII字体：8x16像素格式（宽x高）
// 适用于常规文本显示
extern st_fonts_t font_ascii_8x16;

// 温度专用字体：16x32像素格式
// 适用于温度数值的大尺寸显示
extern st_fonts_t font_temper_16x32;

// 时间专用字体：24x48像素格式
// 适用于时间数字的超大尺寸显示
extern st_fonts_t font_time_24x48;



#endif /* __ST_FONTS_H__ */
