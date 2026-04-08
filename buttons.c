#include "buttons.h"
#include "pico/stdlib.h"
#include "hardware/irq.h"

static const uint PINOS_BOTOES[NUM_BOTOES] = {BTN_0, BTN_1, BTN_2, BTN_3};
static volatile int flag_botao[NUM_BOTOES] = {0, 0, 0, 0};
static volatile absolute_time_t ultimo_press[NUM_BOTOES];

static void gpio_isr(uint gpio, uint32_t events) {
    absolute_time_t agora = get_absolute_time();
    uint i;

    if (!(events & GPIO_IRQ_EDGE_FALL)) return;

    for (i = 0; i < NUM_BOTOES; i++) {
        if (gpio == PINOS_BOTOES[i]) {
            if (absolute_time_diff_us(ultimo_press[i], agora) < DEBOUNCE_US) return;
            ultimo_press[i] = agora;
            flag_botao[i] = 1;
            return;
        }
    }
}

void buttons_init() {
    uint i;
    for (i = 0; i < NUM_BOTOES; i++) {
        ultimo_press[i] = get_absolute_time();
        gpio_init(PINOS_BOTOES[i]);
        gpio_set_dir(PINOS_BOTOES[i], GPIO_IN);
        gpio_pull_up(PINOS_BOTOES[i]);
        gpio_set_irq_enabled_with_callback(
            PINOS_BOTOES[i],
            GPIO_IRQ_EDGE_FALL,
            true,
            &gpio_isr
        );
    }
}

int buttons_ler() {
    uint i;
    for (i = 0; i < NUM_BOTOES; i++) {
        if (flag_botao[i]) {
            flag_botao[i] = 0;
            return (int)i;
        }
    }
    return -1;
}

int buttons_aguardar() {
    int btn = -1;
    while (btn == -1) {
        btn = buttons_ler();
        tight_loop_contents();
    }
    return btn;
}