Jogo genius

# Genius

Jogo da memória para Raspberry Pi Pico 2, feito em C com o SDK oficial.

## Hardware

| Componente | GPIO |
|---|---|
| LED Verde | 20 |
| LED Vermelho | 21 |
| LED Azul | 18 |
| LED Amarelo | 19 |
| Botão Verde | 3 |
| Botão Vermelho | 2 |
| Botão Azul | 5 |
| Botão Amarelo | 4 |
| Alto-falante | 28 |

## Como funciona

- Core 0: lógica do jogo, LEDs e botões
- Core 1: áudio PWM em loop
- Pressionar qualquer botão durante a sequência liga/desliga a música
- Ao errar, os LEDs mostram a pontuação final

