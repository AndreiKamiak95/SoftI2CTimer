
#include "soft_i2c_timer.h"

extern TIM_HandleTypeDef htim7;

uint8_t curPosition = 0;
uint8_t sendByte = 0x00;

uint8_t sendBuffer[8];
uint8_t sizeSendByte;
uint8_t curNumSendByte = 0x00;

uint8_t sizeRecvByte = 0;
uint8_t curNumRecvByte = 0x00;
uint8_t *recvBytes;

static uint8_t isWriteFlag;

static Status statusInterface = SI2C_FREE;

static uint8_t flag = 0;
static uint8_t ack;

#define START 0
#define RESTART 1
#define SEND_BYTE 2
#define GET_BYTE 3
#define STOP 4

static inline void SDA_LOW()
{
	//выставить логический 0 на SDA
	
  PORT_SDA->BSRR |= (1 << (PIN_SDA + 0x10));
}
static inline void SDA_HIGH()
{
	//выставить логическую 1 на SDA
	
	PORT_SDA->BSRR |= (1 << PIN_SDA);
}
static inline void SCL_LOW()
{
	//выставить логический 0 на SCL
	
	PORT_SCL->BSRR |= (1 << (PIN_SCL + 0x10));
}

static inline void SCL_HIGH()
{
	//выставить логическую 1 на SCL
	
	PORT_SCL->BSRR |= (1 << PIN_SCL);
}

static inline void SDA_OUTPUT()
{
	//сконфигурировать ножку SDA на выход
	
	uint16_t buf = PORT_SDA->MODER;
	buf |= 1 << (PIN_SDA * 2);
	buf &= ~(1 << (PIN_SDA * 2 + 1));
	PORT_SDA->MODER = buf;
}

static inline void SCL_OUTPUT()
{
	//сконфигурировать ножку SCL на выход
	
  uint16_t buf = PORT_SCL->MODER;
	buf |= 1 << (PIN_SCL * 2);
	buf &= ~(1 << (PIN_SCL * 2 + 1));
	PORT_SCL->MODER = buf;
}

static inline void SDA_INPUT()
{
	//сконфигурировать ножку SDA на вход
	
  uint16_t buf = PORT_SDA->MODER;
	buf &= ~(1 << (PIN_SDA * 2));
	buf &= ~(1 << (PIN_SDA * 2 + 1));
	PORT_SDA->MODER = buf;
}

static inline uint8_t SDA_READV()
{
	// чтение состояни пина SDA
	
	uint8_t reg = PORT_SDA->IDR;
	reg &= (1 << PIN_SDA);
	reg = reg >> PIN_SDA;
	return reg;
}

void Soft_I2C_Init()
{
	SDA_HIGH();
	SCL_HIGH();
	SDA_OUTPUT();
	SCL_OUTPUT();
}

void Soft_I2C_Start()
{
	static uint8_t curPos = 0;
	
	
	switch(curPos)
	{
		case 0:
			SDA_LOW();
			curPos++;
			break;
		case 1:
			SCL_LOW();
			curPos = 0;
			curPosition = SEND_BYTE;
			break;
	}
}

void Soft_I2C_Stop()
{
	static uint8_t curPos = 0;
	switch(curPos)
	{
		case 0:
			SCL_LOW();
			curPos++;
			break;
		case 1:
			SDA_LOW();
			curPos++;
			break;
		case 2:
			SCL_HIGH();
			curPos++;
			break;
		case 3:
			SDA_HIGH();
			curPos=0;
			curPosition++;
			break;
	}
}

void Soft_I2C_Restart()
{
	static uint8_t curPos = 0;
	switch(curPos)
	{
		case 0:
			SDA_HIGH();
			curPos++;
			break;
		case 1:
			SCL_HIGH();
			curPos++;
			break;
		case 2:
			SDA_LOW();
			curPos++;
			break;
		case 3:
			SCL_LOW();
			curPos = 0;
			curPosition = SEND_BYTE;
			break;
	}
}



