#include "buttons.h"
#include "pico/stdlib.h"
#include "hardware/irq.h"

static const uint PINOS_BOTOES[NUM_BOTOES] = {BTN_0, BTN_1, BTN_2, BTN_3};

static volatile int flag_botao[NUM_BOTOES] = {0, 0, 0, 0};
static volatile uint32_t ultimo_press[NUM_BOTOES];
static volatile int flag_alarm = 0;

static int64_t alarm_callback(alarm_id_t id, void *user_data) {
    flag_alarm = 1;
    return 0;
}

static void gpio_isr(uint gpio, uint32_t events) {
    uint32_t agora = time_us_32();
    uint i;

    if (!(events & GPIO_IRQ_EDGE_FALL)) return;

    for (i = 0; i < NUM_BOTOES; i++) {
        if (gpio == PINOS_BOTOES[i]) {
            if ((agora - ultimo_press[i]) < DEBOUNCE_US) return;
            ultimo_press[i] = agora;
            flag_botao[i] = 1;
            return;
        }
    }
}

void buttons_init() {
    uint i;
    for (i = 0; i < NUM_BOTOES; i++) {
        ultimo_press[i] = time_us_32();
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

// retorna o botao pressionado ou -1 se timeout
int buttons_aguardar_timeout(uint32_t timeout_ms) {
    int btn = -1;
    flag_alarm = 0;
    alarm_id_t alarm = add_alarm_in_ms(timeout_ms, alarm_callback, NULL, false);

    while (btn == -1 && !flag_alarm) {
        btn = buttons_ler();
        tight_loop_contents();
    }

    if (flag_alarm) return -1;
    cancel_alarm(alarm);
    return btn;
}