#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "stm32f10x.h"
#include "delay.h"
#include "lcd_spi.h"
#include "st7735.h"
#include "stfonts.h"


/* ST7735的SPI管脚定义 */
#define CS_PORT     GPIOA//片选
#define CS_PIN      GPIO_Pin_4
#define DC_PORT     GPIOB//数据/命令
#define DC_PIN      GPIO_Pin_1
#define RES_PORT    GPIOB//复位
#define RES_PIN     GPIO_Pin_0
#define BLK_PORT    GPIOB//背光
#define BLK_PIN     GPIO_Pin_10

/* 自己定义一个显存大小，通常大一点不容易出问题 */
#define GRAM_BUFFER_SIZE 4096

/* ST7735命令（寄存器）定义 */
#define ST7735_NOP     0x00 /* 无操作命令 */
#define ST7735_SWRESET 0x01 /* 软件复位 */
#define ST7735_RDDID   0x04 /* 读取显示器ID */
#define ST7735_RDDST   0x09 /* 读取显示状态 */

#define ST7735_SLPIN   0x10 /* 进入睡眠模式 */
#define ST7735_SLPOUT  0x11 /* 退出睡眠模式 */
#define ST7735_PTLON   0x12 /* 部分显示模式开启 */
#define ST7735_NORON   0x13 /* 正常显示模式开启 */

#define ST7735_INVOFF  0x20 /* 关闭显示反转 */
#define ST7735_INVON   0x21 /* 开启显示反转 */
#define ST7735_GAMSET  0x26 /* 设置伽马曲线 */
#define ST7735_DISPOFF 0x28 /* 关闭显示 */
#define ST7735_DISPON  0x29 /* 打开显示 */
#define ST7735_CASET   0x2A /* 设置列地址 */
#define ST7735_RASET   0x2B /* 设置行地址 */
#define ST7735_RAMWR   0x2C /* 写入显存 */
#define ST7735_RAMRD   0x2E /* 读取显存 */

#define ST7735_PTLAR   0x30 /* 设置部分显示区域 */
#define ST7735_COLMOD  0x3A /* 设置像素格式 */
#define ST7735_MADCTL  0x36 /* 设置内存数据访问控制 */

#define ST7735_FRMCTR1 0xB1 /* 帧率控制（普通模式） */
#define ST7735_FRMCTR2 0xB2 /* 帧率控制（空闲模式） */
#define ST7735_FRMCTR3 0xB3 /* 帧率控制（部分模式） */
#define ST7735_INVCTR  0xB4 /* 显示反转控制 */
#define ST7735_DISSET5 0xB6 /* 显示设置 */

#define ST7735_PWCTR1  0xC0 /* 电源控制 1 */
#define ST7735_PWCTR2  0xC1 /* 电源控制 2 */
#define ST7735_PWCTR3  0xC2 /* 电源控制 3 */
#define ST7735_PWCTR4  0xC3 /* 电源控制 4 */
#define ST7735_PWCTR5  0xC4 /* 电源控制 5 */
#define ST7735_VMCTR1  0xC5 /* VCOM 控制 1 */

//#define ST7735_RDID1   0xDA /* 读取 ID 1 */
//#define ST7735_RDID2   0xDB /* 读取 ID 2 */
//#define ST7735_RDID3   0xDC /* 读取 ID 3 */
//#define ST7735_RDID4   0xDD /* 读取 ID 4 */

//#define ST7735_PWCTR6  0xFC /* 电源控制 6 */

#define ST7735_GMCTRP1 0xE0 /* 正伽马校正 */
#define ST7735_GMCTRN1 0xE1 /* 负伽马校正 */

#define CMD_DELAY      0xFF /* 自己增加的命令，用于执行延迟动作 */
#define CMD_EOF        0xFF /* 自己增加的命令，表示命令表结束 */


