#include "I2S.h"

uint8_t I2S_DMA_Finish  = 1;


extern uint8_t AUDIO_PlayState;
extern uint8_t AUDIO_Extension;


void i2s_init(void)
{
	//模块使能及模式为主模式
	SPI2->GCTL|=(SPI_I2S_GCTL_SPIEN_MASK|SPI_I2S_GCTL_MODE_MASK);
	SPI2->GCTL|=SPI_I2S_GCTL_INTEN_MASK;
	
	//时钟空闲输出电平配置
	SPI2->CCTL &= ~SPI_I2S_CCTL_CPOL_MASK;
	
	//工作在全双工
	SPI2->I2SCFGR&=~SPI_I2S_I2SCFGR_HDSEL_MASK;
	
	//向外部提供MCK驱动时钟
	SPI2->I2SCFGR|=SPI_I2S_I2SCFGR_MCKOE_MASK;
	
	//半双工时需要配置输出引脚
	//SPI2->I2SCFGR|=SPI_I2S_I2SCFGR_MCKSEL_MASK;
	
	//配置时钟分频
	SPI2->I2SCFGR |= SPI_I2S_I2SCFGR_I2SDIV(5);
	
	//使能I2S传输功能
	SPI2->I2SCFGR |=SPI_I2S_I2SCFGR_SPII2S_MASK;
	
	//配置通信标准（飞利浦）
	//SPI2->I2SCFGR |=I2S_Standard_Phillips;
	
	//配置数据长度
	//SPI2->I2SCFGR |=I2S_DataFormat_16b;
	
	//配置DMA传输
	SPI2->GCTL |= SPI_I2S_GCTL_DMAMODE_MASK;
	
	//配置使能输出输入
	SPI2->GCTL |= (SPI_I2S_GCTL_TXEN_MASK|SPI_I2S_GCTL_RXEN_MASK);
	//SPI2->GCTL |= (SPI_I2S_GCTL_TXEN_MASK);
}

void I2S_Configure(SPI_I2S_STANDARD_TypeDef   Standard,
                         SPI_I2S_DATAFORMAT_TypeDef DataFormat,
                         SPI_I2S_AUDIO_FREQ_TypeDef AudioFreq,
                         SPI_I2S_TRANS_MODE_TypeDef Mode)
{
		I2S_InitTypeDef I2S_InitStructure;
		if((Mode == I2S_Mode_SlaveTx) || (Mode == I2S_Mode_SlaveRx))
    {
        return;
    }
		
		SPI2->GCTL|=(SPI_I2S_GCTL_TXEN_MASK|SPI_I2S_GCTL_MODE_MASK|SPI_I2S_CCTL_SPILEN_MASK);
		
		I2S_InitStructure.I2S_Mode = Mode;
    I2S_InitStructure.I2S_Standard   = Standard;
    I2S_InitStructure.I2S_DataFormat = DataFormat;
    I2S_InitStructure.I2S_MCLKOutput = I2S_MCLKOutput_Enable;
    I2S_InitStructure.I2S_AudioFreq  = AudioFreq;
    I2S_InitStructure.I2S_CPOL       = I2S_CPOL_Low;
		
		I2S_Init(SPI2, &I2S_InitStructure);
	
		SPI_EnableDMA(SPI2,true);/* Events would trigger the DMA instead of interrupt. */
		I2S_Cmd(SPI2,true);
}

void I2S_DMA_Transfer(uint16_t *Buffer, uint32_t BufferSize)
{
	  DMA_Channel_Init_Type dma_channel_init;

    dma_channel_init.XferMode = DMA_XferMode_MemoryToPeriph;
		dma_channel_init.ReloadMode = DMA_ReloadMode_AutoReload; /* DMA_AutoReloadMode_OneTime */
		dma_channel_init.PeriphAddrIncMode = DMA_AddrIncMode_StayAfterXfer;
		dma_channel_init.MemAddrIncMode = DMA_AddrIncMode_IncAfterXfer;
		dma_channel_init.XferWidth = DMA_XferWidth_32b;
    dma_channel_init.Priority = DMA_Priority_High;
		dma_channel_init.XferCount = BufferSize;
	
		dma_channel_init.MemAddr = (uint32_t)Buffer;
		dma_channel_init.PeriphAddr = (uint32_t)SPI_GetTxDataRegAddr(SPI2);
	
    DMA_InitChannel(DMA1, DMA_REQ_DMA1_SPI2_TX, &dma_channel_init);

    /* Enable interrupts. */
    NVIC_EnableIRQ(DMA1_CH5_IRQn); 
    DMA_EnableChannelInterrupts(DMA1, DMA_REQ_DMA1_SPI2_TX, DMA_CHN_INT_XFER_HALF_DONE | DMA_CHN_INT_XFER_DONE, true);
		I2S_DMA_Finish=0;
		DMA_EnableChannel(DMA1,DMA_REQ_DMA1_SPI2_TX,true);
}

void I2S_PowerON(uint8_t Enable)
{
    if(Enable == 1) /* Power On */
    {
        AUDIO_PlayState = 1;
    }
    else            /* Entry Standby */
    {
        AUDIO_PlayState = 0;
    }
}


void DMA1_CH5_IRQHandler(void)
{
    if(AUDIO_Extension == 0)    /* wav */
    {
			  if (0u != (DMA_CHN_INT_XFER_DONE & DMA_GetChannelInterruptStatus(DMA1, DMA_REQ_DMA1_SPI2_TX)) )
				{
					DMA_ClearChannelInterruptStatus(DMA1, DMA_REQ_DMA1_SPI2_TX, DMA_CHN_INT_XFER_DONE);
					WAV_PlayHandler();
				}
				
				if(0u != (DMA_CHN_INT_XFER_HALF_DONE & DMA_GetChannelInterruptStatus(DMA1, DMA_REQ_DMA1_SPI2_TX)) )
				{
					DMA_ClearChannelInterruptStatus(DMA1, DMA_REQ_DMA1_SPI2_TX, DMA_CHN_INT_XFER_HALF_DONE);
					WAV_PrepareData();
				}
    }
    else                        /* mp3 */
    {
			
			 if (0u != (DMA_CHN_INT_XFER_DONE & DMA_GetChannelInterruptStatus(DMA1, DMA_REQ_DMA1_SPI2_TX)) )
				{
					I2S_DMA_Finish = 1;
					DMA_ClearChannelInterruptStatus(DMA1, DMA_REQ_DMA1_SPI2_TX, DMA_CHN_INT_XFER_DONE);	
				}
				
				if(0u != (DMA_CHN_INT_XFER_HALF_DONE & DMA_GetChannelInterruptStatus(DMA1, DMA_REQ_DMA1_SPI2_TX)) )
				{
					DMA_ClearChannelInterruptStatus(DMA1, DMA_REQ_DMA1_SPI2_TX, DMA_CHN_INT_XFER_HALF_DONE);
				}
    }
}
