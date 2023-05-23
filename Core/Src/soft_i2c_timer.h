#ifndef SOFT_I2C_TIMER_H
#define SOFT_I2C_TIMER_H

//программный i2c версия 2

#include <stdint.h>

//для доступа к регистрам
#include "main.h"
#include "stm32f407xx.h"

#define PIN_SDA 7
#define PIN_SCL 6
#define PORT_SDA GPIOB
#define PORT_SCL GPIOB

#define DELAY_US 5

typedef enum
{
  SI2C_FREE     = 0x00U,
  SI2C_ERROR    = 0x01U,
  SI2C_BUSY     = 0x02U,
	SI2C_READY    = 0x03U
} Status;

//функции для определения платофрмы
static inline void SDA_LOW();
static inline void SDA_HIGH();
static inline void SDA_INPUT();
static inline void SDA_OUTPUT();
static inline uint8_t SDA_READV();
static inline void SCL_OUTPUT();
static inline void SCL_LOW();
static inline void SCL_HIGH();

void Soft_I2C_Init();
void Soft_I2C_MemWrite(uint8_t devAddress, uint8_t memAddress, uint8_t *pData, uint16_t size);
void Soft_I2C_MemRead(uint8_t devAddress, uint8_t memAddress, uint8_t *pData, uint16_t size);
Status Soft_I2C_GetStatus();

#endif //SOFT_I2C_TIMER_H