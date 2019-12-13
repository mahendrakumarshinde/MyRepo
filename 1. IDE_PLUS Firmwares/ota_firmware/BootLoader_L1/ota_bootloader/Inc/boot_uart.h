
#ifndef UART_H_
#define UART_H_

#include "stm32l4xx_hal.h"

extern UART_HandleTypeDef huart5;
extern UART_HandleTypeDef huart3;

/* Timeout for HAL. */
#define UART_TIMEOUT ((uint16_t)1000u)
#define DEBUG_UART huart5

/* Status report for the functions. */
typedef enum {
  UART_OK     = 0x00u, /**< The action was successful. */
  UART_ERROR  = 0xFFu  /**< Generic error. */
} uart_status;

uart_status uart_receive(uint8_t *data, uint16_t length);
uart_status uart_transmit_str(uint8_t *data);
uart_status uart_transmit_ch(uint8_t data);


#endif /* UART_H_ */
