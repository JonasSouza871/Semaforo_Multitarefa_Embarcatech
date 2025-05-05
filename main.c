#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

/* Bibliotecas para o Display OLED */
#include "lib/Display_Bibliotecas/font.h"
#include "lib/Display_Bibliotecas/ssd1306.h"

/* Bibliotecas para a Matriz de LEDs 5 × 5 (WS2812) */
#include "lib/Matriz_Bibliotecas/matriz_led.h"

/* Definições de GPIO / Hardware */
#define PINO_LED_VERDE     11
#define PINO_LED_AZUL      12  // Não usado no fluxo principal
#define PINO_LED_VERMELHO  13
#define PINO_BOTAO_A       5
#define PINO_BOTAO_B       6   // Botão BOOTSEL para reset UF2
#define PINO_BUZZER        10
#define FREQUENCIA_BUZZER  2000

/* Configurações do I2C do Display OLED */
#define I2C_OLED   i2c1
#define PINO_SDA   14
#define PINO_SCL   15
#define ENDERECO_OLED 0x3C  // Endereço padrão do SSD1306
#define OLED_LARGURA  128
#define OLED_ALTURA    64

/* Estados globais do semáforo */
typedef enum { MODO_NORMAL = 0, MODO_NOTURNO } EstadoSemaforo;
volatile EstadoSemaforo estadoAtual = MODO_NORMAL;  // Estado inicial do sistema

typedef enum { LUZ_VERDE, LUZ_AMARELO, LUZ_VERMELHO, LUZ_DESLIGADO } EstadoLuz;
volatile EstadoLuz luzAtual = LUZ_VERDE;  // Estado inicial da luz

/* Temporizações */
#define TEMPO_VERDE     5000
#define TEMPO_AMARELO   3000
#define TEMPO_VERMELHO  5000
#define PISCA_ON_MS     2000 //Ligado Noturno 
#define PISCA_OFF_MS    2000 // desligado Noturno
#define INTERVALO_MS    50

/* Tarefa: Semáforo – modo normal */
void tarefaSemaforoNormal(void *pvParam) {  // CONTROLA SEQUÊNCIA DE LEDs NO MODO NORMAL
    gpio_init(PINO_LED_VERDE);    gpio_set_dir(PINO_LED_VERDE, GPIO_OUT);
    gpio_init(PINO_LED_AZUL);     gpio_set_dir(PINO_LED_AZUL,  GPIO_OUT);  // Pino reservado para debug
    gpio_init(PINO_LED_VERMELHO); gpio_set_dir(PINO_LED_VERMELHO, GPIO_OUT);

    while (true) {
        if (estadoAtual != MODO_NORMAL) {
            vTaskDelay(pdMS_TO_TICKS(INTERVALO_MS));  // PAUSA SE NÃO ESTIVER NO MODO NORMAL
            continue;
        }
        gpio_put(PINO_LED_VERDE, 1);  gpio_put(PINO_LED_VERMELHO, 0);
        luzAtual = LUZ_VERDE;  // ATUALIZA ESTADO GLOBAL
        vTaskDelay(pdMS_TO_TICKS(TEMPO_VERDE));
        if (estadoAtual != MODO_NORMAL) continue;
        gpio_put(PINO_LED_VERDE, 1);  gpio_put(PINO_LED_VERMELHO, 1);  // SIMULA AMARELO COMBINANDO LEDs
        luzAtual = LUZ_AMARELO;
        vTaskDelay(pdMS_TO_TICKS(TEMPO_AMARELO));
        if (estadoAtual != MODO_NORMAL) continue;
        gpio_put(PINO_LED_VERDE, 0);  gpio_put(PINO_LED_VERMELHO, 1);
        luzAtual = LUZ_VERMELHO;
        vTaskDelay(pdMS_TO_TICKS(TEMPO_VERMELHO));
    }
}

/* Tarefa: Semáforo – modo noturno */
void tarefaSemaforoNoturno(void *pvParam) {  // PISCA AMARELO NO MODO NOTURNO
    while (true) {
        if (estadoAtual != MODO_NOTURNO) {
            vTaskDelay(pdMS_TO_TICKS(INTERVALO_MS));  // PAUSA SE NÃO ESTIVER NO MODO NOTURNO
            continue;
        }
        gpio_put(PINO_LED_VERDE, 1);  gpio_put(PINO_LED_VERMELHO, 1);
        luzAtual = LUZ_AMARELO;  // ATUALIZA ESTADO GLOBAL
        vTaskDelay(pdMS_TO_TICKS(PISCA_ON_MS));
        if (estadoAtual != MODO_NOTURNO) continue;
        gpio_put(PINO_LED_VERDE, 0);  gpio_put(PINO_LED_VERMELHO, 0);
        luzAtual = LUZ_DESLIGADO;  // INDICA FASE APAGADA
        vTaskDelay(pdMS_TO_TICKS(PISCA_OFF_MS));
    }
}

