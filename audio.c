#include "audio.h"
#include "audio_musica.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"


#define PWM_CLKDIV 8.0f
#define PWM_WRAP 250
#define SAMPLE_RATE 8000
#define ISR_RATE 88000u   /* 176MHz / (8 * 250) */
#define SKIP ((uint32_t)(ISR_RATE / SAMPLE_RATE))  

#define DUTY_FIXO 80u

// frequencias das cores
static const uint FREQ_NOTAS[4] = {262, 330, 392, 494};

#define DURACAO_NOTA_MS    400
#define DURACAO_ERRO_MS    600
#define DURACAO_VITORIA_MS 150

static volatile uint32_t _pos_bg= 0;
static volatile uint32_t _isr_counter= 0;
static volatile bool _bg_ativo= false;
static volatile bool _efeito_ativo = false;

static int _slice = 0;

static void _pwm_isr(void) {
    pwm_clear_irq(_slice);

    if (!_bg_ativo || _efeito_ativo) {
        pwm_set_gpio_level(AUDIO_PIN, 0);
        return;
    }

    _isr_counter++;
    if (_isr_counter < SKIP) return;
    _isr_counter = 0;

    if (_pos_bg >= WAV_DATA_MUSICA_LENGTH) {
        _pos_bg = 0;
    }
    pwm_set_gpio_level(AUDIO_PIN, WAV_DATA_MUSICA[_pos_bg]);
    _pos_bg++;
}

static void _init_pwm_wav(void) {
    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);
    _slice = (int)pwm_gpio_to_slice_num(AUDIO_PIN);

    pwm_clear_irq((uint)_slice);
    pwm_set_irq_enabled((uint)_slice, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, _pwm_isr);
    irq_set_enabled(PWM_IRQ_WRAP, true);

    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, PWM_CLKDIV);
    pwm_config_set_wrap(&cfg, PWM_WRAP);
    pwm_init((uint)_slice, &cfg, true);
    pwm_set_gpio_level(AUDIO_PIN, 0);
}

static void _set_frequencia_simples(uint freq_hz) {
    irq_set_enabled(PWM_IRQ_WRAP, false);
    pwm_set_irq_enabled((uint)_slice, false);

    uint sys_hz = clock_get_hz(clk_sys);
    uint wrap   = sys_hz / freq_hz - 1u;
    pwm_set_wrap((uint)_slice, wrap);
    pwm_set_gpio_level(AUDIO_PIN, DUTY_FIXO);
}

static void _restaurar_pwm_wav(void) {
    pwm_set_wrap((uint)_slice, PWM_WRAP);
    pwm_set_gpio_level(AUDIO_PIN, 0);
    pwm_clear_irq((uint)_slice);
    pwm_set_irq_enabled((uint)_slice, true);
    irq_set_enabled(PWM_IRQ_WRAP, true);
}


void audio_init(void) {
    _init_pwm_wav();
}

void audio_bg_ligar(void) {
    _bg_ativo     = true;
    _efeito_ativo = false;
}

void audio_bg_desligar(void) {
    _bg_ativo     = false;
    _efeito_ativo = false;
    pwm_set_gpio_level(AUDIO_PIN, 0);
}

void audio_tocar_nota(uint indice_botao) {
    if (indice_botao >= 4u) {
        multicore_fifo_push_blocking(CMD_PRONTO);
        return;
    }

    _efeito_ativo = true;
    _set_frequencia_simples(FREQ_NOTAS[indice_botao]);
    sleep_ms(DURACAO_NOTA_MS);
    pwm_set_gpio_level(AUDIO_PIN, 0);
    _restaurar_pwm_wav();
    _efeito_ativo = false;

    multicore_fifo_push_blocking(CMD_PRONTO);
}

void audio_tocar_erro(void) {
    uint freqs[2] = {180u, 140u};
    int i;
    _efeito_ativo = true;

    for (i = 0; i < 2; i++) {
        _set_frequencia_simples(freqs[i]);
        sleep_ms(DURACAO_ERRO_MS / 2u);
        pwm_set_gpio_level(AUDIO_PIN, 0);
        sleep_ms(80);
    }

    _restaurar_pwm_wav();
    _efeito_ativo = false;

    multicore_fifo_push_blocking(CMD_PRONTO);
}

void audio_tocar_vitoria(void) {
    static const uint vitoria[5] = {262u, 330u, 392u, 523u, 659u};
    int i;

    _efeito_ativo = true;

    for (i = 0; i < 5; i++) {
        _set_frequencia_simples(vitoria[i]);
        sleep_ms(DURACAO_VITORIA_MS);
        pwm_set_gpio_level(AUDIO_PIN, 0);
        sleep_ms(40);
    }

    _restaurar_pwm_wav();
    _efeito_ativo = false;

    multicore_fifo_push_blocking(CMD_PRONTO);
}

void audio_parar(void) {
    _bg_ativo     = false;
    _efeito_ativo = false;
    pwm_set_gpio_level(AUDIO_PIN, 0);
}