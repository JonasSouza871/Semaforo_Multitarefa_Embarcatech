/******************************************************************************************
 *  SEMÁFORO INTELIGENTE c/ DISPLAY SSD1306 – FreeRTOS (RP2040 – BitDog Lab)
 *  • 2 modos: Normal (verde→amarelo→vermelho) e Noturno (amarelo piscando)
 *  • Feedback sonoro via buzzer PWM 2 kHz
 *  • 5 tarefas FreeRTOS (LED Normal, LED Noturno, Botão A, Display, Buzzer)
 *  • Botão B (BOOTSEL) reinicia em modo UF2
 ******************************************************************************************/

 #include "pico/bootrom.h"
 #include "pico/stdlib.h"
 #include "hardware/i2c.h"
 #include "hardware/pwm.h"          // <-- PWM!
 #include "FreeRTOS.h"
 #include "task.h"
 #include <stdio.h>
 
 #include "lib/Display_Bibliotecas/font.h"
 #include "lib/Display_Bibliotecas/ssd1306.h"
 
 /* -------------------------------------------------------------------------- */
 /*                                DEFINIÇÕES                                  */
 /* -------------------------------------------------------------------------- */
 // LEDs
 #define PINO_LED_VERDE     11
 #define PINO_LED_AZUL      12     // (opcional p/ debug)
 #define PINO_LED_VERMELHO  13
 
 // Botões
 #define PINO_BOTAO_A 5
 #define PINO_BOTAO_B 6            // BOOTSEL
 
 // Buzzer (passivo ou ativo) – PWM em ~2 kHz
 #define PINO_BUZZER     10
 #define BUZZER_FREQUENCY 2000     // Hz
 
 /* -------------------------------------------------------------------------- */
 /*                     ESTADOS COMPARTILHADOS (volatile)                      */
 /* -------------------------------------------------------------------------- */
 typedef enum { MODO_NORMAL = 0, MODO_NOTURNO } EstadoSemaforo;
 volatile EstadoSemaforo estadoAtual = MODO_NORMAL;
 
 typedef enum { LUZ_VERDE, LUZ_AMARELO, LUZ_VERMELHO, LUZ_DESLIGADO } EstadoLuz;
 volatile EstadoLuz luzAtual = LUZ_VERDE;
 
 /* -------------------------------------------------------------------------- */
 /*                        CONSTANTES DE TEMPORIZAÇÃO (ms)                     */
 /* -------------------------------------------------------------------------- */
 #define TEMPO_VERDE    5000
 #define TEMPO_AMARELO  2000
 #define TEMPO_VERMELHO 5000
 #define PISCA_ON_MS    2000     // amarelo ligado (noturno)
 #define PISCA_OFF_MS   2000     // amarelo apagado (noturno)
 #define INTERVALO_MS   50       // passo das loops com delay
 
 /* -------------------------------------------------------------------------- */
 /*                    BUZZER – inicialização e macros ON/OFF                  */
 /* -------------------------------------------------------------------------- */
 static uint  slice_buzzer;
 static uint16_t wrap_buzzer;      // valor TOP de PWM (16 bits máx.)
 
 static void buzzer_pwm_init(void)
 {
     gpio_set_function(PINO_BUZZER, GPIO_FUNC_PWM);
     slice_buzzer = pwm_gpio_to_slice_num(PINO_BUZZER);
 
     // Calcula TOP e divisor para a frequência desejada
     // f_pwm = f_sys / (clkdiv * (TOP + 1))
     const float clkdiv = 4.0f;         // escolhemos divisor 4
     wrap_buzzer = (uint16_t)( (125000000 / (clkdiv * BUZZER_FREQUENCY)) - 1 );
 
     pwm_config cfg = pwm_get_default_config();
     pwm_config_set_clkdiv(&cfg, clkdiv);
     pwm_config_set_wrap(&cfg, wrap_buzzer);
     pwm_init(slice_buzzer, &cfg, true);    // inicia já habilitado
 }
 
 #define BUZZER_ON()   pwm_set_gpio_level(PINO_BUZZER, wrap_buzzer / 2) // 50 % duty
 #define BUZZER_OFF()  pwm_set_gpio_level(PINO_BUZZER, 0)
 
 /* -------------------------------------------------------------------------- */
 /*                     TAREFA 1 – SEMÁFORO MODO NORMAL                        */
 /* -------------------------------------------------------------------------- */
 void tarefaSemaforoNormal(void *pvParam)
 {
     gpio_init(PINO_LED_VERDE);    gpio_set_dir(PINO_LED_VERDE, GPIO_OUT);
     gpio_init(PINO_LED_AZUL);     gpio_set_dir(PINO_LED_AZUL,  GPIO_OUT);
     gpio_init(PINO_LED_VERMELHO); gpio_set_dir(PINO_LED_VERMELHO, GPIO_OUT);
 
     while (true) {
         if (estadoAtual != MODO_NORMAL) {
             vTaskDelay(pdMS_TO_TICKS(INTERVALO_MS));
             continue;
         }
 
         /* Verde */
         gpio_put(PINO_LED_VERDE, 1);  gpio_put(PINO_LED_VERMELHO, 0);
         luzAtual = LUZ_VERDE;
         for (uint32_t t = 0; t < TEMPO_VERDE && estadoAtual == MODO_NORMAL; t += INTERVALO_MS)
             vTaskDelay(pdMS_TO_TICKS(INTERVALO_MS));
         if (estadoAtual != MODO_NORMAL) continue;
 
         /* Amarelo (verde+vermelho) */
         gpio_put(PINO_LED_VERDE, 1);  gpio_put(PINO_LED_VERMELHO, 1);
         luzAtual = LUZ_AMARELO;
         for (uint32_t t = 0; t < TEMPO_AMARELO && estadoAtual == MODO_NORMAL; t += INTERVALO_MS)
             vTaskDelay(pdMS_TO_TICKS(INTERVALO_MS));
         if (estadoAtual != MODO_NORMAL) continue;
 
         /* Vermelho */
         gpio_put(PINO_LED_VERDE, 0);  gpio_put(PINO_LED_VERMELHO, 1);
         luzAtual = LUZ_VERMELHO;
         for (uint32_t t = 0; t < TEMPO_VERMELHO && estadoAtual == MODO_NORMAL; t += INTERVALO_MS)
             vTaskDelay(pdMS_TO_TICKS(INTERVALO_MS));
     }
 }
 
 /* -------------------------------------------------------------------------- */
 /*                   TAREFA 2 – SEMÁFORO MODO NOTURNO                         */
 /* -------------------------------------------------------------------------- */
 void tarefaSemaforoNoturno(void *pvParam)
 {
     while (true) {
         if (estadoAtual != MODO_NOTURNO) {
             vTaskDelay(pdMS_TO_TICKS(INTERVALO_MS));
             continue;
         }
 
         /* Amarelo ligado */
         gpio_put(PINO_LED_VERDE, 1);  gpio_put(PINO_LED_VERMELHO, 1);
         luzAtual = LUZ_AMARELO;
         for (uint32_t t = 0; t < PISCA_ON_MS && estadoAtual == MODO_NOTURNO; t += INTERVALO_MS)
             vTaskDelay(pdMS_TO_TICKS(INTERVALO_MS));
         if (estadoAtual != MODO_NOTURNO) continue;
 
         /* Amarelo desligado */
         gpio_put(PINO_LED_VERDE, 0);  gpio_put(PINO_LED_VERMELHO, 0);
         luzAtual = LUZ_DESLIGADO;
         for (uint32_t t = 0; t < PISCA_OFF_MS && estadoAtual == MODO_NOTURNO; t += INTERVALO_MS)
             vTaskDelay(pdMS_TO_TICKS(INTERVALO_MS));
     }
 }
 
 /* -------------------------------------------------------------------------- */
 /*                 TAREFA 3 – BOTÃO A (troca de modos)                        */
 /* -------------------------------------------------------------------------- */
 void tarefaBotaoA(void *pvParam)
 {
     const uint32_t debounceMs = 50;
     bool pressionado = false;
     int64_t ultimoTempo = 0;
 
     gpio_init(PINO_BOTAO_A);  gpio_set_dir(PINO_BOTAO_A, GPIO_IN);
     gpio_pull_up(PINO_BOTAO_A);
 
     while (true) {
         bool nivel = gpio_get(PINO_BOTAO_A);   // 1=solto / 0=press.
 
         if (!nivel && !pressionado) {          // borda de descida
             int64_t agora = to_ms_since_boot(get_absolute_time());
             if (agora - ultimoTempo > debounceMs) {
                 pressionado = true;
                 ultimoTempo = agora;
                 estadoAtual = (estadoAtual == MODO_NORMAL) ? MODO_NOTURNO : MODO_NORMAL;
                 printf("MODO alterado → %s\n",
                        estadoAtual == MODO_NORMAL ? "NORMAL" : "NOTURNO");
             }
         } else if (nivel) {
             pressionado = false;
         }
         vTaskDelay(pdMS_TO_TICKS(10));
     }
 }
 
 /* -------------------------------------------------------------------------- */
 /*              TAREFA 4 – DISPLAY OLED (modo + cor + mensagem)               */
 /* -------------------------------------------------------------------------- */
 void tarefaDisplay(void *pvParam)
 {
     /* Pines I²C do BitDog Lab (I2C1) */
     #define I2C_OLED   i2c1
     #define PINO_SDA   14
     #define PINO_SCL   15
     #define ENDERECO   0x3C
     #define W          128
     #define H          64
 
     /* I²C init */
     i2c_init(I2C_OLED, 400 * 1000);
     gpio_set_function(PINO_SDA, GPIO_FUNC_I2C);
     gpio_set_function(PINO_SCL, GPIO_FUNC_I2C);
     gpio_pull_up(PINO_SDA); gpio_pull_up(PINO_SCL);
 
     /* Display init */
     ssd1306_t oled;
     ssd1306_init(&oled, W, H, false, ENDERECO, I2C_OLED);
     ssd1306_config(&oled);
 
     /* Splash rápido */
     ssd1306_fill(&oled, true);  ssd1306_send_data(&oled);
     vTaskDelay(pdMS_TO_TICKS(700));
     ssd1306_fill(&oled, false); ssd1306_send_data(&oled);
 
     /* Loop */
     while (true) {
         const char *modo = (estadoAtual == MODO_NORMAL) ? "MODO: NORMAL "
                                                         : "MODO: NOTURNO";
         const char *cor, *msg;
         switch (luzAtual) {
             case LUZ_VERDE:    cor = "VERDE    ";  msg = "ATRAVESSE !"; break;
             case LUZ_AMARELO:  cor = "AMARELO  ";  msg = "ATENCAO !  "; break;
             case LUZ_VERMELHO: cor = "VERMELHO ";  msg = "AGUARDE !  "; break;
             default:           cor = "---      ";  msg = "           "; break;
         }
 
         ssd1306_fill(&oled, false);
         ssd1306_rect(&oled, 0, 0, W, 10, false, true);
         ssd1306_draw_string(&oled, "SEMAFORO", 36, 1, false);
         ssd1306_draw_string(&oled, modo,     0, 14, false);
         ssd1306_draw_string(&oled, "COR :",   0, 30, false);
         ssd1306_draw_string(&oled, cor,     42, 30, false);
         ssd1306_rect(&oled, 48, 0, W, 16, false, true);
         ssd1306_draw_string(&oled, msg,     20, 50, false);
         ssd1306_send_data(&oled);
 
         vTaskDelay(pdMS_TO_TICKS(250));
     }
 }
 
 /* -------------------------------------------------------------------------- */
 /*               TAREFA 5 – BUZZER  (padrões sonoros via PWM)                 */
 /* -------------------------------------------------------------------------- */
 void tarefaBuzzer(void *pvParam)
 {
     // PWM já foi inicializado em main()
     BUZZER_OFF();
 
     while (true) {
         uint32_t on_ms  = 0;
         uint32_t off_ms = 200;   // valor mínimo
 
         if (estadoAtual == MODO_NORMAL) {
             switch (luzAtual) {
                 case LUZ_VERDE:      on_ms = 100; off_ms = 900;  break;  // 1 Hz
                 case LUZ_AMARELO:    on_ms = 100; off_ms = 100;  break;  // rápido
                 case LUZ_VERMELHO:   on_ms = 500; off_ms = 1500; break;  // 0.5 s / 1.5 s
                 default:             on_ms = 0;   off_ms = 500;  break;
             }
         } else { /* ---------- MODO NOTURNO ---------- */
             on_ms  = 100;          // beep curto
             off_ms = 1900;         // intervalo total ≈2 s
         }
 
         if (on_ms > 0) {
             BUZZER_ON();
             vTaskDelay(pdMS_TO_TICKS(on_ms));
         }
         BUZZER_OFF();
         vTaskDelay(pdMS_TO_TICKS(off_ms));
     }
 }
 
 /* -------------------------------------------------------------------------- */
 /*             ISR do Botão B – reinicia no bootloader UF2                    */
 /* -------------------------------------------------------------------------- */
 void gpio_irq_handler_botao_b(uint gpio, uint32_t events)
 {
     reset_usb_boot(0, 0);
 }
 
 /* -------------------------------------------------------------------------- */
 /*                              FUNÇÃO  MAIN                                  */
 /* -------------------------------------------------------------------------- */
 int main(void)
 {
     stdio_init_all();
 
     /* Inicializa PWM do buzzer antes de criar as tarefas */
     buzzer_pwm_init();
 
     /* Botão B → UF2 */
     gpio_init(PINO_BOTAO_B);
     gpio_set_dir(PINO_BOTAO_B, GPIO_IN);
     gpio_pull_up(PINO_BOTAO_B);
     gpio_set_irq_enabled_with_callback(PINO_BOTAO_B,
                                        GPIO_IRQ_EDGE_FALL, true,
                                        &gpio_irq_handler_botao_b);
 
     puts("Sistema de semáforo iniciado em MODO NORMAL");
 
     /* Criação das tarefas -------------------------------------------------- */
     xTaskCreate(tarefaSemaforoNormal,  "SemNormal",
                 configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
 
     xTaskCreate(tarefaSemaforoNoturno, "SemNoturno",
                 configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
 
     xTaskCreate(tarefaBotaoA,          "BotaoA",
                 configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
 
     xTaskCreate(tarefaDisplay,         "Display",
                 configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
 
     xTaskCreate(tarefaBuzzer,          "Buzzer",
                 configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
 
     /* Inicia o agendador (não retorna) */
     vTaskStartScheduler();
     while (true) { /* nunca deve chegar aqui */ }
 }
 