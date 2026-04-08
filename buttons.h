#ifndef BUTTONS_H
#define BUTTONS_H

#include "pico/stdlib.h"

#define BTN_0 3   // VERDE 
#define BTN_1 2   // VERMELHO
#define BTN_2 5   // AZUL     
#define BTN_3 4   // AMARELO

#define NUM_BOTOES  4
#define DEBOUNCE_US 200000

void buttons_init();
int  buttons_ler();
int  buttons_aguardar();
int  buttons_aguardar_timeout(uint32_t timeout_ms);

#endif