#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

#include "lib/Display_Bibliotecas/font.h"
#include "lib/Display_Bibliotecas/ssd1306.h"

// Definição dos pinos dos LEDs
#define PINO_LED_VERDE     11
#define PINO_LED_AZUL      12     // (opcional para debug)
#define PINO_LED_VERMELHO  13

// Definição dos pinos dos botões
#define PINO_BOTAO_A 5
#define PINO_BOTAO_B 6            // BOOTSEL

// Definição do pino e frequência do buzzer
#define PINO_BUZZER     10
#define FREQUENCIA_BUZZER 2000     // Hz

// Enumeração para os estados do semáforo
typedef enum { MODO_NORMAL = 0, MODO_NOTURNO } EstadoSemaforo;
volatile EstadoSemaforo estadoAtual = MODO_NORMAL;

// Enumeração para os estados das luzes do semáforo
typedef enum { LUZ_VERDE, LUZ_AMARELO, LUZ_VERMELHO, LUZ_DESLIGADO } EstadoLuz;
volatile EstadoLuz luzAtual = LUZ_VERDE;

// Constantes de temporização em milissegundos
#define TEMPO_VERDE    5000
#define TEMPO_AMARELO  2000
#define TEMPO_VERMELHO 5000
#define PISCA_ON_MS    2000     // Tempo que o amarelo fica ligado no modo noturno
#define PISCA_OFF_MS   2000     // Tempo que o amarelo fica desligado no modo noturno
#define INTERVALO_MS   50       // Intervalo de delay nas loops

// Tarefa para controlar o semáforo no modo normal
void tarefaSemaforoNormal(void *pvParam) {
    // Inicialização dos pinos dos LEDs
    gpio_init(PINO_LED_VERDE);    gpio_set_dir(PINO_LED_VERDE, GPIO_OUT);
    gpio_init(PINO_LED_AZUL);     gpio_set_dir(PINO_LED_AZUL,  GPIO_OUT);
    gpio_init(PINO_LED_VERMELHO); gpio_set_dir(PINO_LED_VERMELHO, GPIO_OUT);

    while (true) {
        // Verifica se o modo atual é normal
        if (estadoAtual != MODO_NORMAL) {
            vTaskDelay(pdMS_TO_TICKS(INTERVALO_MS));
            continue;
        }

        // Luz verde
        gpio_put(PINO_LED_VERDE, 1);  gpio_put(PINO_LED_VERMELHO, 0);
        luzAtual = LUZ_VERDE;
        vTaskDelay(pdMS_TO_TICKS(TEMPO_VERDE));
        if (estadoAtual != MODO_NORMAL) continue;

        // Luz amarela (verde + vermelho)
        gpio_put(PINO_LED_VERDE, 1);  gpio_put(PINO_LED_VERMELHO, 1);
        luzAtual = LUZ_AMARELO;
        vTaskDelay(pdMS_TO_TICKS(TEMPO_AMARELO));
        if (estadoAtual != MODO_NORMAL) continue;

        // Luz vermelha
        gpio_put(PINO_LED_VERDE, 0);  gpio_put(PINO_LED_VERMELHO, 1);
        luzAtual = LUZ_VERMELHO;
        vTaskDelay(pdMS_TO_TICKS(TEMPO_VERMELHO));
    }
}

// Tarefa para controlar o semáforo no modo noturno
void tarefaSemaforoNoturno(void *pvParam) {
    while (true) {
        // Verifica se o modo atual é noturno
        if (estadoAtual != MODO_NOTURNO) {
            vTaskDelay(pdMS_TO_TICKS(INTERVALO_MS));
            continue;
        }

        // Luz amarela ligada
        gpio_put(PINO_LED_VERDE, 1);  gpio_put(PINO_LED_VERMELHO, 1);
        luzAtual = LUZ_AMARELO;
        vTaskDelay(pdMS_TO_TICKS(PISCA_ON_MS));
        if (estadoAtual != MODO_NOTURNO) continue;

        // Luz amarela desligada
        gpio_put(PINO_LED_VERDE, 0);  gpio_put(PINO_LED_VERMELHO, 0);
        luzAtual = LUZ_DESLIGADO;
        vTaskDelay(pdMS_TO_TICKS(PISCA_OFF_MS));
    }
}