/* st7735初始化命令表 */
static const uint8_t init_cmd_list[] =
{
    /* 命令 参数长度 参数 */
    0x11,  0,  /* 退出睡眠模式 */
    CMD_DELAY, 12,  /* 延迟120ms */
    0xB1,  3,  0x01, 0x2C, 0x2D,  /* 帧率控制（普通模式）：频率分频，周期，分频 */
    0xB2,  3,  0x01, 0x2C, 0x2D,  /* 帧率控制（空闲模式）：频率分频，周期，分频 */
    0xB3,  6,  0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D,  /* 帧率控制（部分模式）：频率分频，周期，分频 */
    0xB4,  1,  0x07,  /* 显示反转控制：反转模式 */
    0xC0,  3,  0xA2, 0x02, 0x84,  /* 电源控制1：AVDD, VCL, VGH */
    0xC1,  1,  0xC5,  /* 电源控制2：VGH, VGL */
    0xC2,  2,  0x0A, 0x00,  /* 电源控制3：Opamp current small, Boost frequency */
    0xC3,  2,  0x8A, 0x2A,  /* 电源控制4：Opamp current small, Boost frequency */
    0xC4,  2,  0x8A, 0xEE,  /* 电源控制5：Opamp current small, Boost frequency */
    0xC5,  1,  0x0E,  /* VCOM控制1：VCOMH, VCOML */
    0x36,  1,  0xC8,  /* 内存数据访问控制：扫描方向 */
    0xE0,  16, 0x0F, 0x1A, 0x0F, 0x18, 0x2F, 0x28, 0x20, 0x22, 0x1F, 0x1B, 0x23, 0x37, 0x00, 0x07, 0x02, 0x10,  /* 正伽马校正 */
    0xE1,  16, 0x0F, 0x1B, 0x0F, 0x17, 0x33, 0x2C, 0x29, 0x2E, 0x30, 0x30, 0x39, 0x3F, 0x00, 0x07, 0x03, 0x10,  /* 负伽马校正 */
    0x2A,  4,  0x00, 0x00, 0x00, 0x7F,  /* 列地址设置：起始列，高8位，低8位，结束列，高8位，低8位 */
    0x2B,  4,  0x00, 0x00, 0x00, 0x9F,  /* 行地址设置：起始行，高8位，低8位，结束行，高8位，低8位 */
    0xF6,  1,  0x00,  /* 接口控制 */
    0x3A,  1,  0x05,  /* 像素格式设置：16位/像素 */
    0x29,  0,  /* 打开显示 */

    CMD_DELAY, CMD_EOF, /* 命令表结束 */
};

/* SPI-DMA传输完成标志 */
static volatile bool spi_async_done;
/* 显存缓冲区，图像内容先更新到这个缓冲区，然后一次性写入到LCD */
static uint8_t gram_buff[GRAM_BUFFER_SIZE];


/* SPI-DMA传输完成回调函数 */
static void spi_on_async_finish(void)
{
    /* 标志传输完成 */
    spi_async_done = true;
}

/* LCD-CS 片选 */
static void st7735_select(void)
{
    /* 片选信号低电平有效 */
    GPIO_WriteBit(CS_PORT, CS_PIN, Bit_RESET);
}

/* LCD-CS 片选 */
void st7735_unselect(void)
{
    /* 片选信号高电平失效 */
    GPIO_WriteBit(CS_PORT, CS_PIN, Bit_SET);
}

/* LCD硬复位 */
static void st7735_reset(void)
{
    /* 复位信号低电平有效，复位后延迟150ms */
    GPIO_WriteBit(RES_PORT, RES_PIN, Bit_RESET);
    delay_ms(2); /* 根据st7735说明，复位时间最少2ms */
    GPIO_WriteBit(RES_PORT, RES_PIN, Bit_SET);
    delay_ms(150);
}

static void st7735_bl_on(void)
{
    GPIO_WriteBit(BLK_PORT, BLK_PIN, Bit_SET);
}

static void st7735_bl_off(void)
{
    GPIO_WriteBit(BLK_PORT, BLK_PIN, Bit_RESET);
}

static void st7735_write_cmd(uint8_t cmd)
{
    GPIO_WriteBit(DC_PORT, DC_PIN, Bit_RESET);
    lcd_spi_write(&cmd, 1);
}

static void st7735_write_data(uint8_t *data, size_t size)
{
    GPIO_WriteBit(DC_PORT, DC_PIN, Bit_SET);
    spi_async_done = false;
    lcd_spi_write_async(data, size);
    while (!spi_async_done);
}

