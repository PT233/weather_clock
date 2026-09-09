#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "stm32f10x.h"
#include "rtc.h"

static bool date_validate(const rtc_date_t *date)//判断时间是否合法
{
    if(date->year < 1970 || date->year > 2099)
        return false;
    if(date->month < 1 || date->month > 12)
        return false;
    if(date->day < 1 || date->day > 31)
        return false;
    if(date->hour > 23)
        return false;
    if(date->minute > 59)
        return false;
    if(date->second > 59)
        return false;

    return true;
}

uint32_t date_to_ts(const rtc_date_t *date)//将时间转换为时间戳TimeStamp
{
    uint16_t year = date->year;//65535
    uint8_t month = date->month;//255
    uint8_t day = date->day;
    uint8_t hour = date->hour;
    uint8_t minute = date->minute;
    uint8_t second = date->second;
    
    uint64_t ts = 0;//
    month -= 2;
    if((uint8_t)month <=0)
    {
        year -= 1;
        month += 12;
    }
    /* 计算时间戳 */
    ts = (((year / 4 - year / 100 + year / 400 + 367 * month / 12 + day + year * 365 - 719499) * 24 +
    hour) * 60 + minute) * 60 + second;

    return ts;
}


void ts_to_date(uint32_t seconds, rtc_date_t *date)
{
    uint32_t leapyears = 0, yearhours =0;
    const uint32_t monthdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const uint16_t ONE_YEAR_HOURS = 8760 ;

    memset(date, 0, sizeof(rtc_date_t));//timestamp设置为UTC时间
    /* 秒 */
    date->second = seconds % 60;//总秒数取余60得到时钟显示秒数
    seconds /= 60;              //总秒数除以60得到分钟数

    /* 分 */
    date->minute = seconds % 60;//总分钟数取余60得到时钟显示分钟数
    seconds /= 60;              //总分钟数除以60得到小时数

    /* 年 */
    leapyears = seconds / (1461 * 24);      //365*4+1=1461,得到闰年数leapyears
    date->year = (leapyears << 2) + 1970;   //leapyears*4+1970得到年份,不需要考虑100年闰月的情况，2100年不是闰年
    seconds %= (1461 * 24);                 //seconds 代表的是在经过了若干个完整的 4 年周期（包含 3 个平年和 1 个闰年）之后，剩余的秒数

    for(;;)
    {
        yearhours = ONE_YEAR_HOURS;//设置标准一年的小时数
        if(date->year % 4 == 0)    //如果是闰年
        yearhours += 24;           //闰年多一天
        if(seconds < yearhours)    //如果当前天数小于一年的小时数
        break;
        date->year++;              //年份加一
        seconds -= yearhours;      //总天数减去一年的小时数
    }
    /* 小时 */
    date->hour = seconds % 24;    //小时数
    seconds /= 24;               //天数除以24得到天数
    seconds++;                   //天数是从 0 开始计数的

    /* 闰年 */
    if((date->year % 4) == 0)
    {
        if(seconds > 60)//如果剩余天数大于 60 天，说明已经过了 2 月 29 日，将天数减 1；
        {
            seconds--;
        }
        else
        {
            if(seconds == 60)//如果剩余天数等于 60 天，说明就是 2 月 29 日，将月份设置为 2，日期设置为 29，函数返回；
            {
                date->month = 2;
                date->day = 29;
                return;
            }
        }
    }

    /* 月 */
    for(date->month = 0; monthdays[date->month] < seconds; date->month++)
        seconds -= monthdays[date->month];
    date->month++;  //月数是从 0 开始计数的
    //使用 for 循环逐月减去每个月的天数，直到剩余天数小于当前月的天数，此时得到月份。
    //最后剩余的天数即为日期。
    /* 日 */
    date->day = seconds;
}

void rtc_init(void)
{
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
    RCC_RTCCLKCmd(ENABLE);
    RTC_WaitForSynchro();
    RTC_WaitForLastTask();
    RTC_SetPrescaler(32768 - 1);//设置RTC时钟预分频值为32767.1Hz
    RTC_WaitForLastTask();
}

void rtc_set_date(rtc_date_t *date)
{
    if(!date_validate(date))
        return;
    uint32_t ts = date_to_ts(date);
    RTC_WaitForLastTask();
    RTC_SetCounter(ts);
    RTC_WaitForLastTask();
}

void rtc_get_date(rtc_date_t *date)
{
    uint32_t ts = RTC_GetCounter();
    if(date)//如果date不为空指针
        ts_to_date(ts, date);
}

void rtc_set_timestamp(uint32_t timestamp)
{
    RTC_WaitForLastTask();
    RTC_SetCounter(timestamp);//设置RTC计数器的值
    RTC_WaitForLastTask();
}

void rtc_get_timestamp(uint32_t *timestamp)
{
    if(timestamp)
        *timestamp = RTC_GetCounter();//获取RTC计数器的值
}

