/*
 * gpio.c
 *
 *  Created on: Apr 3, 2025
 *      Author: ASUS_USER
 */
#include "gpio.h"

void MX_GPIO_Init(void)
{

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

}
