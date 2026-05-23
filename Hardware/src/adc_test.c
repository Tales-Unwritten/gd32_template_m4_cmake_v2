#include "adc_bsp.h"



#if 0

/*非双ADC最后那个需要等于1*/
static uint16_t ADC_Buf[4][4][1];//数组的维表示通道对应的值，行表示通道，列表示ADC1、ADC2


static const WHT_ADC_Channel_Config ADC_CH_Config[2*2] =
{
    {
        .Channel          = ADC_CH11,
        .Sample_Time      = ADC_Clk_Time_480,
    },
    {
        .Channel          = ADC_CH10,
        .Sample_Time      = ADC_Clk_Time_480,
    },

    {
        .Channel          = ADC_CH0,
        .Sample_Time      = ADC_Clk_Time_480,
    },
    {
        .Channel          = ADC_CH8,
        .Sample_Time      = ADC_Clk_Time_480,
    },
};


static WHT_ADC_Cache_t cache =
{
    .Name = ADC_1,
    .Channel_Count = 4,
    .Rx_Count = 4,
    .Rx_Buf = &ADC_Buf[0][0][0],
};

static const WHT_ADC_Config_t ADC_Config = 
{
    .Name                = ADC_1,
    .Clk_Div             = APB2_Div8,
    .Mode                = Independent,
    .ExternalTrigConv    = Extern_Event_None,
    .PreemptionPriority  = 15,
    .SubPriority         = 0,
    .DMA_Func            = ENABLE,
    .Channel_Count       = 4,
    .Channel_Config      = (WHT_ADC_Channel_Config*)ADC_CH_Config,
};

static WHT_ADC_BSP_t ADC_BSP;

void ADC_TEST_FUNC_Init(void)
{
    WHT_ADC_BSP_Register(ADC_Config.Name, &ADC_BSP);

    ADC_BSP.WHT_Cache(&cache);
    ADC_BSP.WHT_Config(&ADC_Config);
}




void ADC_TEST_START(void)
{
    static uint8_t count = 0;

    count++;
    if (count == 4)
    {
        /////ADC_BSP.WHT_End_Interrupt_Receive(&ADC_Config);
        ADC_BSP.WHT_End_DMA_Receive(&ADC_Config); 
       //// ADC_BSP.WHT_Receive(&ADC_Config);
    }
    else if (count == 2)
    {
        ////ADC_BSP.WHT_Receive(&ADC_Config);
        //////ADC_BSP.WHT_Interrupt_Receive(&ADC_Config);     
        ADC_BSP.WHT_DMA_Receive(&ADC_Config);
    }
    for (int i = 0; i < 1000000; i++)
    {
    }
    

    printf("CH11 = %fmV, %fmV\r\n", ((float)ADC_Buf[0][0][0]/4.096f*3.3f), ((float)ADC_Buf[0][1][0]/4.096f*3.3f));
    printf("CH11 = %fmV, %fmV\r\n", ((float)ADC_Buf[1][0][0]/4.096f*3.3f), ((float)ADC_Buf[1][1][0]/4.096f*3.3f));
    printf("CH11 = %fmV, %fmV\r\n", ((float)ADC_Buf[2][0][0]/4.096f*3.3f), ((float)ADC_Buf[2][1][0]/4.096f*3.3f));
    printf("CH11 = %fmV, %fmV\r\n", ((float)ADC_Buf[3][0][0]/4.096f*3.3f), ((float)ADC_Buf[3][1][0]/4.096f*3.3f));

    printf("CH10 = %fmV, %fmV\r\n", ((float)ADC_Buf[0][2][0]/4.096f*3.3f), ((float)ADC_Buf[0][3][0]/4.096f*3.3f));
    printf("CH10 = %fmV, %fmV\r\n", ((float)ADC_Buf[1][2][0]/4.096f*3.3f), ((float)ADC_Buf[1][3][0]/4.096f*3.3f));
    printf("CH10 = %fmV, %fmV\r\n", ((float)ADC_Buf[2][2][0]/4.096f*3.3f), ((float)ADC_Buf[2][3][0]/4.096f*3.3f));
    printf("CH10 = %fmV, %fmV\r\n", ((float)ADC_Buf[3][2][0]/4.096f*3.3f), ((float)ADC_Buf[3][3][0]/4.096f*3.3f));
    memset(ADC_Buf, 0 ,sizeof(ADC_Buf));
    printf("\r\n\r\n");
}
#else