void Soft_I2C_SendByte()
{
	static uint8_t curPos = 0;
	static uint8_t curNumBit = 0;
	
	switch(curPos)
	{
		case 0:
			label:
			SCL_LOW();
			
			if(sendByte & 0x80)
			{
				SDA_HIGH();
			}
			else
			{
				SDA_LOW();
			}
			curPos++;
			sendByte <<= 1;
			curNumBit++;
			break;
			
		case 1:
			SCL_HIGH();
			curPos++;
			break;
		
		case 2:
			
			if(curNumBit == 8)
			{
				curPos++;
				curNumBit = 0;
				SCL_LOW();
				SDA_INPUT();
			}
			else
			{
				curPos = 0;
				goto label;
			}
			break;
			
		case 3:
			SCL_HIGH();
			curPos++;
			break;

		case 4:
			ack = SDA_READV();
			SCL_LOW();
			curPos++;
		
			if(ack == 1)
			{
				statusInterface = SI2C_ERROR;
				curPosition = STOP;
				sendByte = 0;
				curNumSendByte = 0;
			}
			break;
		
		case 5:
			SDA_OUTPUT();
			SDA_LOW();
			curPos = 0;
			sendByte++;
		
			if(isWriteFlag == 0)
			{
				if(curNumSendByte == 1)
				{
					curPosition = RESTART;
					curNumSendByte++;
					sendByte = sendBuffer[curNumSendByte];
					return;
				}
				if(curNumSendByte == 2)
				{
					curPosition = GET_BYTE;
					curNumSendByte = 0;
					return;
				}
			}
		
			if(curNumSendByte == sizeSendByte - 1)
			{
				curPosition = STOP;
				sendByte = 0;
			}
			else
			{
				curNumSendByte++;
				sendByte = sendBuffer[curNumSendByte];
				curPosition = SEND_BYTE;
			}
			break;
	}
}

void Soft_I2C_GetByte()
{
	static uint8_t res=0;
	static uint8_t sda;
	static uint8_t curPos = 0;
	static uint8_t countRecvBit = 0;
	
	switch(curPos)
	{
		case 0:
			res<<=1;
			SCL_HIGH();
			curPos++;
			break;
		
		case 1:
			SDA_INPUT();
			SDA_HIGH();
			sda = SDA_READV();
			if(sda == 1) res = res | 0x01;
			SCL_LOW();
			countRecvBit++;
			if(countRecvBit == 8)
			{
				countRecvBit = 0;
				recvBytes[curNumRecvByte] = res;
				curNumRecvByte++;
				if(curNumRecvByte == sizeRecvByte)
				{
					SDA_HIGH();
					curPos++;
				}
				else
				{
					SDA_LOW();
					curPos = 0;
				}
				SDA_OUTPUT();
			}
			else
			{
				curPos = 0;
			}
			break;
			
		case 2:
			SDA_OUTPUT();
			curPos++;
			break;
		
		case 3:
			SCL_HIGH();
			curPos++;
			break;
		
		case 4:
			SCL_LOW();
			curPos++;
			break;
		
		case 5:
			SDA_HIGH();
			curPos = 0;
			curPosition = STOP;
			break;
	}
	
}

void TIM7_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim7);

	switch(curPosition)
	{
		case START: 
			Soft_I2C_Start();
			break;
		
		case RESTART:
			Soft_I2C_Restart();
			break;
		
		case SEND_BYTE:
			Soft_I2C_SendByte();
			break;
		
		case GET_BYTE:
			Soft_I2C_GetByte();
			break;
		
		case STOP:
			Soft_I2C_Stop();
			break;
		
		case 5:
			HAL_TIM_Base_Stop(&htim7);
			curPosition = 0;
			if(statusInterface != SI2C_ERROR)
				statusInterface = SI2C_READY;
			break;
	}
		
}

void Soft_I2C_MemWrite(uint8_t devAddress, uint8_t memAddress, uint8_t *pData, uint16_t size)
{
	sendBuffer[0] = devAddress << 1;
	sendBuffer[1] = memAddress;
  for(uint8_t i = 0; i < size; i++)
	{
		sendBuffer[i + 2] = pData[i]; 
	}
	sizeSendByte = size + 2;
	sendByte = sendBuffer[0];
	isWriteFlag = 1;
	statusInterface = SI2C_BUSY;
	HAL_TIM_Base_Start_IT(&htim7);
}

void Soft_I2C_MemRead(uint8_t devAddress, uint8_t memAddress, uint8_t *pData, uint16_t size)
{
	sendBuffer[0] = devAddress << 1;
	sendBuffer[1] = memAddress;
	sendBuffer[2] = (devAddress << 1) + 1;
	
	sendByte = sendBuffer[0];
	sizeSendByte = 3;
	
	recvBytes = pData;
	sizeRecvByte = size;
	
	isWriteFlag = 0;
	statusInterface = SI2C_BUSY;
	HAL_TIM_Base_Start_IT(&htim7);
}

Status Soft_I2C_GetStatus()
{
	if(statusInterface == SI2C_READY)
	{
		statusInterface = SI2C_FREE;
		return SI2C_READY;
	}
	if(statusInterface == SI2C_ERROR)
	{
		statusInterface = SI2C_FREE;
		return SI2C_FREE;
	}
	
	return statusInterface;
}