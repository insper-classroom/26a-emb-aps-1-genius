#ifndef BUTTONS_H
#define BUTTONS_H

#include "pico/stdlib.h"

#define BTN_0 5 //VERDE
#define BTN_1 6 //VERMELHO
#define BTN_2 7 //AZUL
#define BTN_3 8 //AMARELO

#define NUM_BOTOES 4
#define DEBOUNCE_US 200000

void buttons_init(void);

/* Retorna o índice do botão pressionado (0-3) ou -1 se nenhum */
int buttons_ler(void);

/* Bloqueia até um botão ser pressionado; retorna o índice (0-3) */
int buttons_aguardar(void);

#endif