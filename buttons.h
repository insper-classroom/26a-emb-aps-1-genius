#ifndef BUTTONS_H
#define BUTTONS_H

#include "pico/stdlib.h"

#define BTN_0 3   //Verde
#define BTN_1 2   //Vermelho
#define BTN_2 5   //azul
#define BTN_3 4   //amarelo

#define NUM_BOTOES  4
#define DEBOUNCE_US 200000

void buttons_init(void);
int  buttons_ler(void);
int  buttons_aguardar(void);

#endif