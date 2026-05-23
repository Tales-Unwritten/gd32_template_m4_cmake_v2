//备注：拷贝代码请加上作者信息
//作者：王海涛
//邮箱：1126471088@qq.com
//版本：V1.0.0
#include "adc_bsp.h"
#include "gd32f4xx_misc.h"

/*ADC总管理结构体*/
typedef struct
{
	WHT_ADC_enum Name;                     //ADC号
	WHT_ADC_Info_t Info;                   //所有标志,提供给外部设备
	WHT_ADC_Cache_t* Cache;                //外设缓存,由外部设备提供
	WHT_ADC_Channel_Config* Channel_Config;//ADC通道配置
	unsigned short Channel_Count;          //采集通道个数
	unsigned short Temp_Count;             //无需关心
	WHT_DMA_Config_t DMA_Receive_Config;   //DMA配置
}WHT_Data_t;

static WHT_Data_t WHT_ADC1_ADC2;
static WHT_Data_t WHT_ADC1_ADC2_ADC3;
static WHT_Data_t WHT_ADC1;
static WHT_Data_t WHT_ADC2;
static WHT_Data_t WHT_ADC3;

#define WHT_ADC     ((WHT_Data_t*)cache->Private_Data)//简化操作


/*ADC GPIO初始化*/
static void WHT_ADC_BSP_GPIO_Init(const WHT_ADC_Config_t* config)
{
	for (unsigned char i = 0; i < config->Channel_Count; i++)
	{
		switch (config->Channel_Config[i].Channel)
		{
		case ADC_CH0:
			WHT_GPIO_BSP.WHT_Set_Clock(PortA, ENABLE);
			WHT_GPIO_BSP.WHT_Set_Mode(PortA, Pin0, Mode_Ain);
			break;
		case ADC_CH1:
			WHT_GPIO_BSP.WHT_Set_Clock(PortA, ENABLE);
			WHT_GPIO_BSP.WHT_Set_Mode(PortA, Pin1, Mode_Ain);
			break;
		case ADC_CH2:
			WHT_GPIO_BSP.WHT_Set_Clock(PortA, ENABLE);
			WHT_GPIO_BSP.WHT_Set_Mode(PortA, Pin2, Mode_Ain);
			break;
		case ADC_CH3:
			WHT_GPIO_BSP.WHT_Set_Clock(PortA, ENABLE);
			WHT_GPIO_BSP.WHT_Set_Mode(PortA, Pin3, Mode_Ain);
			break;
		case ADC_CH4:
			WHT_GPIO_BSP.WHT_Set_Clock(PortA, ENABLE);
			WHT_GPIO_BSP.WHT_Set_Mode(PortA, Pin4, Mode_Ain);
			break;
		case ADC_CH5:
			WHT_GPIO_BSP.WHT_Set_Clock(PortA, ENABLE);
			WHT_GPIO_BSP.WHT_Set_Mode(PortA, Pin5, Mode_Ain);
			break;
		case ADC_CH6:
			WHT_GPIO_BSP.WHT_Set_Clock(PortA, ENABLE);
			WHT_GPIO_BSP.WHT_Set_Mode(PortA, Pin6, Mode_Ain);
			break;
		case ADC_CH7:
			WHT_GPIO_BSP.WHT_Set_Clock(PortA, ENABLE);
			WHT_GPIO_BSP.WHT_Set_Mode(PortA, Pin7, Mode_Ain);
			break;
		case ADC_CH8:
			WHT_GPIO_BSP.WHT_Set_Clock(PortB, ENABLE);
			WHT_GPIO_BSP.WHT_Set_Mode(PortB, Pin0, Mode_Ain);
			break;
		case ADC_CH9:
			WHT_GPIO_BSP.WHT_Set_Clock(PortB, ENABLE);
			WHT_GPIO_BSP.WHT_Set_Mode(PortB, Pin1, Mode_Ain);
			break;
		case ADC_CH10:
			WHT_GPIO_BSP.WHT_Set_Clock(PortC, ENABLE);
			WHT_GPIO_BSP.WHT_Set_Mode(PortC, Pin0, Mode_Ain);
			break;
		case ADC_CH11:
			WHT_GPIO_BSP.WHT_Set_Clock(PortC, ENABLE);
			WHT_GPIO_BSP.WHT_Set_Mode(PortC, Pin1, Mode_Ain);
			break;
		case ADC_CH12:
			WHT_GPIO_BSP.WHT_Set_Clock(PortC, ENABLE);
			WHT_GPIO_BSP.WHT_Set_Mode(PortC, Pin2, Mode_Ain);
			break;
		case ADC_CH13:
			WHT_GPIO_BSP.WHT_Set_Clock(PortC, ENABLE);
			WHT_GPIO_BSP.WHT_Set_Mode(PortC, Pin3, Mode_Ain);
			break;
		case ADC_CH14:
			WHT_GPIO_BSP.WHT_Set_Clock(PortC, ENABLE);
			WHT_GPIO_BSP.WHT_Set_Mode(PortC, Pin4, Mode_Ain);
			break;
		case ADC_CH15:
			WHT_GPIO_BSP.WHT_Set_Clock(PortC, ENABLE);
			WHT_GPIO_BSP.WHT_Set_Mode(PortC, Pin5, Mode_Ain);
			break;
		default:
			break;
		}
	}
}
/*ADC中断优先级配置*/
static void NVIC_Configuration(const WHT_ADC_Config_t* config)
{
	NVIC_SetPriority(ADC_IRQn, config->PreemptionPriority);
	NVIC_EnableIRQ(ADC_IRQn);
}
/*ADC 注册*/
static ErrStatus WHT_ADC_BSP_Register(const WHT_ADC_Config_t* config, const WHT_ADC_Cache_t* cache)
{
    if ((WHT_ADC != NULL) && (cache->Info != NULL))
        return SUCCESS;

	switch (config->Name)
	{
	case ADC1_ADC2:RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_ADC2, ENABLE);((WHT_ADC_Cache_t*)cache)->Private_Data = &WHT_ADC1_ADC2;break;
	case ADC1_2_3:RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_ADC2 | RCC_APB2Periph_ADC3, ENABLE);((WHT_ADC_Cache_t*)cache)->Private_Data = &WHT_ADC1_ADC2_ADC3;break;
	case ADC_1:RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE); ((WHT_ADC_Cache_t*)cache)->Private_Data = &WHT_ADC1; break;
	case ADC_2:RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE); ((WHT_ADC_Cache_t*)cache)->Private_Data = &WHT_ADC2; break;
	case ADC_3:RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC3, ENABLE); ((WHT_ADC_Cache_t*)cache)->Private_Data = &WHT_ADC3; break;
    default: return ERROR;
	}

    WHT_ADC->Name = config->Name < ADC_1 ? ADC_1 : config->Name;
    ((WHT_ADC_Cache_t*)cache)->Info = &WHT_ADC->Info;
	WHT_ADC->DMA_Receive_Config.IT_Callback = NULL;

	NVIC_Configuration(config);

	/*注册DMA*/
	switch (config->Name)
	{
	case ADC1_ADC2:
	case ADC1_2_3:
	case ADC_1:
		WHT_ADC->DMA_Receive_Config.Stream = Stream_ADC1_1;
		WHT_ADC->DMA_Receive_Config.Channel = Channel_ADC1_1;
		break;
	case ADC_2:
		WHT_ADC->DMA_Receive_Config.Stream = Stream_ADC2_1;
		WHT_ADC->DMA_Receive_Config.Channel = Channel_ADC2_1;
		break;
	case ADC_3:
		WHT_ADC->DMA_Receive_Config.Stream = Stream_ADC3_2;
		WHT_ADC->DMA_Receive_Config.Channel = Channel_ADC3_2;
		break;
	}
	WHT_DMA_BSP.WHT_Register(&WHT_ADC->DMA_Receive_Config);
	return SUCCESS; 
}
/*ADC 双ADC配置*/
static void WHT_ADC_BSP_Dual_Config(const WHT_ADC_Config_t* config, const WHT_ADC_Cache_t* cache)
{
	ADC_InitTypeDef ADC_InitStruct;
	ADC_CommonInitTypeDef ADC_CommonInitStruct;
	WHT_ADC_Config_t Temp_Config;

	WHT_ADC->Cache = (WHT_ADC_Cache_t*)cache;
	WHT_ADC->Info.Cache_New_Addr = (void*)cache;
	WHT_ADC->Channel_Count = config->Channel_Count;
	WHT_ADC->Channel_Config = config->Channel_Config;
	
	ADC_Cmd(ADC1, DISABLE);//ADC模块内部断电
	ADC_Cmd(ADC2, DISABLE);//ADC模块内部断电

	WHT_ADC_BSP_GPIO_Init(config);     //配置ADC1的GPIO
	Temp_Config.Name = config->Name;
	Temp_Config.Channel_Count = config->Channel_Count;
	Temp_Config.Channel_Config = config->Channel_Config + config->Channel_Count;
	WHT_ADC_BSP_GPIO_Init(&Temp_Config);//配置ADC2的GPIO
	
	// -------------------ADC Common 结构体 参数 初始化------------------------
	/*独立ADC模式*/
	ADC_CommonInitStruct.ADC_Mode = config->Mode;
	/*时钟为fpclk x分频*/	
	ADC_CommonInitStruct.ADC_Prescaler = config->Clk_Div;
	/*DMA直接访问模式*/	
	ADC_CommonInitStruct.ADC_DMAAccessMode = ADC_DMAAccessMode_1;
	// 采样时间间隔	
	ADC_CommonInitStruct.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_20Cycles;  
	ADC_CommonInit(&ADC_CommonInitStruct);

	ADC_InitStruct.ADC_Resolution         = ADC_Resolution_12b;   
	ADC_InitStruct.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;//禁用外部边缘触发
	ADC_InitStruct.ADC_ScanConvMode       = ENABLE;                 //依次转换规则x里配置的通道或转换规则1里配置的通道
	ADC_InitStruct.ADC_ContinuousConvMode = ENABLE;                 //持续转换
	ADC_InitStruct.ADC_ExternalTrigConv   = config->ExternalTrigConv;
	ADC_InitStruct.ADC_DataAlign          = ADC_DataAlign_Right;    //右对齐
	ADC_InitStruct.ADC_NbrOfConversion    = config->Channel_Count;
	ADC_Init(ADC1, &ADC_InitStruct);

	ADC_InitStruct.ADC_ExternalTrigConv   = Extern_Event_None;
	ADC_Init(ADC2, &ADC_InitStruct);

	/*配置ADC1通道的转换顺序和采样时间*/
	for (unsigned char i = 0; i < WHT_ADC->Channel_Count; i++)
	{
		ADC_RegularChannelConfig(ADC1, WHT_ADC->Channel_Config[i].Channel, i + 1, WHT_ADC->Channel_Config[i].Sample_Time);
	}
	/*配置ADC2通道的转换顺序和采样时间*/
	for (unsigned char i = WHT_ADC->Channel_Count; i < WHT_ADC->Channel_Count * 2; i++)
	{
		ADC_RegularChannelConfig(ADC2, WHT_ADC->Channel_Config[i].Channel, i - WHT_ADC->Channel_Count + 1, WHT_ADC->Channel_Config[i].Sample_Time);
	}

	/*关闭ADC转速完成中断*/
	ADC_ITConfig(ADC1, ADC_IT_EOC | ADC_IT_AWD | ADC_IT_JEOC, DISABLE);
	ADC_ITConfig(ADC2, ADC_IT_EOC | ADC_IT_AWD | ADC_IT_JEOC, DISABLE);
}
/*ADC 配置*/
static void WHT_ADC_BSP_Config(const WHT_ADC_Config_t* config, const WHT_ADC_Cache_t* cache)
{
	ADC_InitTypeDef ADC_InitStruct;
	ADC_CommonInitTypeDef ADC_CommonInitStruct;

    if ((WHT_ADC == NULL) || (cache->Info == NULL))
        return;
	if (config->Name == ADC1_ADC2)
	{
		WHT_ADC_BSP_Dual_Config(config, cache);
		return;
	}
	
	WHT_ADC->Cache = (WHT_ADC_Cache_t*)cache;
	WHT_ADC->Info.Cache_New_Addr = (void*)cache;
	WHT_ADC->Channel_Count = config->Channel_Count;
	WHT_ADC->Channel_Config = config->Channel_Config;

	/*ADC模块内部断电*/
	ADC_Cmd((ADC_TypeDef*)WHT_ADC->Name, DISABLE);
	WHT_ADC_BSP_GPIO_Init(config);

	// -------------------ADC Common 结构体 参数 初始化------------------------
	/*独立ADC模式*/
	ADC_CommonInitStruct.ADC_Mode = config->Mode;
	/*时钟为fpclk x分频*/	
	ADC_CommonInitStruct.ADC_Prescaler = config->Clk_Div;
	/*禁止DMA直接访问模式*/	
	ADC_CommonInitStruct.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
	// 采样时间间隔	
	ADC_CommonInitStruct.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_20Cycles;  
	ADC_CommonInit(&ADC_CommonInitStruct);

	/*初始化配置ADC相关寄存器*/
	ADC_InitStruct.ADC_Resolution         = ADC_Resolution_12b;   
	ADC_InitStruct.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;//禁用外部边缘触发
	ADC_InitStruct.ADC_ScanConvMode       = ENABLE;                 //依次转换规则x里配置的通道或转换规则1里配置的通道
	ADC_InitStruct.ADC_ContinuousConvMode = ENABLE;                 //持续转换
	ADC_InitStruct.ADC_ExternalTrigConv   = config->ExternalTrigConv;
	ADC_InitStruct.ADC_DataAlign          = ADC_DataAlign_Right;    //右对齐
	ADC_InitStruct.ADC_NbrOfConversion    = config->Channel_Count;
	ADC_Init((ADC_TypeDef*)WHT_ADC->Name, &ADC_InitStruct);

	/*配置ADCx通道的转换顺序和采样时间*/
	for (unsigned char i = 0; i < WHT_ADC->Channel_Count; i++)
	{
		ADC_RegularChannelConfig((ADC_TypeDef*)WHT_ADC->Name, WHT_ADC->Channel_Config[i].Channel, i + 1, WHT_ADC->Channel_Config[i].Sample_Time);
	}

	/*关闭ADC转速完成中断*/
	ADC_ITConfig((ADC_TypeDef*)WHT_ADC->Name, ADC_IT_EOC | ADC_IT_AWD | ADC_IT_JEOC, DISABLE);
}