/* Tarefa: Botão A (troca de modos) */
void tarefaBotaoA(void *pvParam) {  // DETECTA BOTÃO PARA ALTERNAR MODOS
    const uint32_t debounceMs = 50;  // Tempo de debounce
    bool pressionado = false;
    int64_t ultimoTempo = 0;

    gpio_init(PINO_BOTAO_A);
    gpio_set_dir(PINO_BOTAO_A, GPIO_IN);
    gpio_pull_up(PINO_BOTAO_A);  // Ativa pull-up interno

    while (true) {
        bool nivel = gpio_get(PINO_BOTAO_A);
        if (!nivel && !pressionado) {
            int64_t agora = to_ms_since_boot(get_absolute_time());
            if (agora - ultimoTempo > debounceMs) {
                pressionado = true;
                ultimoTempo = agora;
                estadoAtual = (estadoAtual == MODO_NORMAL) ? MODO_NOTURNO : MODO_NORMAL;  // Alterna modo
                printf("MODO alterado → %s\n", (estadoAtual == MODO_NORMAL) ? "NORMAL" : "NOTURNO");  // Log para debug
            }
        } else if (nivel) {
            pressionado = false;
        }
        vTaskDelay(pdMS_TO_TICKS(10));  // Evita consumo excessivo de CPU
    }
}

/* Tarefa: Display OLED */
void tarefaDisplay(void *pvParam) {  // ATUALIZA DISPLAY COM INFORMAÇÕES DO SEMÁFORO
    i2c_init(I2C_OLED, 400 * 1000);  // Inicializa I2C a 400kHz
    gpio_set_function(PINO_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PINO_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PINO_SDA);
    gpio_pull_up(PINO_SCL);

    ssd1306_t oled;
    ssd1306_init(&oled, OLED_LARGURA, OLED_ALTURA, false, ENDERECO_OLED, I2C_OLED);  // Configura display SSD1306
    ssd1306_config(&oled);

    ssd1306_fill(&oled, true);   ssd1306_send_data(&oled);  // Teste inicial do display
    vTaskDelay(pdMS_TO_TICKS(700));
    ssd1306_fill(&oled, false);  ssd1306_send_data(&oled);

    while (true) {
        const char *modo = (estadoAtual == MODO_NORMAL) ? "MODO: NORMAL " : "MODO: NOTURNO";
        const char *cor, *msg;
        switch (luzAtual) {
            case LUZ_VERDE:    cor = "VERDE    ";  msg = "ATRAVESSE !"; break;
            case LUZ_AMARELO:  cor = "AMARELO  ";  msg = "ATENCAO !  "; break;
            case LUZ_VERMELHO: cor = "VERMELHO ";  msg = "AGUARDE !  "; break;
            default:           cor = "---      ";  msg = "           "; break;
        }
        ssd1306_fill(&oled, false);
        ssd1306_rect(&oled, 0, 0, OLED_LARGURA, 10, false, true);  // Desenha borda superior
        ssd1306_draw_string(&oled, "SEMAFORO", 36, 1, false);
        ssd1306_draw_string(&oled, modo, 0, 14, false);
        ssd1306_draw_string(&oled, "COR :", 0, 30, false);
        ssd1306_draw_string(&oled, cor, 42, 30, false);
        ssd1306_rect(&oled, 48, 0, OLED_LARGURA, 16, false, true);
        ssd1306_draw_string(&oled, msg, 20, 50, false);
        ssd1306_send_data(&oled);  // Atualiza display
        vTaskDelay(pdMS_TO_TICKS(250));  // Atualiza a cada 250ms
    }
}

