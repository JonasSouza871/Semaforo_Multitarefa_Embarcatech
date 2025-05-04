#ifndef MATRIZ_LED_H
#define MATRIZ_LED_H

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "generated/ws2812.pio.h"

/* ---------- Hardware ---------- */
#define PINO_WS2812   7
#define NUM_LINHAS    5
#define NUM_COLUNAS   5
#define NUM_PIXELS    (NUM_LINHAS * NUM_COLUNAS)
#define RGBW_ATIVO    false

/* ---------- Utilidades de cor ---------- */
#define GRB(r,g,b)   ( ((uint32_t)(g) << 16) | ((uint32_t)(r) << 8) | (b) )

extern const uint32_t COR_VERDE;
extern const uint32_t COR_AMARELO;
extern const uint32_t COR_VERMELHO;
extern const uint32_t COR_OFF;

/* Padrões 5 × 5 (✓, !, X) */
extern const uint8_t PAD_OK[5];
extern const uint8_t PAD_EXC[5];
extern const uint8_t PAD_X[5];

/* API mínima */
void inicializar_matriz_led(void);
void matriz_draw_pattern(const uint8_t pad[5], uint32_t cor_on);
void matriz_clear(void);

#endif /* MATRIZ_LED_H */