/*ADC 手动触发获取ADC值*/
static void WHT_ADC_BSP_Receive(const WHT_ADC_Cache_t* cache)
{
	WHT_ADC->Info.Busy_Flag = SET;
	((ADC_TypeDef*)WHT_ADC->Name)->CR1 &=  ~(0x01 << 8);            //关闭扫描模式
	((ADC_TypeDef*)WHT_ADC->Name)->CR2 &=  ~(0x01 << 1);            //关闭连续转换模式
	ADC_ClearFlag((ADC_TypeDef*)WHT_ADC->Name, ADC_FLAG_AWD | ADC_FLAG_EOC | ADC_FLAG_JEOC | ADC_FLAG_JSTRT | ADC_FLAG_STRT);//清除所有中断标志位
	ADC_ITConfig((ADC_TypeDef*)WHT_ADC->Name, ADC_IT_EOC, DISABLE); //规则转换完成
	ADC_ITConfig((ADC_TypeDef*)WHT_ADC->Name, ADC_IT_AWD, DISABLE); //模拟看门狗
	ADC_ITConfig((ADC_TypeDef*)WHT_ADC->Name, ADC_IT_JEOC, DISABLE);//注入转换完成
	ADC_DMACmd((ADC_TypeDef*)WHT_ADC->Name, DISABLE);
	ADC_Cmd((ADC_TypeDef*)WHT_ADC->Name, ENABLE);

	/*配置ADCx通道的转换顺序和采样时间*/
	for (unsigned char channel = 0; channel < WHT_ADC->Channel_Count; channel++)
	{
		ADC_RegularChannelConfig((ADC_TypeDef*)WHT_ADC->Name, WHT_ADC->Channel_Config[channel].Channel, 1, WHT_ADC->Channel_Config[channel].Sample_Time);
	
		for (WHT_ADC->Temp_Count = 0; WHT_ADC->Temp_Count < WHT_ADC->Cache->Rx_Count; WHT_ADC->Temp_Count++)
		{
			/*强制软件触发转换*/
			ADC_Cmd((ADC_TypeDef*)WHT_ADC->Name, ENABLE);
			/*等待转换完成*/
			while (ADC_GetFlagStatus((ADC_TypeDef*)WHT_ADC->Name, ADC_FLAG_EOC) == RESET);
			/*保存转换结果*/
			((unsigned short(*)[WHT_ADC->Channel_Count])WHT_ADC->Cache->Rx_Buf)[WHT_ADC->Temp_Count][channel] = ADC_GetConversionValue((ADC_TypeDef*)WHT_ADC->Name);
			ADC_ClearFlag((ADC_TypeDef*)WHT_ADC->Name, ADC_FLAG_EOC);
		}
	}
	/*配置ADCx通道的转换顺序和采样时间*/
	for (unsigned char i = 0; i < WHT_ADC->Channel_Count; i++)
	{
		ADC_RegularChannelConfig((ADC_TypeDef*)WHT_ADC->Name, WHT_ADC->Channel_Config[i].Channel, i + 1, WHT_ADC->Channel_Config[i].Sample_Time);
	}

	ADC_Cmd((ADC_TypeDef*)WHT_ADC->Name, DISABLE);  //关闭ADC功能
	((ADC_TypeDef*)WHT_ADC->Name)->CR1 |= 0x01 << 8;//开启扫描模式
	((ADC_TypeDef*)WHT_ADC->Name)->CR2 |= 0x01 << 1;//开启连续转换模式

	WHT_ADC->Info.Busy_Flag = RESET;
}
/*ADC 中断获取ADC值*/
static void WHT_ADC_BSP_Interrupt_Receive(const WHT_ADC_Cache_t* cache)
{
	WHT_ADC->Info.Busy_Flag = SET;
	WHT_ADC->Temp_Count = 65535;//65535是自己定义的标识，用来同步
	((ADC_TypeDef*)WHT_ADC->Name)->CR1 &=  ~(0x01 << 8);            //关闭扫描模式
	((ADC_TypeDef*)WHT_ADC->Name)->CR2 &=  ~(0x01 << 1);            //关闭连续转换模式
	ADC_ClearFlag((ADC_TypeDef*)WHT_ADC->Name, ADC_FLAG_AWD | ADC_FLAG_EOC | ADC_FLAG_JEOC | ADC_FLAG_JSTRT | ADC_FLAG_STRT);//清除所有中断标志位
	ADC_ITConfig((ADC_TypeDef*)WHT_ADC->Name, ADC_IT_EOC, ENABLE);  //规则转换完成
	ADC_ITConfig((ADC_TypeDef*)WHT_ADC->Name, ADC_IT_AWD, DISABLE); //模拟看门狗
	ADC_ITConfig((ADC_TypeDef*)WHT_ADC->Name, ADC_IT_JEOC, DISABLE);//注入转换完成
	ADC_DMACmd((ADC_TypeDef*)WHT_ADC->Name, DISABLE);               //关闭ADC的DMA
	ADC_Cmd((ADC_TypeDef*)WHT_ADC->Name, ENABLE);                   //开启ADC功能

	ADC_RegularChannelConfig((ADC_TypeDef*)WHT_ADC->Name, WHT_ADC->Channel_Config[0].Channel, 1, WHT_ADC->Channel_Config[0].Sample_Time);

	/*开启事件触发转换*/
	ADC_SoftwareStartConv((ADC_TypeDef*)WHT_ADC->Name);
}
/*ADC DMA获取双ADC值*/
static void WHT_ADC_BSP_DMA_2Receive(const WHT_ADC_Cache_t* cache)
{
    DMA_InitTypeDef DMA_InitStruct =
    {
        .DMA_Channel            = 0,                          //DMA通道
        .DMA_PeripheralBaseAddr = (unsigned int)&ADC1->DR,    //外设基地址
        .DMA_Memory0BaseAddr    = (unsigned int)WHT_ADC->Cache->Rx_Buf,//存储器首地址
        .DMA_DIR                = DMA_DIR_PeripheralToMemory, //P->M
        .DMA_BufferSize         = WHT_ADC->Channel_Count * WHT_ADC->Cache->Rx_Count,//传输数据个数
        .DMA_PeripheralInc      = DMA_PeripheralInc_Disable,  //外设地址不递增
        .DMA_MemoryInc          = DMA_MemoryInc_Enable,       //存储器地址递增
        .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word,//外设数据大小为4字节
        .DMA_MemoryDataSize     = DMA_MemoryDataSize_Word,    //存储器数据大小为4字节
        .DMA_Mode               = DMA_Mode_Circular,          //循环传输模式
        .DMA_Priority           = DMA_Priority_Low,           //优先级最低
        .DMA_FIFOMode           = DMA_FIFOMode_Disable,       //FIFO模式
        .DMA_FIFOThreshold      = DMA_FIFOThreshold_Full,     //FIFO阈值
        .DMA_MemoryBurst        = DMA_MemoryBurst_Single,     //内存突发传输
        .DMA_PeripheralBurst    = DMA_PeripheralBurst_Single, //外设突发传输
    };

	WHT_ADC->Info.Busy_Flag = SET;
    WHT_ADC->DMA_Receive_Config.IT_Callback = NULL;//这个回调函数主要处理DMA传输错误的处理
	WHT_ADC->DMA_Receive_Config.DMA_InitStruct = &DMA_InitStruct;
    
	while (WHT_ADC->DMA_Receive_Config.Info->BSP_Mutex == SET);//等待没有外设使用此通道的DMA
    WHT_ADC->DMA_Receive_Config.Info->BSP_Mutex = SET;         //DMA通道上锁
    WHT_DMA_BSP.WHT_Config(&WHT_ADC->DMA_Receive_Config);      //配置当前DMA

	ADC_ClearFlag(ADC1, ADC_FLAG_AWD | ADC_FLAG_EOC | ADC_FLAG_JEOC | ADC_FLAG_JSTRT | ADC_FLAG_STRT);//清除所有中断标志位
	ADC_ClearFlag(ADC2, ADC_FLAG_AWD | ADC_FLAG_EOC | ADC_FLAG_JEOC | ADC_FLAG_JSTRT | ADC_FLAG_STRT);//清除所有中断标志位
	ADC_ITConfig(ADC1, ADC_IT_EOC, DISABLE); //规则转换完成
	ADC_ITConfig(ADC2, ADC_IT_EOC, DISABLE); //规则转换完成
	ADC_ITConfig(ADC1, ADC_IT_AWD, DISABLE); //模拟看门狗
	ADC_ITConfig(ADC2, ADC_IT_AWD, DISABLE); //模拟看门狗
	ADC_ITConfig(ADC1, ADC_IT_JEOC, DISABLE);//注入转换完成
	ADC_ITConfig(ADC2, ADC_IT_JEOC, DISABLE);//注入转换完成
	ADC_DMACmd(ADC1, ENABLE);                //开启DMA请求
	ADC_Cmd(ADC1, ENABLE);                   //开启ADC功能
	ADC_Cmd(ADC2, ENABLE);                   //开启ADC功能
	/*多ADC模式*/
  	ADC_MultiModeDMARequestAfterLastTransferCmd(ENABLE);
	/*开启事件触发转换*/
	ADC_SoftwareStartConv(ADC1);
}
/*ADC DMA获取ADC值*/
static void WHT_ADC_BSP_DMA_Receive(const WHT_ADC_Cache_t* cache)
{
    DMA_InitTypeDef DMA_InitStruct =
    {
        .DMA_Channel            = 0,                          //DMA通道
        .DMA_PeripheralBaseAddr = 0,                          //外设基地址
        .DMA_Memory0BaseAddr    = 0,                          //存储器地址
        .DMA_DIR                = DMA_DIR_PeripheralToMemory, //P->M
        .DMA_BufferSize         = 0,                          //传输个数
        .DMA_PeripheralInc      = DMA_PeripheralInc_Disable,  //外设地址不递增
        .DMA_MemoryInc          = DMA_MemoryInc_Enable,       //存储器地址递增
        .DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord,//外设数据大小为2字节
        .DMA_MemoryDataSize     = DMA_MemoryDataSize_HalfWord,    //存储器数据大小为2字节
        .DMA_Mode               = DMA_Mode_Circular,          //循环传输模式
        .DMA_Priority           = DMA_Priority_Low,           //优先级最低
        .DMA_FIFOMode           = DMA_FIFOMode_Disable,       //FIFO模式
        .DMA_FIFOThreshold      = DMA_FIFOThreshold_Full,     //FIFO阈值
        .DMA_MemoryBurst        = DMA_MemoryBurst_Single,     //内存突发传输
        .DMA_PeripheralBurst    = DMA_PeripheralBurst_Single, //外设突发传输
    };

	if (WHT_ADC->Name == ADC1_ADC2)
	{
		WHT_ADC_BSP_DMA_2Receive(cache);
		return;
	}
	else if (WHT_ADC->Name == ADC1_2_3)
	{
		return;
	}
	
	DMA_InitStruct.DMA_PeripheralBaseAddr = (unsigned int)&((ADC_TypeDef*)WHT_ADC->Name)->DR;//外设基地址
    DMA_InitStruct.DMA_Memory0BaseAddr = (unsigned int)WHT_ADC->Cache->Rx_Buf;               //数据源首地址
    DMA_InitStruct.DMA_BufferSize = WHT_ADC->Channel_Count * WHT_ADC->Cache->Rx_Count;       //传输数据个数

	WHT_ADC->Info.Busy_Flag = SET;
	WHT_ADC->DMA_Receive_Config.IT_Callback = NULL;//这个回调函数主要处理DMA传输错误的处理
    WHT_ADC->DMA_Receive_Config.DMA_InitStruct = &DMA_InitStruct;
	while (WHT_ADC->DMA_Receive_Config.Info->BSP_Mutex == SET);//等待没有外设使用此通道的DMA
    WHT_ADC->DMA_Receive_Config.Info->BSP_Mutex = SET;         //DMA通道上锁
    WHT_DMA_BSP.WHT_Config(&WHT_ADC->DMA_Receive_Config);      //配置当前DMA

	ADC_ClearFlag((ADC_TypeDef*)WHT_ADC->Name, ADC_FLAG_AWD | ADC_FLAG_EOC | ADC_FLAG_JEOC | ADC_FLAG_JSTRT | ADC_FLAG_STRT);//清除所有中断标志位
	ADC_ITConfig((ADC_TypeDef*)WHT_ADC->Name, ADC_IT_EOC, DISABLE); //规则转换完成
	ADC_ITConfig((ADC_TypeDef*)WHT_ADC->Name, ADC_IT_AWD, DISABLE); //模拟看门狗
	ADC_ITConfig((ADC_TypeDef*)WHT_ADC->Name, ADC_IT_JEOC, DISABLE);//注入转换完成
	ADC_DMACmd((ADC_TypeDef*)WHT_ADC->Name, ENABLE);                //开启DMA请求
	ADC_Cmd((ADC_TypeDef*)WHT_ADC->Name, ENABLE);                   //开启ADC功能
  	//单ADC模式
	ADC_DMARequestAfterLastTransferCmd((ADC_TypeDef*)WHT_ADC->Name, ENABLE);
	/*开启事件触发转换*/
	ADC_SoftwareStartConv((ADC_TypeDef*)WHT_ADC->Name);
}

