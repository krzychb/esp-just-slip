#ifndef SOFTUART_H_
#define SOFTUART_H_

#include "user_interface.h"

#define SOFTUART_MAX_RX_BUFF 64 

#define SOFTUART_GPIO_COUNT 16

typedef struct softuart_pin_t {
	uint8_t gpio_id;
	uint32_t gpio_mux_name;
	uint8_t gpio_func;
} softuart_pin_t;

typedef struct softuart_buffer_t {
	char receive_buffer[SOFTUART_MAX_RX_BUFF]; 
	uint8_t receive_buffer_tail;
	uint8_t receive_buffer_head;
	uint8_t buffer_overflow; 
} softuart_buffer_t;

typedef struct {
	softuart_pin_t pin_rx;
	softuart_pin_t pin_tx;
	//optional rs485 tx enable pin (high -> tx enabled)
	uint8_t pin_rs485_tx_enable;
	//wether or not this softuart is rs485 and controlls rs485 tx enable pin
	uint8_t is_rs485;
	volatile softuart_buffer_t buffer;
	uint16_t bit_time;
} Softuart;


BOOL Softuart_Available(Softuart *s);
void Softuart_Intr_Handler(Softuart *s);


//define mapping from pin to functio mode
typedef struct {
	uint32_t gpio_mux_name;
	uint8_t gpio_func;
} softuart_reg_t;

#endif /* SOFTUART_H_ */
