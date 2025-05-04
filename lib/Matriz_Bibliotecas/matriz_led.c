#include "matriz_led.h"

/* ---------- Cores ---------- */
const uint32_t COR_VERDE    = GRB(0,   150, 0);
const uint32_t COR_AMARELO  = GRB(255, 140, 0);
const uint32_t COR_VERMELHO = GRB(190,   0, 0);
const uint32_t COR_OFF      = 0;

/* ---------- Padrões gráficos (MSB-à-esquerda) ---------- */
const uint8_t PAD_OK[5]  = {0b00001,0b00010,0b00100,0b11000,0b10000};
const uint8_t PAD_EXC[5] = {0b00100,0b00100,0b00100,0b00000,0b00100};
const uint8_t PAD_X[5]   = {0b10001,0b01010,0b00100,0b01010,0b10001};

/* ---------- Aux interno ---------- */
static inline void ws2812_put(uint32_t grb)
{   pio_sm_put_blocking(pio0, 0, grb << 8u); }

/* ---------- Implementação da API ---------- */
void inicializar_matriz_led(void)
{
    PIO pio = pio0;
    uint off = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, 0, off, PINO_WS2812, 800000, RGBW_ATIVO);
}

void matriz_draw_pattern(const uint8_t pad[5], uint32_t cor_on)
{
    /* placa montada “de cabeça-para-baixo” → linha 4 primeiro */
    for (int lin = 4; lin >= 0; --lin) {
        for (int col = 0; col < 5; ++col) {
            bool aceso = pad[lin] & (1 << (4 - col));
            ws2812_put(aceso ? cor_on : COR_OFF);
        }
    }
    sleep_us(60);               /* latch */
}

void matriz_clear(void)
{
    for (int i = 0; i < NUM_PIXELS; ++i)
        ws2812_put(COR_OFF);
    sleep_us(60);
}
