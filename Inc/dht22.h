#ifndef DHT22_H
#define DHT22_H

#include "stm32f4xx_hal.h"

void              DHT22_Init(TIM_HandleTypeDef *timer, GPIO_TypeDef *gpio, uint16_t gpio_pin);
HAL_StatusTypeDef DHT22_Read(float *temperature, float *humidity);

#endif