/*ADC 结束中断获取ADC值*/
static void WHT_ADC_BSP_End_Interrupt_Receive(const WHT_ADC_Cache_t* cache)
{
	ADC_ITConfig((ADC_TypeDef*)WHT_ADC->Name, ADC_IT_EOC, DISABLE);//规则转换完成
	ADC_ClearFlag((ADC_TypeDef*)WHT_ADC->Name, ADC_FLAG_AWD | ADC_FLAG_EOC | ADC_FLAG_JEOC | ADC_FLAG_JSTRT | ADC_FLAG_STRT);//清除所有中断标志位
	ADC_Cmd((ADC_TypeDef*)WHT_ADC->Name, DISABLE);                 //关闭ADC功能
	((ADC_TypeDef*)WHT_ADC->Name)->CR1 |= 0x01 << 8;               //开启扫描模式
	((ADC_TypeDef*)WHT_ADC->Name)->CR2 |= 0x01 << 1;               //开启连续转换模式

	/*配置ADCx通道的转换顺序和采样时间*/
	for (unsigned char i = 0; i < WHT_ADC->Channel_Count; i++)
	{
		ADC_RegularChannelConfig((ADC_TypeDef*)WHT_ADC->Name, WHT_ADC->Channel_Config[i].Channel, i + 1, WHT_ADC->Channel_Config[i].Sample_Time);
	}

	WHT_ADC->Info.Busy_Flag = RESET;
}

