#ifndef LEDS_H
#define LEDS_H

#include "pico/stdlib.h"

/* GPIOs dos LEDs — ordem igual à dos botões */
#define LED_VERDE    11
#define LED_VERMELHO 12
#define LED_AZUL     13
#define LED_AMARELO  14

#define NUM_LEDS 4

void leds_init(void);
void led_acender(uint indice);
void led_apagar(uint indice);
void led_apagar_todos(void);
void led_piscar(uint indice, uint vezes, uint ms_on, uint ms_off);
void leds_piscar_todos(uint vezes, uint ms_on, uint ms_off);

#endif