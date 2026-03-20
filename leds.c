#include "leds.h"
#include "pico/stdlib.h"

static const uint PINOS_LEDS[NUM_LEDS] = {
    LED_VERDE,
    LED_VERMELHO,
    LED_AZUL,
    LED_AMARELO
};

void leds_init(void) {
    for (int i = 0; i < NUM_LEDS; i++) {
        gpio_init(PINOS_LEDS[i]);
        gpio_set_dir(PINOS_LEDS[i], GPIO_OUT);
        gpio_put(PINOS_LEDS[i], 0);
    }
}

void led_acender(uint indice) {
    if (indice < NUM_LEDS) {
        gpio_put(PINOS_LEDS[indice], 1);
    }
}

void led_apagar(uint indice) {
    if (indice < NUM_LEDS) {
        gpio_put(PINOS_LEDS[indice], 0);
    }
}

void led_apagar_todos(void) {
    for (int i = 0; i < NUM_LEDS; i++) {
        gpio_put(PINOS_LEDS[i], 0);
    }
}

void led_piscar(uint indice, uint vezes, uint ms_on, uint ms_off) {
    for (uint i = 0; i < vezes; i++) {
        led_acender(indice);
        sleep_ms(ms_on);
        led_apagar(indice);
        sleep_ms(ms_off);
    }
}

void leds_piscar_todos(uint vezes, uint ms_on, uint ms_off) {
    for (uint i = 0; i < vezes; i++) {
        for (int j = 0; j < NUM_LEDS; j++) {
            gpio_put(PINOS_LEDS[j], 1);
        }
        sleep_ms(ms_on);
        for (int j = 0; j < NUM_LEDS; j++) {
            gpio_put(PINOS_LEDS[j], 0);
        }
        sleep_ms(ms_off);
    }
}