/* Tarefa: Buzzer */
void tarefaBuzzer(void *pvParam) {  // GERA SONS BASEADOS NO ESTADO DA LUZ
    uint slice_buzzer;
    uint16_t wrap_buzzer;

    gpio_set_function(PINO_BUZZER, GPIO_FUNC_PWM);
    slice_buzzer = pwm_gpio_to_slice_num(PINO_BUZZER);
    const float clkdiv = 4.0f;
    wrap_buzzer = (uint16_t)((125000000 / (clkdiv * FREQUENCIA_BUZZER)) - 1);  // Configura PWM para 2000Hz

    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, clkdiv);
    pwm_config_set_wrap(&cfg, wrap_buzzer);
    pwm_init(slice_buzzer, &cfg, true);  // Inicia PWM

    #define BUZZER_LIGAR()     pwm_set_gpio_level(PINO_BUZZER, wrap_buzzer / 2)  // Duty cycle de 50%
    #define BUZZER_DESLIGAR()  pwm_set_gpio_level(PINO_BUZZER, 0)

    BUZZER_DESLIGAR();  // Garante buzzer desligado no início

    while (true) {
        uint32_t on_ms = 0, off_ms = 200;
        if (estadoAtual == MODO_NORMAL) {
            switch (luzAtual) {
                case LUZ_VERDE:    on_ms = 1000; off_ms = 900;  break;
                case LUZ_AMARELO:  on_ms = 100; off_ms = 100;  break;  // Beep rápido para atenção
                case LUZ_VERMELHO: on_ms = 500; off_ms = 1500; break;
                default:           on_ms = 0;   off_ms = 500;  break;
            }
        } else {
            on_ms = 100; off_ms = 1900;  // Beep esparso no modo noturno
        }
        if (on_ms > 0) {
            BUZZER_LIGAR();
            vTaskDelay(pdMS_TO_TICKS(on_ms));
        }
        BUZZER_DESLIGAR();
        vTaskDelay(pdMS_TO_TICKS(off_ms));
    }
}

/* Tarefa: Matriz 5 × 5 (animações) */
void tarefaMatriz(void *pvParam) {  //EXIBE ANIMAÇÕES NA MATRIZ DE LEDs
    inicializar_matriz_led();  //Inicializa WS2812
    uint8_t shift = 0;

    while (true) {
        switch (luzAtual) {
            case LUZ_VERDE:
                uint8_t pad[5];
                for (int i = 0; i < 5; ++i)
                    pad[i] = (PAD_OK[i] << shift) | (PAD_OK[i] >> (5 - shift));  //Desloca padrão "✓"
                matriz_draw_pattern(pad, COR_VERDE);  //Exibe animação
                shift = (shift + 1) % 5;
                vTaskDelay(pdMS_TO_TICKS(200));  //Atualiza a cada 200ms para animação suave
                break;
            case LUZ_AMARELO:
                matriz_draw_pattern(PAD_EXC, COR_AMARELO);  //Exibe "!"
                vTaskDelay(pdMS_TO_TICKS(250));  //Liga por 150ms para piscar rápido
                matriz_clear();
                vTaskDelay(pdMS_TO_TICKS(250));  //Desliga por 150ms para piscar rápido
                break;
            case LUZ_VERMELHO:
                matriz_draw_pattern(PAD_X, COR_VERMELHO);
                vTaskDelay(pdMS_TO_TICKS(400));  //Liga por 400ms para piscar lento
                matriz_clear();
                vTaskDelay(pdMS_TO_TICKS(400));  //Desliga por 400ms para piscar lento
                break;
            default:
                matriz_clear();  //Limpa matriz em estado indefinido
                vTaskDelay(pdMS_TO_TICKS(200));  //Pausa de 200ms em estado indefinido
                break;
        }
    }
}

/* Interrupção: Botão B (UF2) */
void gpio_irq_handler_botao_b(uint gpio, uint32_t events) {
    reset_usb_boot(0, 0);  // Reinicia para modo UF2
}

/* Função principal */
int main(void) {
    stdio_init_all();  // Inicializa comunicação serial
    gpio_init(PINO_BOTAO_B);
    gpio_set_dir(PINO_BOTAO_B, GPIO_IN);
    gpio_pull_up(PINO_BOTAO_B);
    gpio_set_irq_enabled_with_callback(PINO_BOTAO_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler_botao_b);  // Habilita interrupção para UF2

    xTaskCreate(tarefaSemaforoNormal,  "SemNormal", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    xTaskCreate(tarefaSemaforoNoturno, "SemNoturno", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    xTaskCreate(tarefaBotaoA,          "BotaoA", configMINIMAL_STACK_SIZE, NULL, 3, NULL);  // Prioridade maior para resposta rápida
    xTaskCreate(tarefaDisplay,         "Display", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(tarefaBuzzer,          "Buzzer", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(tarefaMatriz,          "Matriz", configMINIMAL_STACK_SIZE + 64, NULL, 1, NULL);  // Maior stack para matriz

    vTaskStartScheduler();  // Inicia FreeRTOS
    panic_unsupported();
}