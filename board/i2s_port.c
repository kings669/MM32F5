#include "i2s_port.h"
#include "wav.h"

uint8_t I2S_DMA_Finish  = 1;

extern uint8_t AUDIO_PlayState;
extern uint8_t AUDIO_Extension;


void I2S_Configure(I2S_Protocol_Type   Standard,
                         I2S_DataWidth_Type DataFormat,
                         uint32_t AudioFreq,
                         I2S_XferMode_Type Mode)
{
    /* Setup the I2S. */
    I2S_Master_Init_Type i2s_master_init;
		
    i2s_master_init.ClockFreqHz  = CLOCK_APB1_FREQ;
    i2s_master_init.SampleRate   = AudioFreq;
    i2s_master_init.DataWidth    = DataFormat;
    i2s_master_init.Protocol     = Standard;
    i2s_master_init.EnableMCLK   = true;
    i2s_master_init.Polarity     = I2S_Polarity_1;
    i2s_master_init.XferMode     = Mode;
	
		I2S_InitMaster(SPI2, &i2s_master_init);
		I2S_EnableDMA(SPI2, true);
		I2S_Enable(SPI2, true);
}

void I2S_DMA_Transfer(uint16_t *Buffer, uint32_t BufferSize)
{
	  /* Setup the DMA for I2S RX. */
    DMA_Channel_Init_Type dma_channel_init;

    dma_channel_init.MemAddr           = (uint32_t)(Buffer);
    dma_channel_init.MemAddrIncMode    = DMA_AddrIncMode_IncAfterXfer;
    dma_channel_init.PeriphAddr        = I2S_GetTxDataRegAddr(SPI2);  /* use tx data register here. */
    dma_channel_init.PeriphAddrIncMode = DMA_AddrIncMode_StayAfterXfer;
    dma_channel_init.Priority          = DMA_Priority_Highest;
    dma_channel_init.XferCount         = BufferSize;
    dma_channel_init.XferMode          = DMA_XferMode_MemoryToPeriph;
    dma_channel_init.ReloadMode        = DMA_ReloadMode_AutoReload;             /* DMA_AutoReloadMode_Circular */
    dma_channel_init.XferWidth         = DMA_XferWidth_16b;
    DMA_InitChannel(DMA1, DMA_REQ_DMA1_SPI2_TX, &dma_channel_init);
	
    /* Enable DMA transfer done interrupt. */
    DMA_EnableChannelInterrupts(DMA1, DMA_REQ_DMA1_SPI2_TX, DMA_CHN_INT_XFER_DONE, true);
		DMA_EnableChannelInterrupts(DMA1, DMA_REQ_DMA1_SPI2_TX, DMA_CHN_INT_XFER_HALF_DONE, true);
    NVIC_EnableIRQ(DMA1_CH5_IRQn);
	
		DMA_EnableChannel(DMA1, DMA_REQ_DMA1_SPI2_TX, true);
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
					DMA_EnableChannel(DMA1, DMA_REQ_DMA1_SPI2_TX, true);
					I2S_DMA_Finish = 1;
					DMA_ClearChannelInterruptStatus(DMA1, DMA_REQ_DMA1_SPI2_TX, DMA_CHN_INT_XFER_DONE);	
				}
				
				if(0u != (DMA_CHN_INT_XFER_HALF_DONE & DMA_GetChannelInterruptStatus(DMA1, DMA_REQ_DMA1_SPI2_TX)) )
				{
					DMA_ClearChannelInterruptStatus(DMA1, DMA_REQ_DMA1_SPI2_TX, DMA_CHN_INT_XFER_HALF_DONE);
				}
    }
}


