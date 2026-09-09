#include <stdbool.h>
#include <stdint.h>
#include "stm32f10x.h"
#include "delay.h"

// 软件I2C引脚定义
#define SCL_PORT    GPIOB
#define SCL_PIN     GPIO_Pin_6
#define SDA_PORT    GPIOB
#define SDA_PIN     GPIO_Pin_7

#define SCL_HIGH()  GPIO_SetBits(SCL_PORT, SCL_PIN)
#define SCL_LOW()   GPIO_ResetBits(SCL_PORT, SCL_PIN)
#define SDA_HIGH()  GPIO_SetBits(SDA_PORT, SDA_PIN)
#define SDA_LOW()   GPIO_ResetBits(SDA_PORT, SDA_PIN)
#define SDA_READ()  GPIO_ReadInputDataBit(SDA_PORT, SDA_PIN)
#define DELAY()     delay_us(5)

/**
 * @brief I2C开始信号，在SCL高电平期间，SDA产生一个下降沿，其后SCL拉低
 *
 */
static void i2c_start(void)
{
    SDA_HIGH();
    SCL_HIGH();
    DELAY();

    SDA_LOW();
    DELAY();
    SCL_LOW();

}
/**
 * @brief I2C停止信号，在SCL高电平期间，SDA产生一个上升沿
 *
 */
static void i2c_stop(void)
{
    SDA_LOW();
    DELAY();
    SCL_HIGH();
    DELAY();

    SDA_HIGH();
    DELAY();
}
/**
 * @brief I2C写一个字节 1 BYTE = 8 BIT
 *
 * @param data 要写入的数据
 * @return true 写入成功
 * @return false 写入失败
 */
static bool i2c_write_byte(uint8_t data)
{
    for(uint8_t i = 0; i < 8; i++)
    {
        if(data & 0x80)//&1000 0000判断最高位是否为1
        {
            SDA_HIGH();
        }
        else
        {
            SDA_LOW();
        }
        DELAY();
        SCL_HIGH();
        DELAY();
        SCL_LOW();
        data <<= 1;//左移一位，将下一位数据移动到最高位，以便在下一次循环中发送。
    }
    SDA_HIGH();
    DELAY();
    SCL_HIGH();
    DELAY();
    if(SDA_READ())
    {
        SCL_LOW();
        return false;
    }
    SCL_LOW();
    DELAY();
    return true;
}
/**
 * @brief I2C读一个字节
 *
 * @param ack 是否发送ACK应答位
 * @return uint8_t 读取到的数据
 */
static uint8_t i2c_read_byte(bool ack)
{
    uint8_t data = 0;
    SDA_HIGH();
    for(uint8_t i = 0; i < 8; i++)
    {
        data <<= 1;
        SCL_HIGH();
        DELAY();
        if(SDA_READ())
        {
            data |= 0x01;//0000 0001 判断最低位是否为1
        }
        SCL_LOW();
        DELAY();
    }
    if(ack)
    {
        SDA_LOW();
    }
    else
    {
        SDA_HIGH();
    }
    DELAY();
    SCL_HIGH();
    DELAY();
    SCL_LOW();
    SDA_HIGH();
    DELAY();
    return data;
}

void i2c_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);//后续删除
    GPIO_InitStructure.GPIO_Pin = SCL_PIN | SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(SCL_PORT, &GPIO_InitStructure);
    GPIO_Init(SDA_PORT, &GPIO_InitStructure);
}

void swi2c_init(void)
{
    i2c_init();
}
/**
 * @brief I2C写数据
 *
 * @param addr 从机地址
 * @param reg 寄存器地址
 * @param data 要写入的数据
 * @param length 数据长度
 * @return true 写入成功
 * @return false 写入失败
 */
bool swi2c_write(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t length)
{
    i2c_start();

    if(!i2c_write_byte(addr << 1))
    {
        i2c_stop();
        return false;
    }
    i2c_write_byte(reg);
    for(uint16_t i = 0; i < length; i++)
    {
        i2c_write_byte(data[i]);
    }
    i2c_stop();
    return true;
}
/**
 * @brief I2C读数据
 *
 * @param addr 从机地址
 * @param reg 寄存器地址
 * @param data 读取到的数据
 * @param length 数据长度
 * @return true 读取成功
 * @return false 读取失败
 */
bool swi2c_read(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t length)
{
    i2c_start();

    if(!i2c_write_byte(addr << 1))
    {
        i2c_stop();
        return false; 
    }
    i2c_write_byte(reg);
    i2c_start();

    i2c_write_byte((addr << 1) | 0x01);//读写位位于最低位（LSB），0表示写操作，1表示读操作
    for(uint16_t i = 0; i < length; i++)
    {
        data[i] = i2c_read_byte(i == length - 1 ? false : true);
    }
    i2c_stop();
    return true;
}