static void st7735_exec_cmds(const uint8_t *cmd_list)
{
    while (1)
    {
        uint8_t cmd = *cmd_list++;//解引用指针，命令
        uint8_t num = *cmd_list++;//解引用指针，参数个数
        if (cmd == CMD_DELAY)
        {
            if (num == CMD_EOF)
                break;             //跳出while循环
            else
                delay_ms(num * 10);
        }
        else
        {
            st7735_write_cmd(cmd);
            if (num > 0) 
            {
                st7735_write_data((uint8_t *)cmd_list, num);
            }
            cmd_list += num;
        }
    }
}

static void st7735_set_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    // column address set
    st7735_write_cmd(ST7735_CASET);
    uint8_t data[] = {0x00, x0 + ST7735_XSTART, 0x00, x1 + ST7735_XSTART};
    //先从x坐标定义
    st7735_write_data(data, sizeof(data));

    // row address set
    st7735_write_cmd(ST7735_RASET);
    data[1] = y0 + ST7735_YSTART;
    data[3] = y1 + ST7735_YSTART;
    st7735_write_data(data, sizeof(data));

    // write to RAM
    st7735_write_cmd(ST7735_RAMWR);
}

/* ST7735液晶屏GPIO引脚初始化函数 */
static void st7735_pin_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /******************** CS（片选）引脚配置 ********************/
    GPIO_InitStructure.GPIO_Pin = CS_PIN;            // 选择片选引脚
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // 设置GPIO速度为50MHz（高速模式）
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  // 推挽输出模式（强驱动能力）
    GPIO_Init(CS_PORT, &GPIO_InitStructure);          // 应用配置到CS端口
    GPIO_WriteBit(CS_PORT, CS_PIN, Bit_SET);          // 初始置高电平（默认不选中屏幕）

    /******************** DC（数据/命令）引脚配置 ********************/
    GPIO_InitStructure.GPIO_Pin = DC_PIN;            // 选择数据/命令控制引脚
    // 复用之前的配置参数（速度50MHz，推挽输出）
    GPIO_Init(DC_PORT, &GPIO_InitStructure);          // 应用配置到DC端口
    GPIO_WriteBit(DC_PORT, DC_PIN, Bit_RESET);        // 初始置低电平（默认命令模式）

    /******************** RES（复位）引脚配置 ********************/
    GPIO_InitStructure.GPIO_Pin = RES_PIN;           // 选择硬件复位引脚
    GPIO_Init(RES_PORT, &GPIO_InitStructure);         // 应用配置到RES端口
    GPIO_WriteBit(RES_PORT, RES_PIN, Bit_RESET);      // 初始置低电平（准备后续复位操作）

    /******************** BLK（背光控制）引脚配置 ********************/
    GPIO_InitStructure.GPIO_Pin = BLK_PIN;           // 选择背光控制引脚
    GPIO_Init(BLK_PORT, &GPIO_InitStructure);         // 应用配置到BLK端口
    GPIO_WriteBit(BLK_PORT, BLK_PIN, Bit_RESET);      // 初始置低电平（默认关闭背光）
}

void st7735_init()
{
    lcd_spi_init();
    lcd_spi_send_finish_register(spi_on_async_finish);
    st7735_pin_init();

    st7735_reset();

    st7735_select();
    st7735_exec_cmds(init_cmd_list);
    st7735_unselect();

    st7735_bl_on();
}

/**
 * @brief 在ST7735屏幕上绘制一个指定颜色的像素点
 * @param x: 像素点的X坐标(0起始)
 * @param y: 像素点的Y坐标(0起始)
 * @param color: RGB565格式的颜色值(16位)
 *
 * 该函数执行以下操作：
 * 1. 边界检查 - 确保坐标在屏幕有效范围内
 * 2. 颜色数据转换 - 将16位颜色值拆分为高/低字节
 * 3. SPI通信设置 - 选择芯片并设置写操作窗口
 * 4. 数据传输 - 发送颜色数据到显存
 * 5. 释放SPI总线 - 完成操作后取消芯片选择
 */
