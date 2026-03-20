#include "audio.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"

/* Frequências (Hz) associadas a cada botão */
static const uint FREQ_NOTAS[4] = {262, 330, 392, 523};

/* Duração de cada tipo de som em ms */
#define DURACAO_NOTA_MS    400
#define DURACAO_ERRO_MS    600
#define DURACAO_VITORIA_MS 150

/* Estado interno — acessado apenas pelo Core 1, sem necessidade de volatile */
static int _slice = 0;

/* ---------- helpers internos ---------- */

static void _set_frequencia(uint freq_hz) {
    uint sys_hz = clock_get_hz(clk_sys);
    uint wrap   = sys_hz / freq_hz - 1;
    pwm_set_wrap(_slice, wrap);
    pwm_set_gpio_level(AUDIO_PIN, wrap / 2);  /* duty 50% */
}

static void _parar_pwm(void) {
    pwm_set_gpio_level(AUDIO_PIN, 0);
}

/* ---------- API pública ---------- */

void audio_init(void) {
    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);
    _slice = pwm_gpio_to_slice_num(AUDIO_PIN);

    pwm_config cfg = pwm_get_default_config();
    pwm_init(_slice, &cfg, true);
    pwm_set_gpio_level(AUDIO_PIN, 0);
}

/*
 * Todas as funções abaixo são chamadas pelo Core 1.
 * Ao terminar, enviam CMD_PRONTO via FIFO para desbloquear o Core 0.
 */

void audio_tocar_nota(uint indice_botao) {
    if (indice_botao >= 4) {
        multicore_fifo_push_blocking(CMD_PRONTO);
        return;
    }
    _set_frequencia(FREQ_NOTAS[indice_botao]);
    sleep_ms(DURACAO_NOTA_MS);
    _parar_pwm();
    multicore_fifo_push_blocking(CMD_PRONTO);
}

void audio_tocar_erro(void) {
    /* Dois bipes graves descendentes */
    for (int i = 0; i < 2; i++) {
        _set_frequencia(180 - (uint)(i * 40));
        sleep_ms(DURACAO_ERRO_MS / 2);
        _parar_pwm();
        sleep_ms(80);
    }
    multicore_fifo_push_blocking(CMD_PRONTO);
}

void audio_tocar_vitoria(void) {
    /* Melodia ascendente rápida */
    static const uint vitoria[5] = {262, 330, 392, 523, 659};
    for (int i = 0; i < 5; i++) {
        _set_frequencia(vitoria[i]);
        sleep_ms(DURACAO_VITORIA_MS);
        _parar_pwm();
        sleep_ms(40);
    }
    multicore_fifo_push_blocking(CMD_PRONTO);
}

void audio_parar(void) {
    _parar_pwm();
}