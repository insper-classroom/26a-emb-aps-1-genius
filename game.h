#ifndef GAME_H
#define GAME_H

#include "pico/stdlib.h"

#define MAX_SEQUENCIA 20

typedef enum {
    ESTADO_INICIO,
    ESTADO_MOSTRAR,
    ESTADO_ENTRADA,
    ESTADO_ACERTO,
    ESTADO_ERRO,
    ESTADO_VITORIA
} estado_jogo_t;

typedef struct {
    int sequencia[MAX_SEQUENCIA];
    int nivel;          
    int pontuacao;
    estado_jogo_t estado;
} jogo_t;

void game_init(jogo_t *jogo);
void game_gerar_sequencia(jogo_t *jogo);
void game_mostrar_sequencia(jogo_t *jogo);
int  game_verificar_entrada(jogo_t *jogo, int btn);
void game_feedback_acerto(void);
void game_feedback_erro(void);
void game_feedback_vitoria(void);

#endif