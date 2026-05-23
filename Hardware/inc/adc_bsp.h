//备注：拷贝代码请加上作者信息
//作者：王海涛
//邮箱：1126471088@qq.com
//版本：V1.0.0
/********************************************************
    说明：
    1、ADC独立模式已支持。
    2、双ADC模式暂未支持。
    3、注入模式暂未支持。
    4、支持阻塞模式、中断模式、DMA模式。
    5、基于stm32f4xx标准库V1.9.0
**********************************************************/
#ifndef __ADC_BSP_H__
#define __ADC_BSP_H__

#include "gd32f4xx.h"
#include "gd32f4xx_adc.h"
#include "../Hardware/inc/gpio_bsp.h"
#include "../Hardware/inc/dma_bsp.h"

/* STM32F429IGT6 ADC通道对应的GPIO
    ADC通道       ADC1           ADC2       ADC3
    ADC_CH0       PA0            PA0        PA0
    ADC_CH1       PA1            PA1        PA1
    ADC_CH2       PA2            PA2        PA2
    ADC_CH3       PA3            PA3        PA3
    ADC_CH4       PA4            PA4        PF6
    ADC_CH5       PA5            PA5        PF7
    ADC_CH6       PA6            PA6        PF8
    ADC_CH7       PA7            PA7        PF9
    ADC_CH8       PB0            PB0        PF10
    ADC_CH9       PB1            PB1        PF3
    ADC_CH10      PC0            PC0        PC0
    ADC_CH11      PC1            PC1        PC1
    ADC_CH12      PC2            PC2        PC2
    ADC_CH13      PC3            PC3        PC3
    ADC_CH14      PC4            PC4        PF4
    ADC_CH15      PC5            PC5        PF5
    ADC_CH16      内部VSS        内部VSS    内部VSS
    ADC_CH17      内部参考电压   内部VSS    内部VSS
    ADC_CH18      内部温度或电池 内部VSS    内部VSS
*/

/*ADC号枚举*/
typedef enum
{
    ADC1_ADC2 = 0,    //双ADC模式使用
    ADC1_2_3  = 1,    //三ADC模式使用
    ADC_1 = ADC1_BASE,
    ADC_2 = ADC2_BASE,
    ADC_3 = ADC3_BASE,
}WHT_ADC_enum;

/*ADC通道*/
typedef enum
{
    ADC_CH0  = ADC_Channel_0,
    ADC_CH1  = ADC_Channel_1,
    ADC_CH2  = ADC_Channel_2,
    ADC_CH3  = ADC_Channel_3,
    ADC_CH4  = ADC_Channel_4,
    ADC_CH5  = ADC_Channel_5,
    ADC_CH6  = ADC_Channel_6,
    ADC_CH7  = ADC_Channel_7,
    ADC_CH8  = ADC_Channel_8,
    ADC_CH9  = ADC_Channel_9,
    ADC_CH10 = ADC_Channel_10,
    ADC_CH11 = ADC_Channel_11,
    ADC_CH12 = ADC_Channel_12,
    ADC_CH13 = ADC_Channel_13,
    ADC_CH14 = ADC_Channel_14,
    ADC_CH15 = ADC_Channel_15,
    ADC_CH16 = ADC_Channel_16,     //内部温度或内部VSS
    ADC_CH17 = ADC_Channel_Vrefint,//内部参考电压
    ADC_CH18 = ADC_Channel_Vbat,   //内部电池电压或内部温度
}WHT_ADC_Channel_enum;

/*ADC时钟源分频枚举*/
typedef enum
{
    APB2_Div2 = ADC_Prescaler_Div2,//if APB2=90Mhz ADC_CLK Max 30Mhz
    APB2_Div4 = ADC_Prescaler_Div4,
    APB2_Div6 = ADC_Prescaler_Div6,
    APB2_Div8 = ADC_Prescaler_Div8,
}WHT_ADC_Clk_enum;

/*ADC采样时间枚举*/
typedef enum
{
    ADC_Clk_Time_3   = ADC_SampleTime_3Cycles,  //3个ADC时钟周期
    ADC_Clk_Time_15  = ADC_SampleTime_15Cycles, //15个ADC时钟周期
    ADC_Clk_Time_28  = ADC_SampleTime_28Cycles, //28个ADC时钟周期
    ADC_Clk_Time_56  = ADC_SampleTime_56Cycles, //56个ADC时钟周期
    ADC_Clk_Time_84  = ADC_SampleTime_84Cycles, //84个ADC时钟周期
    ADC_Clk_Time_112 = ADC_SampleTime_112Cycles,//112个ADC时钟周期
    ADC_Clk_Time_144 = ADC_SampleTime_144Cycles,//144个ADC时钟周期
    ADC_Clk_Time_480 = ADC_SampleTime_480Cycles,//480个ADC时钟周期
}WHT_ADC_Sample_Time_enum;

