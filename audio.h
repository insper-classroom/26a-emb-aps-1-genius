#ifndef AUDIO_H
#define AUDIO_H

#include "pico/stdlib.h"

#define AUDIO_PIN 28   /* alto-falante — NAO conflita com ADC (GPIO 26) */

/* Comandos trocados via FIFO entre Core 0 e Core 1 */
#define CMD_NOTA_0   0u
#define CMD_NOTA_1   1u
#define CMD_NOTA_2   2u
#define CMD_NOTA_3   3u
#define CMD_ERRO     10u
#define CMD_VITORIA  11u
#define CMD_BG_ON    20u
#define CMD_BG_OFF   21u
#define CMD_BG       CMD_BG_ON
#define CMD_PRONTO   99u   /* Core 1 → Core 0: efeito concluido */

void audio_init(void);
void audio_bg_ligar(void);
void audio_bg_desligar(void);
void audio_tocar_nota(uint indice_botao);
void audio_tocar_erro(void);
void audio_tocar_vitoria(void);
void audio_parar(void);

#endif