// Tarefa para controlar o botão A (troca de modos)
void tarefaBotaoA(void *pvParam) {
    const uint32_t debounceMs = 50;
    bool pressionado = false;
    int64_t ultimoTempo = 0;

    // Inicialização do pino do botão A
    gpio_init(PINO_BOTAO_A);  gpio_set_dir(PINO_BOTAO_A, GPIO_IN);
    gpio_pull_up(PINO_BOTAO_A);

    while (true) {
        bool nivel = gpio_get(PINO_BOTAO_A);   // 1=solto / 0=pressionado

        // Detecta borda de descida (botão pressionado)
        if (!nivel && !pressionado) {
            int64_t agora = to_ms_since_boot(get_absolute_time());
            if (agora - ultimoTempo > debounceMs) {
                pressionado = true;
                ultimoTempo = agora;
                // Alterna entre os modos normal e noturno
                estadoAtual = (estadoAtual == MODO_NORMAL) ? MODO_NOTURNO : MODO_NORMAL;
                printf("MODO alterado → %s\n", estadoAtual == MODO_NORMAL ? "NORMAL" : "NOTURNO");
            }
        } else if (nivel) {
            pressionado = false;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Tarefa para controlar o display OLED
void tarefaDisplay(void *pvParam) {
    // Definição dos pinos I²C do BitDog Lab (I2C1)
    #define I2C_OLED   i2c1
    #define PINO_SDA   14
    #define PINO_SCL   15
    #define ENDERECO   0x3C
    #define LARGURA    128
    #define ALTURA     64

    // Inicialização do I²C
    i2c_init(I2C_OLED, 400 * 1000);
    gpio_set_function(PINO_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PINO_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PINO_SDA); gpio_pull_up(PINO_SCL);

    // Inicialização do display OLED
    ssd1306_t oled;
    ssd1306_init(&oled, LARGURA, ALTURA, false, ENDERECO, I2C_OLED);
    ssd1306_config(&oled);

    // Splash rápido no display
    ssd1306_fill(&oled, true);  ssd1306_send_data(&oled);
    vTaskDelay(pdMS_TO_TICKS(700));
    ssd1306_fill(&oled, false); ssd1306_send_data(&oled);

    while (true) {
        // Define o texto do modo atual
        const char *modo = (estadoAtual == MODO_NORMAL) ? "MODO: NORMAL " : "MODO: NOTURNO";
        const char *cor, *msg;

        // Define o texto da cor e mensagem atual
        switch (luzAtual) {
            case LUZ_VERDE:    cor = "VERDE    ";  msg = "ATRAVESSE !"; break;
            case LUZ_AMARELO:  cor = "AMARELO  ";  msg = "ATENCAO !  "; break;
            case LUZ_VERMELHO: cor = "VERMELHO ";  msg = "AGUARDE !  "; break;
            default:           cor = "---      ";  msg = "           "; break;
        }

        // Atualiza o display com os novos valores
        ssd1306_fill(&oled, false);
        ssd1306_rect(&oled, 0, 0, LARGURA, 10, false, true);
        ssd1306_draw_string(&oled, "SEMAFORO", 36, 1, false);
        ssd1306_draw_string(&oled, modo,     0, 14, false);
        ssd1306_draw_string(&oled, "COR :",   0, 30, false);
        ssd1306_draw_string(&oled, cor,     42, 30, false);
        ssd1306_rect(&oled, 48, 0, LARGURA, 16, false, true);
        ssd1306_draw_string(&oled, msg,     20, 50, false);
        ssd1306_send_data(&oled);

        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

// Tarefa para controlar o buzzer
void tarefaBuzzer(void *pvParam) {
    static uint slice_buzzer;
    static uint16_t wrap_buzzer;

    // Configuração do pino do buzzer para PWM
    gpio_set_function(PINO_BUZZER, GPIO_FUNC_PWM);
    slice_buzzer = pwm_gpio_to_slice_num(PINO_BUZZER);

    // Cálculo do TOP e divisor para a frequência desejada
    const float clkdiv = 4.0f;
    wrap_buzzer = (uint16_t)( (125000000 / (clkdiv * FREQUENCIA_BUZZER)) - 1 );

    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, clkdiv);
    pwm_config_set_wrap(&cfg, wrap_buzzer);
    pwm_init(slice_buzzer, &cfg, true);

    // Macros para ligar e desligar o buzzer
    #define BUZZER_LIGAR()   pwm_set_gpio_level(PINO_BUZZER, wrap_buzzer / 2)
    #define BUZZER_DESLIGAR()  pwm_set_gpio_level(PINO_BUZZER, 0)

    BUZZER_DESLIGAR();

    while (true) {
        uint32_t on_ms  = 0;
        uint32_t off_ms = 200;

        // Define os tempos de ligar e desligar o buzzer com base no modo e na luz atual
        if (estadoAtual == MODO_NORMAL) {
            switch (luzAtual) {
                case LUZ_VERDE:      on_ms = 100; off_ms = 900;  break;  // 1 Hz
                case LUZ_AMARELO:    on_ms = 100; off_ms = 100;  break;  // rápido
                case LUZ_VERMELHO:   on_ms = 500; off_ms = 1500; break;  // 0.5 s / 1.5 s
                default:             on_ms = 0;   off_ms = 500;  break;
            }
        } else {
            on_ms  = 100;          // beep curto
            off_ms = 1900;         // intervalo total ≈2 s
        }

        // Liga e desliga o buzzer conforme os tempos definidos
        if (on_ms > 0) {
            BUZZER_LIGAR();
            vTaskDelay(pdMS_TO_TICKS(on_ms));
        }
        BUZZER_DESLIGAR();
        vTaskDelay(pdMS_TO_TICKS(off_ms));
    }
}

// Interrupção para o botão B (reinicia no bootloader UF2)
void gpio_irq_handler_botao_b(uint gpio, uint32_t events) {
    reset_usb_boot(0, 0);
}

// Função principal
int main(void) {
    stdio_init_all();

    // Configuração do botão B para reiniciar no modo UF2
    gpio_init(PINO_BOTAO_B);
    gpio_set_dir(PINO_BOTAO_B, GPIO_IN);
    gpio_pull_up(PINO_BOTAO_B);
    gpio_set_irq_enabled_with_callback(PINO_BOTAO_B,
                                       GPIO_IRQ_EDGE_FALL, true,
                                       &gpio_irq_handler_botao_b);

    puts("Sistema de semáforo iniciado em MODO NORMAL");

    // Criação das tarefas FreeRTOS
    xTaskCreate(tarefaSemaforoNormal,  "SemNormal", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(tarefaSemaforoNoturno, "SemNoturno", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(tarefaBotaoA,          "BotaoA", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(tarefaDisplay,         "Display", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(tarefaBuzzer,          "Buzzer", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);

    // Inicia o agendador FreeRTOS
    vTaskStartScheduler();
    while (true) { }
}
