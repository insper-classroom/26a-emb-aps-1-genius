#ifndef AUDIO_H
#define AUDIO_H

#include "pico/stdlib.h"
#define AUDIO_PIN 21

/* Comandos trocados via FIFO entre Core 0 e Core 1 */
#define CMD_NOTA_0   0u
#define CMD_NOTA_1   1u
#define CMD_NOTA_2   2u
#define CMD_NOTA_3   3u
#define CMD_ERRO     10u
#define CMD_VITORIA  11u
#define CMD_PRONTO   99u   /* Core 1 → Core 0: som concluído */

void audio_init(void);
void audio_tocar_nota(uint indice_botao);
void audio_tocar_erro(void);
void audio_tocar_vitoria(void);
void audio_parar(void);

#endif