void st7735_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    // 检查坐标是否超出屏幕分辨率（ST7735_WIDTH/HEIGHT在头文件中定义）
    if (x >= ST7735_WIDTH || y >= ST7735_HEIGHT)
        return;

    // 将16位颜色值转换为2字节数组[高字节, 低字节]
    uint8_t data[] = {color >> 8, color & 0xFF};//因为是“，”语句没有结束所以color可以两次计算

    // 启动SPI通信
    st7735_select();                     // 拉低CS引脚选择芯片
    
    // 设置要写入的显示区域（单个像素：x,y到x+1,y+1）
    st7735_set_window(x, y, x + 1, y + 1);
    
    // 发送颜色数据（2字节）
    st7735_write_data(data, sizeof(data));
    
    // 结束SPI通信
    st7735_unselect();                   // 释放CS引脚
}


/**
 * @brief 在字体库中查找指定字符对应的字形数据
 * 
 * @param font  字体结构体指针，包含字体信息及数据
 * @param ch    需要查找的字符
 * @return const uint8_t* 返回字符对应的字形数据指针，未找到返回NULL
 */
static const uint8_t *st7735_find_font(const st_fonts_t *font, char ch)
{
    /* 计算每行像素数据占用的字节数（位对齐处理）：
       (width +7)/8 相当于对宽度进行8位对齐的向上取整，
       例如9像素宽度需要2个字节（8+1） */
    uint32_t bytes_per_line = (font->width + 7) / 8;//2 =（7+8）/8

    /* 遍历字体库中所有预存的字符 */
    for (uint32_t i = 0; i < font->count; i++)
    {
        /* 计算第i个字符数据的起始地址：
           每个字符数据块大小为 [1字节字符码] + [height行像素 × 每行字节数]
           +1 用于跳过首字节的ASCII码存储位置 */
        const uint8_t *pcode = font->data + i * (font->height * bytes_per_line + 1);//通常，字模数据是按行存储的，也就是一行一行地记录像素点的状态
        
        /* 检查当前字符块的ASCII码是否匹配目标字符 */
        if (*pcode == ch)
        {
            /* 匹配成功，返回字形数据指针（跳过首字节的ASCII码） */
            return pcode + 1;
        }
    }

    /* 未找到对应字符时返回空指针 */
    return NULL;
}


/**
 * @brief 在指定位置绘制单个字符
 * @param x     起始列坐标
 * @param y     起始行坐标
 * @param ch    要显示的ASCII字符
 * @param font  使用的字体结构体指针
 * @param color 字体颜色（RGB565格式）
 * @param bgcolor 背景颜色（RGB565格式）
 */
void st7735_write_char(uint16_t x, uint16_t y, char ch, st_fonts_t *font, uint16_t color, uint16_t bgcolor)
{
    st7735_select();  // 使能LCD片选

    // 设置字符显示窗口（列x到x+字体宽度-1，行y到y+字体高度-1）
    st7735_set_window(x, y, x + font->width - 1, y + font->height - 1);

    uint32_t bytes_per_line = (font->width + 7) / 8; // 计算每行占用的字节数（位压缩存储）
    
    uint8_t *pbuff = gram_buff; // 指向GRAM缓冲区的指针
    const uint8_t *fcode = st7735_find_font(font, ch); // 获取字符的字模数据
    
    // 遍历字体的每一行
    for (uint32_t y = 0; y < font->height; y++)
    {
        const uint8_t *pcode = fcode + y * bytes_per_line; // 当前行的字模数据起始位置
        // 遍历当前行的每个像素
        for (uint32_t x = 0; x < font->width; x++)
        {
            uint8_t b = pcode[x >> 3]; // 获取当前字节（每字节存储8个像素信息）

            // 检查当前bit位是否为1（通过左移操作判断最高位）
            if ((b << (x & 0x7)) & 0x80) 
            //用于确定当前像素在字节中的位置（0 - 7）。b << (x & 0x7) 将当前字节左移，
            //使得当前x像素对应的 bit 位移动到最高位。然后，通过 & 0x80 操作检查最高位是否为 1。
            //如果最高位为 1，表示该像素需要显示前景色；否则，显示背景色。
            {      
                // 前景色：将16位颜色拆分为高8位和低8位
                *pbuff++ = color >> 8;    // 颜色高字节
                *pbuff++ = color & 0xFF;  // 颜色低字节
            }
            else
            {
                // 背景色
                *pbuff++ = bgcolor >> 8;
                *pbuff++ = bgcolor & 0xFF;
            }
        }
    }

    // 将GRAM缓冲区中的数据写入LCD（计算实际写入的数据长度）
    st7735_write_data(gram_buff, pbuff - gram_buff);

    st7735_unselect(); // 关闭LCD片选
}

