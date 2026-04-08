#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"

#include "audio.h"
#include "leds.h"
#include "buttons.h"
#include "game.h"

/* ------------------------------------------------------------------ */
/*  Core 1 — audio                                                      */
/*                                                                      */
/*  Protocolo FIFO:                                                     */
/*    Core 0 -> Core 1 : CMD_NOTA_x | CMD_ERRO | CMD_VITORIA           */
/*                       CMD_BG_ON  | CMD_BG_OFF                       */
/*    Core 1 -> Core 0 : CMD_PRONTO (ao concluir efeito)               */
/* ------------------------------------------------------------------ */
static void core1_audio_task(void) {
    audio_init();

    while (true) {
        uint32_t cmd = multicore_fifo_pop_blocking();

        if (cmd == CMD_BG_ON) {
            audio_bg_ligar();
        } else if (cmd == CMD_BG_OFF) {
            audio_bg_desligar();
        } else if (cmd <= CMD_NOTA_3) {
            audio_tocar_nota((uint)cmd);
        } else if (cmd == CMD_ERRO) {
            audio_tocar_erro();
        } else if (cmd == CMD_VITORIA) {
            audio_tocar_vitoria();
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Helpers Core 0                                                      */
/* ------------------------------------------------------------------ */
static void tocar_nota_sincronizado(uint indice) {
    multicore_fifo_push_blocking((uint32_t)indice);
    multicore_fifo_pop_blocking();
}

static void tocar_erro_sincronizado(void) {
    multicore_fifo_push_blocking(CMD_ERRO);
    multicore_fifo_pop_blocking();
}

static void tocar_vitoria_sincronizado(void) {
    multicore_fifo_push_blocking(CMD_VITORIA);
    multicore_fifo_pop_blocking();
}

/*
 * Exibe pontuacao piscando os LEDs:
 * Cada grupo de 4 = pisca todos juntos uma vez
 * Resto = pisca LEDs individuais em sequencia
 * Ex: pontuacao 6 -> 1 piscada geral + 2 individuais
 */
static void exibir_pontuacao_leds(int pontuacao) {
    int grupos = pontuacao / NUM_LEDS;
    int resto  = pontuacao % NUM_LEDS;
    int g;
    int r;

    sleep_ms(500);

    for (g = 0; g < grupos; g++) {
        leds_piscar_todos(1, 300, 150);
    }
    for (r = 0; r < resto; r++) {
        led_piscar((uint)r, 1, 300, 150);
    }

    sleep_ms(500);
}

/* ------------------------------------------------------------------ */
/*  Core 0 — logica do jogo                                            */
/* ------------------------------------------------------------------ */
int main(void) {
    /* clock 176 MHz necessario para o PWM de audio WAV */
    set_sys_clock_khz(176000, true);
    stdio_init_all();

    /*
     * Seed pseudoaleatoria:
     * Pino ADC flutuante (ruido analogico) XOR tempo de boot
     * Seed diferente a cada energizacao — atende requisito do srand()
     */
    adc_init();
    adc_gpio_init(26);
    adc_select_input(0);
    uint32_t seed = adc_read();
    seed ^= (uint32_t)time_us_32();
    srand(seed);

    leds_init();
    buttons_init();

    multicore_launch_core1(core1_audio_task);

    /* inicia musica de fundo */
    multicore_fifo_push_blocking(CMD_BG_ON);

    jogo_t jogo;

novo_jogo:
    game_init(&jogo);
    game_gerar_sequencia(&jogo);

    printf("=== GENIUS iniciado (seed=%lu) ===\n", (unsigned long)seed);

    /* animacao de inicio */
    {
        int i;
        for (i = 0; i < NUM_LEDS; i++) {
            led_acender((uint)i);
            tocar_nota_sincronizado((uint)i);
            led_apagar((uint)i);
        }
    }
    sleep_ms(400);

    while (true) {
        int i;
        int pos;
        bool erro;

        /* --- mostra sequencia ---
         * Durante esta fase, qualquer botao pressionado togla a musica
         */
        jogo.estado = ESTADO_MOSTRAR;
        printf("Nivel %d | Pontuacao: %d\n", jogo.nivel, jogo.pontuacao);

        sleep_ms(600);
        for (i = 0; i < jogo.nivel; i++) {
            int btn_seq = jogo.sequencia[i];

            /* verifica se jogador pressionou algo durante a sequencia (toggle BG) */
            {
                int pressionado = buttons_ler();
                if (pressionado != -1) {
                    /* qualquer botao durante a exibicao togla a musica */
                    static bool bg_ligado = true;
                    bg_ligado = !bg_ligado;
                    if (bg_ligado) {
                        multicore_fifo_push_blocking(CMD_BG_ON);
                    } else {
                        multicore_fifo_push_blocking(CMD_BG_OFF);
                    }
                }
            }

            led_acender((uint)btn_seq);
            tocar_nota_sincronizado((uint)btn_seq);
            led_apagar((uint)btn_seq);
            sleep_ms(200);
        }

        /* --- coleta entrada do jogador --- */
        jogo.estado = ESTADO_ENTRADA;
        pos  = 0;
        erro = false;

        while (pos < jogo.nivel && !erro) {
            int btn = buttons_aguardar();

            led_acender((uint)btn);
            tocar_nota_sincronizado((uint)btn);
            led_apagar((uint)btn);

            if (btn != jogo.sequencia[pos]) {
                erro = true;
            } else {
                pos++;
            }
        }

        /* --- avalia resultado --- */
        if (erro) {
            jogo.estado = ESTADO_ERRO;
            printf("ERRO! Pontuacao final: %d\n", jogo.pontuacao);

            tocar_erro_sincronizado();
            leds_piscar_todos(3, 100, 100);
            exibir_pontuacao_leds(jogo.pontuacao);

            sleep_ms(1000);
            goto novo_jogo;
        }

        /* acertou o nivel */
        jogo.pontuacao++;
        jogo.estado = ESTADO_ACERTO;
        printf("Acertou nivel %d!\n", jogo.nivel);

        if (jogo.nivel == MAX_SEQUENCIA) {
            jogo.estado = ESTADO_VITORIA;
            printf("VITORIA! Pontuacao: %d\n", jogo.pontuacao);
            tocar_vitoria_sincronizado();
            leds_piscar_todos(5, 100, 60);
            exibir_pontuacao_leds(jogo.pontuacao);
            sleep_ms(2000);
            goto novo_jogo;
        }

        leds_piscar_todos(1, 150, 0);
        sleep_ms(500);
        jogo.nivel++;
    }

    return 0;
}