/*双ADC模式的第一个ADC，必须是ADC1
双ADC模式的第二个ADC，必须是ADC2*/
typedef enum
{ 
    Independent                       = ADC_Mode_Independent,              //独立模式
    /*以下双重模式 ADC1和ADC2一起工作 ADC3独立*/
    DualMode_RegSimult_InjecSimult    = ADC_DualMode_RegSimult_InjecSimult,//双重模式，规则同步+注入同步
    DualMode_RegSimult_AlterTrig      = ADC_DualMode_RegSimult_AlterTrig,  //双重模式，规则同步+交替触发
    DualMode_InjecSimult              = ADC_DualMode_InjecSimult,          //双重模式，注入同步
    DualMode_RegSimult                = ADC_DualMode_RegSimult,            //双重模式，规则同步
    DualMode_Interl                   = ADC_DualMode_Interl,               //双重模式，交叉
    DualMode_AlterTrig                = ADC_DualMode_AlterTrig,            //双重模式，交替触发
    /*以下三重模式 ADC1 ADC2 ADC3一起工作*/
    TripleMode_RegSimult_InjecSimult  = ADC_TripleMode_RegSimult_InjecSimult,//三重模式，规则同步+注入同步
    TripleMode_RegSimult_AlterTrig    = ADC_TripleMode_RegSimult_AlterTrig,  //三重模式，规则同步+交替触发
    TripleMode_InjecSimult            = ADC_TripleMode_InjecSimult,          //三重模式，注入同步
    TripleMode_RegSimult              = ADC_TripleMode_RegSimult,            //三重模式，规则同步
    TripleMode_Interl                 = ADC_TripleMode_Interl,               //三重模式，交叉
    TripleMode_AlterTrig              = ADC_TripleMode_AlterTrig,            //三重模式，交替触发
}WHT_ADC_Work_Mode_enum;

/*ADC规则通道触发转换信号源*/
typedef enum
{
    Timer_1_CC1_Event  = ADC_ExternalTrigConv_T1_CC1,  //定时器1通道1输入比较事件
    Timer_1_CC2_Event  = ADC_ExternalTrigConv_T1_CC2,  //定时器1通道2输入比较事件
    Timer_1_CC3_Event  = ADC_ExternalTrigConv_T1_CC3,  //定时器1通道3输入比较事件
    Timer_2_CC2_Event  = ADC_ExternalTrigConv_T2_CC2,  //定时器2通道2输入比较事件
    Timer_2_CC3_Event  = ADC_ExternalTrigConv_T2_CC3,  //定时器2通道3输入比较事件
    Timer_2_CC4_Event  = ADC_ExternalTrigConv_T2_CC4,  //定时器2通道4输入比较事件
    Timer_2_TRGO_Event = ADC_ExternalTrigConv_T2_TRGO, //定时器2触发输出事件
    Timer_3_CC1_Event  = ADC_ExternalTrigConv_T3_CC1,  //定时器3通道1输入比较事件
    Timer_3_TRGO_Event = ADC_ExternalTrigConv_T3_TRGO, //定时器3触发输出事件
    Timer_4_CC4_Event  = ADC_ExternalTrigConv_T4_CC4,  //定时器4通道4输入比较事件
    Timer_5_CC1_Event  = ADC_ExternalTrigConv_T5_CC1,  //定时器5通道1输入比较事件
    Timer_5_CC2_Event  = ADC_ExternalTrigConv_T5_CC2,  //定时器5通道2输入比较事件
    Timer_5_CC3_Event  = ADC_ExternalTrigConv_T5_CC3,  //定时器5通道3输入比较事件
    Timer_8_CC1_Event  = ADC_ExternalTrigConv_T8_CC1,  //定时器8通道1输入比较事件
    Timer_8_TRGO_Event = ADC_ExternalTrigConv_T8_TRGO, //定时器8触发输出事件
    EXTI_Lin11_Event   = ADC_ExternalTrigConv_Ext_IT11,//外部中断线11触发事件
    Extern_Event_None,                                 //手动触发转换
}WHT_ADC_Trig_Event_enum;

