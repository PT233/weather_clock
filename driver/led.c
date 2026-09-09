#include <stdbool.h>
#include "stm32f10x.h"
//#include "delay.h"

#define LED_GPIO GPIOC
#define LED_PIN GPIO_Pin_13

static bool led_state = false;
volatile uint32_t pwmValue = 0;    // PWM占空比值 (0~100)
volatile int8_t direction = 1;     // 渐变方向：1=渐亮，-1=渐暗

void led_init(void)
{
    GPIO_InitTypeDef GPIO_INIT;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    GPIO_INIT.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_INIT.GPIO_Pin = LED_PIN;
    GPIO_INIT.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED_GPIO, &GPIO_INIT);
    GPIO_SetBits(LED_GPIO, LED_PIN);

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);			//开启TIM2的时钟

	TIM_InternalClockConfig(TIM3);		//选择TIM2为内部时钟，若不调用此函数，TIM默认也为内部时钟
	
	/*时基单元初始化*/
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;				//定义结构体变量
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;     //时钟分频，选择不分频，此参数用于配置滤波器时钟，不影响时基单元功能
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; //计数器模式，选择向上计数
	TIM_TimeBaseInitStructure.TIM_Period = 100 - 1;					//计数周期，即ARR的值
	TIM_TimeBaseInitStructure.TIM_Prescaler = 720 - 1;				//预分频器，即PSC的值
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;            //重复计数器，高级定时器才会用到
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStructure);             //将结构体变量交给TIM_TimeBaseInit，配置TIM2的时基单元
	
	/*输出比较初始化*/
	TIM_OCInitTypeDef TIM_OCInitStructure;							//定义结构体变量
	TIM_OCStructInit(&TIM_OCInitStructure);							//结构体初始化，若结构体没有完整赋值
																	//则最好执行此函数，给结构体所有成员都赋一个默认值
																	//避免结构体初值不确定的问题
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;				//输出比较模式，选择PWM模式1
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;		//输出极性，选择为高，若选择极性为低，则输出高低电平取反
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;	//输出使能
	TIM_OCInitStructure.TIM_Pulse = 0;								//初始的CCR值
	TIM_OC1Init(TIM3, &TIM_OCInitStructure);	
    
    TIM_Cmd(TIM3, ENABLE);
}
void led_set(bool on)
{
    led_state = on;
    if(led_state == true)
    {
        GPIO_ResetBits(LED_GPIO, LED_PIN);//灭灯
    }
    else
    {
        GPIO_SetBits(LED_GPIO, LED_PIN);//开灯
    }
    //后续学习   GPIO_WriteBit(LED_PORT, LED_PIN, on ? Bit_RESET : Bit_SET);
}
void led_on(void)
{
    led_set(true);
}
void led_off(void)
{
    led_set(false);
}
void led_toggle(void)
{
    led_set(!led_state);

}
//void led_pwm(uint32_t value)
//{
//    PWM_SetCompare(value);
//}