/*非双ADC最后那个需要等于1*/
static uint16_t ADC_Buf[4][2][2];//数组的维表示通道对应的值，行表示通道，列表示ADC1、ADC2


static const WHT_ADC_Channel_Config ADC_CH_Config[2*2] =
{
    {
        .Channel          = ADC_CH11,
        .Sample_Time      = ADC_Clk_Time_480,
    },
    {
        .Channel          = ADC_CH10,
        .Sample_Time      = ADC_Clk_Time_480,
    },

    {
        .Channel          = ADC_CH0,
        .Sample_Time      = ADC_Clk_Time_480,
    },
    {
        .Channel          = ADC_CH8,
        .Sample_Time      = ADC_Clk_Time_480,
    },
};


static WHT_ADC_Cache_t cache =
{
    .Rx_Count = 4,
    .Rx_Buf = &ADC_Buf[0][0][0],
};

static const WHT_ADC_Config_t ADC_Config = 
{
    .Name                = ADC1_ADC2,
    .Clk_Div             = APB2_Div8,
    .Mode                = DualMode_RegSimult,
    .ExternalTrigConv    = Extern_Event_None,
    .PreemptionPriority  = 15,
    .SubPriority         = 0,
    .Channel_Count       = 2,
    .Channel_Config      = (WHT_ADC_Channel_Config*)ADC_CH_Config,
};


void ADC_TEST_FUNC_Init(void)
{
	WHT_ADC_BSP.WHT_Register(&ADC_Config, &cache);
    WHT_ADC_BSP.WHT_Config(&ADC_Config, &cache);
}




void ADC_TEST_START(void)
{
    static uint8_t count = 0;

    count++;
    if (count == 4)
    {
        ///WHT_ADC_BSP.WHT_End_Interrupt_Receive(&cache);
        WHT_ADC_BSP.WHT_End_DMA_Receive(&cache); 
        ///WHT_ADC_BSP.WHT_Receive(&cache);
    }
    else if (count == 2)
    {
        ///WHT_ADC_BSP.WHT_Receive(&cache);
        ///WHT_ADC_BSP.WHT_Interrupt_Receive(&cache);     
        WHT_ADC_BSP.WHT_DMA_Receive(&cache);
    }
    for (int i = 0; i < 1000000; i++)
    {
    }
    

    printf("CH11 = %fmV, %fmV\r\n", ((float)ADC_Buf[0][0][0]/4.096f*3.3f), ((float)ADC_Buf[0][0][1]/4.096f*3.3f));
    printf("CH11 = %fmV, %fmV\r\n", ((float)ADC_Buf[1][0][0]/4.096f*3.3f), ((float)ADC_Buf[1][0][1]/4.096f*3.3f));
    printf("CH11 = %fmV, %fmV\r\n", ((float)ADC_Buf[2][0][0]/4.096f*3.3f), ((float)ADC_Buf[2][0][1]/4.096f*3.3f));
    printf("CH11 = %fmV, %fmV\r\n", ((float)ADC_Buf[3][0][0]/4.096f*3.3f), ((float)ADC_Buf[3][0][1]/4.096f*3.3f));

    printf("CH10 = %fmV, %fmV\r\n", ((float)ADC_Buf[0][1][0]/4.096f*3.3f), ((float)ADC_Buf[0][1][1]/4.096f*3.3f));
    printf("CH10 = %fmV, %fmV\r\n", ((float)ADC_Buf[1][1][0]/4.096f*3.3f), ((float)ADC_Buf[1][1][1]/4.096f*3.3f));
    printf("CH10 = %fmV, %fmV\r\n", ((float)ADC_Buf[2][1][0]/4.096f*3.3f), ((float)ADC_Buf[2][1][1]/4.096f*3.3f));
    printf("CH10 = %fmV, %fmV\r\n", ((float)ADC_Buf[3][1][0]/4.096f*3.3f), ((float)ADC_Buf[3][1][1]/4.096f*3.3f));
    memset(ADC_Buf, 0 ,sizeof(ADC_Buf));
    printf("\r\n\r\n");
}

#endif