/**
 * @brief 在指定位置绘制字符串（支持自动换行）
 * @param str 要显示的字符串指针
 */
void st7735_write_string(uint16_t x, uint16_t y, const char *str, st_fonts_t *font, uint16_t color, uint16_t bgcolor)
{
    while (*str!= '\0')
    {
        // 处理换行逻辑
        if (x + font->width >= ST7735_WIDTH) // 超出屏幕宽度时换行
        {
            x = 0;
            y += font->height;
            if (y + font->height >= ST7735_HEIGHT) // 超出屏幕高度时停止绘制
            {
                break;
            }

            // 跳过换行后的空格字符
            if (*str == ' ')
            {
                str++;
                continue;
            }
        }

        // 绘制当前字符并更新位置
        st7735_write_char(x, y, *str, font, color, bgcolor);
        x += font->width; // 移动到下一个字符位置
        str++;
    }
}

/**
 * @brief 绘制指定字体索引的字符（包装函数）
 * @param index 字体库中的字符索引
 */
void st7735_write_font(uint16_t x, uint16_t y, st_fonts_t *font, uint32_t index, uint16_t color, uint16_t bgcolor)
{
    st7735_write_char(x, y, index, font, color, bgcolor);
}

/**
 * @brief 连续绘制多个字体字符
 * @param index 起始字符索引
 * @param count 要绘制的字符数量
 */
void st7735_write_fonts(uint16_t x, uint16_t y, st_fonts_t *font, uint32_t index, uint32_t count, uint16_t color, uint16_t bgcolor)
{
    for (uint32_t i = index; i < count && i < font->count; i++)
    {
        // 处理换行逻辑
        if (x + font->width >= ST7735_WIDTH)
        {
            x = 0;
            y += font->height;
            if (y + font->height >= ST7735_HEIGHT)
            {
                break;
            }
        }
        st7735_write_font(x, y, font, i, color, bgcolor);
        x += font->width;
    }
}

/**
 * @brief 填充矩形区域
 * @param w 矩形宽度
 * @param h 矩形高度
 */
void st7735_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    // 边界检查
    if (x >= ST7735_WIDTH || y >= ST7735_HEIGHT)
        return;
    // 调整超出屏幕的尺寸
    if (x + w - 1 >= ST7735_WIDTH)
        w = ST7735_WIDTH - x;
    if (y + h - 1 >= ST7735_HEIGHT)
        h = ST7735_HEIGHT - y;

    st7735_select();
    st7735_set_window(x, y, x + w - 1, y + h - 1);

    // 分块填充（适用于大尺寸填充，节省内存）
    for (uint32_t i = 0; i < w * h; i += GRAM_BUFFER_SIZE / 2)
    {
        uint32_t size = w * h - i;// 计算剩余待填充的像素数
        if (size > GRAM_BUFFER_SIZE / 2)
            size = GRAM_BUFFER_SIZE / 2;
            
        // 首次填充需要初始化缓冲区
        if (i == 0)
        {
            uint8_t *pbuff = gram_buff;
            for (uint32_t j = 0; j < size; j++)
            {
                *pbuff++ = color >> 8;
                *pbuff++ = color & 0xFF;
            }
        }
        st7735_write_data(gram_buff, size * 2); // 每个像素占2字节
    }

    st7735_unselect();
}

/**
 * @brief 全屏填充
 */

void st7735_fill_screen(uint16_t color)
{
    st7735_fill_rect(0, 0, ST7735_WIDTH, ST7735_HEIGHT, color);
}

/**
 * @brief 绘制图像（RGB565格式）
 * @param data 图像数据指针（需为RGB565格式数组）
 */
void st7735_draw_image(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *data)
{
    // 边界检查
    if ((x >= ST7735_WIDTH) || (y >= ST7735_HEIGHT)) 
        return;
    if ((x + w - 1) >= ST7735_WIDTH) 
        return;
    if ((y + h - 1) >= ST7735_HEIGHT) 
        return;

    st7735_select();
    st7735_set_window(x, y, x + w - 1, y + h - 1);
    st7735_write_data((uint8_t *)data, w * h * 2); // 直接写入原始图像数据
    st7735_unselect();
}
