#include "buttons.h"
#include "pico/stdlib.h"
#include "hardware/irq.h"

static const uint PINOS_BOTOES[NUM_BOTOES] = {BTN_0, BTN_1, BTN_2, BTN_3};

static volatile int   _flag_botao[NUM_BOTOES] = {0, 0, 0, 0};
static volatile absolute_time_t _ultimo[NUM_BOTOES];

static void _gpio_isr(uint gpio, uint32_t events) {
    if (!(events & GPIO_IRQ_EDGE_FALL)) return;

    for (int i = 0; i < NUM_BOTOES; i++) {
        if (gpio == PINOS_BOTOES[i]) {
            absolute_time_t agora = get_absolute_time();
            if (absolute_time_diff_us(_ultimo[i], agora) < DEBOUNCE_US) return;
            _ultimo[i]    = agora;
            _flag_botao[i] = 1;
            return;
        }
    }
}

void buttons_init(void) {
    for (int i = 0; i < NUM_BOTOES; i++) {
        _ultimo[i] = get_absolute_time();
        gpio_init(PINOS_BOTOES[i]);
        gpio_set_dir(PINOS_BOTOES[i], GPIO_IN);
        gpio_pull_up(PINOS_BOTOES[i]);
        gpio_set_irq_enabled_with_callback(
            PINOS_BOTOES[i],
            GPIO_IRQ_EDGE_FALL,
            true,
            &_gpio_isr
        );
    }
}

int buttons_ler(void) {
    for (int i = 0; i < NUM_BOTOES; i++) {
        if (_flag_botao[i]) {
            _flag_botao[i] = 0;
            return i;
        }
    }
    return -1;
}

int buttons_aguardar(void) {
    int btn = -1;
    while (btn == -1) {
        btn = buttons_ler();
        tight_loop_contents();
    }
    return btn;
}