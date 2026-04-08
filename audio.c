#include "audio.h"
#include "audio_musica.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"

//pwm
static const float PWM_CLK = 8.0f;
static const uint PWM_WRAP = 250u;
static const uint32_t SKIP = 11u; /* 88000 Hz / 8000 Hz */
static const uint DUTY_FIXO = 80u;
//duracao
static const uint DURACAO_NOTA_MS = 400u;
static const uint DURACAO_ERRO_MS = 600u;
static const uint DURACAO_VITORIA_MS = 150u;
//cores
static const uint FREQ_NOTAS[4] = {262u, 330u, 392u, 494u};

/* variaveis globais da ISR — volatile obrigatorio */
static volatile uint32_t pos_bg      = 0u;
static volatile uint32_t isr_counter = 0u;
static volatile bool     bg_ativo    = false;
static volatile bool     efeito_ativo = false;

/* slice: lido mas nao escrito pela ISR */
static int slice = 0;

static void pwm_isr() {
    pwm_clear_irq((uint)slice);

    if (!bg_ativo || efeito_ativo) {
        pwm_set_gpio_level(AUDIO_PIN, 0);
        return;
    }

    isr_counter++;
    if (isr_counter < SKIP) return;
    isr_counter = 0;

    if (pos_bg >= WAV_DATA_MUSICA_LENGTH) {
        pos_bg = 0u;
    }
    pwm_set_gpio_level(AUDIO_PIN, WAV_DATA_MUSICA[pos_bg]);
    pos_bg++;
}

static void init_pwm_wav() {
    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);
    slice = (int)pwm_gpio_to_slice_num(AUDIO_PIN);

    pwm_clear_irq((uint)slice);
    pwm_set_irq_enabled((uint)slice, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pwm_isr);
    irq_set_enabled(PWM_IRQ_WRAP, true);

    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, PWM_CLK);
    pwm_config_set_wrap(&cfg, PWM_WRAP);
    pwm_init((uint)slice, &cfg, true);
    pwm_set_gpio_level(AUDIO_PIN, 0);
}

static void set_frequencia_simples(uint freq_hz) {
    irq_set_enabled(PWM_IRQ_WRAP, false);
    pwm_set_irq_enabled((uint)slice, false);

    uint sys_hz = clock_get_hz(clk_sys);
    uint wrap   = sys_hz / freq_hz - 1u;
    pwm_set_wrap((uint)slice, wrap);
    pwm_set_gpio_level(AUDIO_PIN, DUTY_FIXO);
}

static void restaurar_pwm_wav() {
    pwm_set_wrap((uint)slice, PWM_WRAP);
    pwm_set_gpio_level(AUDIO_PIN, 0);
    pwm_clear_irq((uint)slice);
    pwm_set_irq_enabled((uint)slice, true);
    irq_set_enabled(PWM_IRQ_WRAP, true);
}

void audio_init() {
    init_pwm_wav();
}

void audio_bg_ligar() {
    bg_ativo     = true;
    efeito_ativo = false;
}

void audio_bg_desligar() {
    bg_ativo     = false;
    efeito_ativo = false;
    pwm_set_gpio_level(AUDIO_PIN, 0);
}

void audio_tocar_nota(uint indice_botao) {
    if (indice_botao >= 4u) {
        multicore_fifo_push_blocking(CMD_PRONTO);
        return;
    }

    efeito_ativo = true;
    set_frequencia_simples(FREQ_NOTAS[indice_botao]);
    sleep_ms(DURACAO_NOTA_MS);
    pwm_set_gpio_level(AUDIO_PIN, 0);
    restaurar_pwm_wav();
    efeito_ativo = false;

    multicore_fifo_push_blocking(CMD_PRONTO);
}

void audio_tocar_erro() {
    static const uint freqs[2] = {180u, 140u};
    int i;

    efeito_ativo = true;

    for (i = 0; i < 2; i++) {
        set_frequencia_simples(freqs[i]);
        sleep_ms(DURACAO_ERRO_MS / 2u);
        pwm_set_gpio_level(AUDIO_PIN, 0);
        sleep_ms(80u);
    }

    restaurar_pwm_wav();
    efeito_ativo = false;

    multicore_fifo_push_blocking(CMD_PRONTO);
}

void audio_tocar_vitoria() {
    static const uint vitoria[5] = {262u, 330u, 392u, 523u, 659u};
    int i;

    efeito_ativo = true;

    for (i = 0; i < 5; i++) {
        set_frequencia_simples(vitoria[i]);
        sleep_ms(DURACAO_VITORIA_MS);
        pwm_set_gpio_level(AUDIO_PIN, 0);
        sleep_ms(40u);
    }

    restaurar_pwm_wav();
    efeito_ativo = false;

    multicore_fifo_push_blocking(CMD_PRONTO);
}

void audio_parar() {
    bg_ativo     = false;
    efeito_ativo = false;
    pwm_set_gpio_level(AUDIO_PIN, 0);
}