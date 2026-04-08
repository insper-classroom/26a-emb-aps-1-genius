#include "game.h"
#include "leds.h"
#include "audio.h"
#include "pico/stdlib.h"
#include <stdlib.h>
#include <stdio.h>

static const uint INTERVALO_SHOW_MS = 600u;
static const uint PAUSA_ENTRE_MS = 200u;

void game_init(jogo_t *jogo) {
    jogo->nivel = 1;
    jogo->pontuacao = 0;
    jogo->estado = ESTADO_INICIO;
}

void game_gerar_sequencia(jogo_t *jogo) {
    for (int i = 0; i < MAX_SEQUENCIA; i++) {
        jogo->sequencia[i] = rand() % 4;
    }
}

void game_mostrar_sequencia(jogo_t *jogo) {
    sleep_ms(INTERVALO_SHOW_MS);
    for (int i = 0; i < jogo->nivel; i++) {
        int btn = jogo->sequencia[i];
        led_acender(btn);
        audio_tocar_nota(btn);
        led_apagar(btn);
        sleep_ms(PAUSA_ENTRE_MS);
    }
}

// 1 acertou este passo, sequência ainda não completa
// 2 acertou e completou o nível (avança)
// 0 errou
int game_verificar_entrada(jogo_t *jogo, int btn) {
    static int pos = 0;

    if (jogo->estado == ESTADO_MOSTRAR) {
        pos = 0;
    }

    if (btn != jogo->sequencia[pos]) {
        pos = 0;
        return 0;
    }

    pos++;
    if (pos == jogo->nivel) {
        pos = 0;
        return 2;
    }
    return 1;
}

void game_feedback_acerto() {
    leds_piscar_todos(1, 150, 80);
}

void game_feedback_erro() {
    led_apagar_todos();
    audio_tocar_erro();
    leds_piscar_todos(3, 100, 100);
}

void game_feedback_vitoria() {
    audio_tocar_vitoria();
    leds_piscar_todos(5, 100, 60);
}