/*ADC 结束DMA获取ADC值*/
static void WHT_ADC_BSP_End_DMA_Receive(const WHT_ADC_Cache_t* cache)
{
	WHT_DMA_BSP.WHT_Close(&WHT_ADC->DMA_Receive_Config);//关闭DMA
	if (WHT_ADC->Name == ADC1_ADC2)
	{
		ADC_DMACmd(ADC1, DISABLE);              //关闭DMA请求
		ADC_Cmd(ADC1, DISABLE);                 //关闭ADC功能
		ADC_Cmd(ADC2, DISABLE);                 //关闭ADC功能
	}
	else
	{
		ADC_DMACmd((ADC_TypeDef*)WHT_ADC->Name, DISABLE);//关闭DMA请求
		ADC_Cmd((ADC_TypeDef*)WHT_ADC->Name, DISABLE);   //关闭ADC功能
	}
	WHT_ADC->Info.Busy_Flag = RESET;
}


/*全局常量*/
const WHT_ADC_BSP_t WHT_ADC_BSP =
{
	.WHT_Register              = WHT_ADC_BSP_Register,
	.WHT_Config                = WHT_ADC_BSP_Config,
	.WHT_Receive               = WHT_ADC_BSP_Receive,
	.WHT_Interrupt_Receive     = WHT_ADC_BSP_Interrupt_Receive,
	.WHT_DMA_Receive           = WHT_ADC_BSP_DMA_Receive,
	.WHT_End_Interrupt_Receive = WHT_ADC_BSP_End_Interrupt_Receive,
	.WHT_End_DMA_Receive       = WHT_ADC_BSP_End_DMA_Receive,
};




