#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "delay.h"
#include "swi2c.h"
#include "mpu6050.h"


/* MPU6050寄存器地址，只列出了使用到的部分 */
#define WHO_AM_I_REG        0x75    /* 读出来的值应该是0x68 */
#define PWR_MGMT_1_REG      0x6B    //Power Management
#define SMPLRT_DIV_REG      0x19    //Sample Rate Divider
#define ACCEL_CONFIG_REG    0x1C    //Accelerometer Configuration
#define ACCEL_XOUT_H_REG    0x3B    //Accelerometer X-axis
#define TEMP_OUT_H_REG      0x41    //Temperature
#define GYRO_CONFIG_REG     0x1B    //Gyroscope Configuration陀螺仪配置
#define GYRO_XOUT_H_REG     0x43    //Gyroscope X-axis

/* MPU6050器件地址 */
#define MPU6050_ADDR    0x68    //它是 MPU - 6050 传感器默认的 I2C 从机地址

/* MPU6050器件地址 */
#define MPU6050_ADDR    0x68

/**
 * @brief 初始化MPU6050
 *
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool mpu6050_init(void)
{
    /* 初始化软件I2C */
    swi2c_init();

    /* 读取WHO_AM_I寄存器，检查MPU6050是否能够正常通信 */
    uint8_t whoami;
    swi2c_read(MPU6050_ADDR, WHO_AM_I_REG, &whoami, 1);

    /* 如果WHO_AM_I寄存器的值不是0x68，则表示通信失败 */
    if (whoami != 0x68)
    {
        return false;
    }

    uint8_t data;

    /* 复位MPU6050 */
    data = 0;
    swi2c_write(MPU6050_ADDR, PWR_MGMT_1_REG, &data, 1);

    /* 等待MPU6050复位完成 */
    delay_ms(100);

    /* 设置采样率为1kHz 8kHz / (1 + 7) = 1kHz*/
    data = 0x07;
    swi2c_write(MPU6050_ADDR, SMPLRT_DIV_REG, &data, 1);

    /* 设置加速度计量程为2g */
    data = 0x00;
    swi2c_write(MPU6050_ADDR, ACCEL_CONFIG_REG, &data, 1);

    /* 设置陀螺仪量程为250deg/s */
    data = 0x00;
    swi2c_write(MPU6050_ADDR, GYRO_CONFIG_REG, &data, 1);

    return true;
}

/**
 * @brief 读取MPU6050的加速度计数据
 *
 * @param accel 加速度计数据
 */
void mpu6050_read_accel(mpu6050_accel_t *accel)
{
    uint8_t raw_data[6];

    /* 读取加速度计数据 */
    swi2c_read(MPU6050_ADDR, ACCEL_XOUT_H_REG, raw_data, 6);
    //raw_data[]对应ACCEL_XOUT_H, ACCEL_XOUT_L, ACCEL_YOUT_H, 
    //ACCEL_YOUT_L, ACCEL_ZOUT_H, and ACCEL_ZOUT_L

    /* 将原始数据转换为实际的加速度值 */
    accel->x_raw = (int16_t)(raw_data[0] << 8 | raw_data[1]);
    accel->y_raw = (int16_t)(raw_data[2] << 8 | raw_data[3]);
    accel->z_raw = (int16_t)(raw_data[4] << 8 | raw_data[5]);
    accel->x = accel->x_raw / 16384.0f;
    accel->y = accel->y_raw / 16384.0f;
    accel->z = accel->z_raw / 16384.0f;

}

void mpu6050_read_gyro(mpu6050_gyro_t *gyro)
{
    uint8_t raw_data[6];

    /* 读取陀螺仪的原始数据 */
    swi2c_read(MPU6050_ADDR, GYRO_XOUT_H_REG, raw_data, 6);

    /* 将读取到的原始数据转换成16位有符号数 */
    gyro->x_raw = (int16_t)(raw_data[0] << 8 | raw_data[1]);
    gyro->y_raw = (int16_t)(raw_data[2] << 8 | raw_data[3]);
    gyro->z_raw = (int16_t)(raw_data[4] << 8 | raw_data[5]);

    /* 陀螺仪原始数据是16位有符号数，需要转换成浮点数 */
    gyro->x = gyro->x_raw / 131.0f;
    gyro->y = gyro->y_raw / 131.0f;
    gyro->z = gyro->z_raw / 131.0f;
}

float mpu6050_read_temper(void)
{
    uint8_t raw_data[2];

    swi2c_read(MPU6050_ADDR, TEMP_OUT_H_REG, raw_data, 2);

    int16_t temp_raw = (int16_t)(raw_data[0] << 8 | raw_data[1]);

    /* 根据温度转换公式计算温度值，公式参考mpu6050手册 */
    return (float)temp_raw / 340.0f + 36.53f;
}
