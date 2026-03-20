#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/adc.h"

#include "audio.h"
#include "leds.h"
#include "buttons.h"
#include "game.h"

/* ------------------------------------------------------------------ */
/*  Core 1 — dedicado ao subsistema de áudio                          */
/*                                                                      */
/*  Protocolo FIFO:                                                     */
/*    Core 0 → Core 1 : CMD_NOTA_x | CMD_ERRO | CMD_VITORIA            */
/*    Core 1 → Core 0 : CMD_PRONTO  (ao concluir cada som)             */
/* ------------------------------------------------------------------ */

static void core1_audio_task(void) {
    audio_init();

    while (true) {
        uint32_t cmd = multicore_fifo_pop_blocking();

        if (cmd <= CMD_NOTA_3) {
            audio_tocar_nota((uint)cmd);   /* envia CMD_PRONTO ao terminar */
        } else if (cmd == CMD_ERRO) {
            audio_tocar_erro();            /* envia CMD_PRONTO ao terminar */
        } else if (cmd == CMD_VITORIA) {
            audio_tocar_vitoria();         /* envia CMD_PRONTO ao terminar */
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Helpers do Core 0                                                   */
/*  Enviam comando e BLOQUEIAM até o Core 1 confirmar CMD_PRONTO        */
/* ------------------------------------------------------------------ */

static void tocar_nota_sincronizado(uint indice) {
    multicore_fifo_push_blocking((uint32_t)indice);
    multicore_fifo_pop_blocking();   /* aguarda CMD_PRONTO do Core 1 */
}

static void tocar_erro_sincronizado(void) {
    multicore_fifo_push_blocking(CMD_ERRO);
    multicore_fifo_pop_blocking();
}

static void tocar_vitoria_sincronizado(void) {
    multicore_fifo_push_blocking(CMD_VITORIA);
    multicore_fifo_pop_blocking();
}

/* ------------------------------------------------------------------ */
/*  Core 0 — lógica principal do jogo                                  */
/* ------------------------------------------------------------------ */

int main(void) {
    stdio_init_all();

    /*
     * Seed pseudoaleatória:
     * Lemos um pino ADC flutuante (ruído analógico) para variar a seed
     * a cada energização, atendendo ao requisito do srand().
     */
    adc_init();
    adc_gpio_init(26);       /* GPIO 26 = ADC0, deixado flutuante */
    adc_select_input(0);
    uint32_t seed = adc_read();
    seed ^= (uint32_t)time_us_32();
    srand(seed);

    /* Inicializa periféricos do Core 0 */
    leds_init();
    buttons_init();

    /* Inicia Core 1 com a tarefa de áudio */
    multicore_launch_core1(core1_audio_task);

    jogo_t jogo;

novo_jogo:
    game_init(&jogo);
    game_gerar_sequencia(&jogo);

    printf("=== GENIUS iniciado (seed=%lu) ===\n", (unsigned long)seed);

    /* Animação de início: varre LEDs sincronizado com áudio */
    for (int i = 0; i < NUM_LEDS; i++) {
        led_acender(i);
        tocar_nota_sincronizado(i);  /* LED apaga só após confirmar fim do som */
        led_apagar(i);
    }
    sleep_ms(400);

    while (true) {

        /* --- Mostra a sequência --- */
        jogo.estado = ESTADO_MOSTRAR;
        printf("Nivel %d | Pontuacao: %d\n", jogo.nivel, jogo.pontuacao);

        sleep_ms(600);
        for (int i = 0; i < jogo.nivel; i++) {
            int btn = jogo.sequencia[i];
            led_acender(btn);
            tocar_nota_sincronizado(btn);  /* espera CMD_PRONTO antes de apagar */
            led_apagar(btn);
            sleep_ms(200);
        }

        /* --- Coleta entrada do jogador --- */
        jogo.estado = ESTADO_ENTRADA;
        int pos   = 0;
        bool erro = false;

        while (pos < jogo.nivel && !erro) {
            int btn = buttons_aguardar();

            /* Feedback imediato: acende LED + toca nota sincronizado */
            led_acender(btn);
            tocar_nota_sincronizado(btn);
            led_apagar(btn);

            if (btn != jogo.sequencia[pos]) {
                erro = true;
            } else {
                pos++;
            }
        }

        /* --- Avalia resultado --- */
        if (erro) {
            jogo.estado = ESTADO_ERRO;
            printf("ERRO! Esperado: %d | Pontuacao final: %d\n",
                   jogo.sequencia[pos], jogo.pontuacao);

            /* Core 1 toca erro; Core 0 pisca LEDs após confirmação */
            tocar_erro_sincronizado();
            leds_piscar_todos(3, 100, 100);

            sleep_ms(1000);
            goto novo_jogo;
        }

        /* Acertou o nível */
        jogo.pontuacao++;
        jogo.estado = ESTADO_ACERTO;
        printf("Acertou nivel %d!\n", jogo.nivel);

        if (jogo.nivel == MAX_SEQUENCIA) {
            jogo.estado = ESTADO_VITORIA;
            printf("VITORIA! Pontuacao: %d\n", jogo.pontuacao);
            tocar_vitoria_sincronizado();
            leds_piscar_todos(5, 100, 60);
            sleep_ms(2000);
            goto novo_jogo;
        }

        /* Feedback de acerto e avança nível */
        leds_piscar_todos(1, 150, 0);
        sleep_ms(500);
        jogo.nivel++;
    }

    return 0;
}