/*ADC中断回调函数*/
static void ADC_IRQHandler_Callback(const WHT_ADC_Cache_t* cache, unsigned char* channel)
{
	if (ADC_GetITStatus((ADC_TypeDef*)WHT_ADC->Name, ADC_IT_EOC) == RESET)
		return;
	
	if (WHT_ADC->Temp_Count == 65535)//65535是自己定义的标识，用来同步
	{
		WHT_ADC->Temp_Count = 0;
		*channel = 0;
	}

	if (ADC_GetITStatus((ADC_TypeDef*)WHT_ADC->Name, ADC_IT_EOC) == SET)
	{
		((unsigned short(*)[WHT_ADC->Channel_Count])WHT_ADC->Cache->Rx_Buf)[WHT_ADC->Temp_Count][*channel] = ADC_GetConversionValue((ADC_TypeDef*)WHT_ADC->Name);
		ADC_ClearITPendingBit((ADC_TypeDef*)WHT_ADC->Name, ADC_IT_EOC);
	}
	else if (ADC_GetITStatus((ADC_TypeDef*)WHT_ADC->Name, ADC_IT_JEOC) == SET)
	{
		ADC_GetInjectedConversionValue((ADC_TypeDef*)WHT_ADC->Name, ADC_InjectedChannel_1);
		ADC_GetInjectedConversionValue((ADC_TypeDef*)WHT_ADC->Name, ADC_InjectedChannel_2);
		ADC_GetInjectedConversionValue((ADC_TypeDef*)WHT_ADC->Name, ADC_InjectedChannel_3);
		ADC_GetInjectedConversionValue((ADC_TypeDef*)WHT_ADC->Name, ADC_InjectedChannel_4);
		ADC_ClearITPendingBit((ADC_TypeDef*)WHT_ADC->Name, ADC_IT_JEOC);
	}
	else if (ADC_GetITStatus((ADC_TypeDef*)WHT_ADC->Name, ADC_IT_AWD) == SET)
	{
		ADC_ClearITPendingBit((ADC_TypeDef*)WHT_ADC->Name, ADC_IT_AWD);
	}

	if (++WHT_ADC->Temp_Count >= WHT_ADC->Cache->Rx_Count)
	{
		WHT_ADC->Temp_Count = 0;
		if (++*channel >= WHT_ADC->Channel_Count)
			*channel = 0;
		ADC_RegularChannelConfig((ADC_TypeDef*)WHT_ADC->Name, WHT_ADC->Channel_Config[*channel].Channel, 1, WHT_ADC->Channel_Config[*channel].Sample_Time);
	}
	/*开启事件触发转换*/
	ADC_SoftwareStartConv((ADC_TypeDef*)WHT_ADC->Name);
}

void ADC_IRQHandler(void)
{
	static unsigned char ADC1_Channel = 0;
	static unsigned char ADC2_Channel = 0;
	static unsigned char ADC3_Channel = 0;

	if (ADC_GetFlagStatus(ADC1, ADC_FLAG_AWD | ADC_FLAG_EOC | ADC_FLAG_JEOC | ADC_FLAG_OVR) == SET)
	{
		ADC_IRQHandler_Callback(WHT_ADC1.Cache, &ADC1_Channel);
	}
	else if (ADC_GetFlagStatus(ADC2, ADC_FLAG_AWD | ADC_FLAG_EOC | ADC_FLAG_JEOC | ADC_FLAG_OVR) == SET)
	{
		ADC_IRQHandler_Callback(WHT_ADC2.Cache, &ADC2_Channel);
	}
	else if (ADC_GetFlagStatus(ADC3, ADC_FLAG_AWD | ADC_FLAG_EOC | ADC_FLAG_JEOC | ADC_FLAG_OVR) == SET)
	{
		ADC_IRQHandler_Callback(WHT_ADC3.Cache, &ADC3_Channel);
	}
}