/*ADC注入通道触发转换信号源*/
typedef enum
{
    J_Timer_1_CC4_Event  = ADC_ExternalTrigInjecConv_T1_CC4,  //定时器1通道4输入比较事件
    J_Timer_1_TRGO_Event = ADC_ExternalTrigInjecConv_T1_TRGO, //定时器1触发输出事件
    J_Timer_2_CC1_Event  = ADC_ExternalTrigInjecConv_T2_CC1,  //定时器2通道1输入比较事件
    J_Timer_2_TRGO_Event = ADC_ExternalTrigInjecConv_T2_TRGO, //定时器2触发输出事件
    J_Timer_3_CC2_Event  = ADC_ExternalTrigInjecConv_T3_CC2,  //定时器3通道2输入比较事件
    J_Timer_3_CC4_Event  = ADC_ExternalTrigInjecConv_T3_CC4,  //定时器3通道4输入比较事件
    J_Timer_4_CC1_Event  = ADC_ExternalTrigInjecConv_T4_CC1,  //定时器4通道1输入比较事件
    J_Timer_4_CC2_Event  = ADC_ExternalTrigInjecConv_T4_CC2,  //定时器4通道2输入比较事件
    J_Timer_4_CC3_Event  = ADC_ExternalTrigInjecConv_T4_CC3,  //定时器4通道3输入比较事件
    J_Timer_4_TRGO_Event = ADC_ExternalTrigInjecConv_T4_TRGO, //定时器4触发输出事件
    J_Timer_5_CC4_Event  = ADC_ExternalTrigInjecConv_T5_CC4,  //定时器5通道4输入比较事件
    J_Timer_5_TRGO_Event = ADC_ExternalTrigInjecConv_T5_TRGO, //定时器5触发输出事件
    J_Timer_8_CC2_Event  = ADC_ExternalTrigInjecConv_T8_CC2,  //定时器8通道2输入比较事件
    J_Timer_8_CC3_Event  = ADC_ExternalTrigInjecConv_T8_CC3,  //定时器8通道3输入比较事件
    J_Timer_8_CC4_Event  = ADC_ExternalTrigInjecConv_T8_CC4,  //定时器8通道4输入比较事件
    J_EXTI_Lin15_Event   = ADC_ExternalTrigInjecConv_Ext_IT15,//外部中断线15触发事件
}WHT_ADC_J_Trig_Event_enum;

typedef struct
{
    void* Cache_New_Addr;//缓存最新指向的缓存地址
    FlagStatus Busy_Flag;//忙碌标志,切记只读
}WHT_ADC_Info_t;

/*ADC 通道 转换顺序 采样时间结构体*/
typedef struct
{
    /*ADC通道转换顺序参考此结构体数组下标的顺序，最大支持16个配置数组*/
    /*使用范例如 WHT_ADC_Channel_Config channel[4]*/
    WHT_ADC_Channel_enum Channel;        //通道
    WHT_ADC_Sample_Time_enum Sample_Time;//通道采样时间
}WHT_ADC_Channel_Config;

/*ADC 配置结构体*/
typedef struct
{
    WHT_ADC_enum Name;                     //ADC号
    WHT_ADC_Clk_enum Clk_Div;              //时钟分频因子
    WHT_ADC_Work_Mode_enum Mode;           //工作模式选择，ADC2和ADC3只能选择独立模式
    unsigned char Channel_Count;           //采集通道数 is 1 to 16
    WHT_ADC_Trig_Event_enum ExternalTrigConv;//转换触发信号选择,一般情况下选择软件触发转换
    unsigned char PreemptionPriority;      //抢占优先级
    unsigned char SubPriority;             //子优先级
    WHT_ADC_Channel_Config* Channel_Config;//ADC通道配置
}WHT_ADC_Config_t;

typedef struct
{
    unsigned short Rx_Count; //每个通道接收个数
    unsigned short* Rx_Buf;  //数据缓存，数组的深度为为通道采集的值，行为通道号，列为ADC号。范例4个通道，每个通道采集8个数据unsigned short ADC_Buf[8][4][1]表示独立模式,ADC_Buf[8][4][2]表示双ADC模式
    volatile const WHT_ADC_Info_t* Info;//ADC状态信息
    void* Private_Data;//私有数据
}WHT_ADC_Cache_t;

typedef struct
{
    ErrStatus (*WHT_Register)(const WHT_ADC_Config_t* config, const WHT_ADC_Cache_t* cache);//注册
    void (*WHT_Config)(const WHT_ADC_Config_t* config, const WHT_ADC_Cache_t* cache);//每个设备可自行配置
    void (*WHT_Receive)(const WHT_ADC_Cache_t* cache);              //启动接收数据非中断版
    void (*WHT_Interrupt_Receive)(const WHT_ADC_Cache_t* cache);    //启动接收数据中断版
    void (*WHT_DMA_Receive)(const WHT_ADC_Cache_t* cache);          //启动接收数据DMA版
    void (*WHT_End_Interrupt_Receive)(const WHT_ADC_Cache_t* cache);//结束中断接收数据
    void (*WHT_End_DMA_Receive)(const WHT_ADC_Cache_t* cache);      //结束DMA接收数据
}WHT_ADC_BSP_t;

/*全局常量*/
extern const WHT_ADC_BSP_t WHT_ADC_BSP;


// 双模式时，ADC1和ADC2转换的数据都存放在ADC1的数据寄存器，
// ADC1的在低十六位，ADC2的在高十六位

#endif // !__ADC_BSP_H__
