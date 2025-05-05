
# <Nome do Projeto> 🚀

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Raspberry Pi Pico](https://img.shields.io/badge/Platform-Raspberry%20Pi%20Pico-purple.svg)](https://www.raspberrypi.com/products/raspberry-pi-pico/)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)]()



## 📘 Descrição Breve

Este repositório contém o firmware para `<Nome do Projeto>`, um projeto embarcado desenvolvido para a placa Raspberry Pi Pico.

**Objetivo Principal:** <Descreva de forma concisa o que o projeto faz. Exemplo: "Controlar um semáforo inteligente com modos diurno/noturno e feedback visual/sonoro.">

**Utilidade:** <Explique para que serve o projeto ou qual problema ele resolve. Exemplo: "Serve como uma demonstração didática do uso de FreeRTOS, PIO para WS2812 e comunicação I2C com display OLED no Pico.">

---

## ✨ Funcionalidades Principais

*   **<Funcionalidade 1>:** <Descrição técnica. Ex: "Controle de LEDs RGB (WS2812) via PIO para exibir padrões visuais.">
*   **<Funcionalidade 2>:** <Descrição técnica. Ex: "Interface com display OLED (SSD1306) via I2C para exibir status e informações.">
*   **<Funcionalidade 3>:** <Descrição técnica. Ex: "Gerenciamento de tarefas concorrentes utilizando FreeRTOS (se aplicável).">
*   **<Funcionalidade 4>:** <Descrição técnica. Ex: "Leitura de entradas digitais (botões) com debounce para interação do usuário.">
*   **<Funcionalidade 5>:** <Descrição técnica. Ex: "Geração de sinais sonoros (buzzer) via PWM.">
*   **<Funcionalidade 6>:** <Descrição técnica. Ex: "Comunicação Serial via USB para debugging e monitoramento.">
*   **(Opcional) <Outra Funcionalidade>:** <Descrição...>

---

## ⚙️ Hardware Necessário

*   **Microcontrolador:** 1x Raspberry Pi Pico ou Pico W
*   **<Componente Principal 1>:** <Quantidade>x <Nome do Componente. Ex: "Display OLED SSD1306 128x64 I2C">
*   **<Componente Principal 2>:** <Quantidade>x <Nome do Componente. Ex: "Matriz de LED WS2812 5x5">
*   **<Componente Sensor/Atuador 1>:** <Quantidade>x <Nome do Componente. Ex: "LED Difuso 5mm (Verde, Amarelo, Vermelho)">
*   **<Componente Sensor/Atuador 2>:** <Quantidade>x <Nome do Componente. Ex: "Botão Tátil (Push Button)">
*   **<Componente Sensor/Atuador 3>:** <Quantidade>x <Nome do Componente. Ex: "Buzzer Ativo/Passivo 5V">
*   **Resistores:** <Valores e Quantidades. Ex: "3x 220Ω para LEDs, 1x 10kΩ para Pull-up (se necessário)">
*   **Protoboard e Jumpers:** Para montagem do circuito.
*   **Fonte de Alimentação:** Cabo Micro USB (para o Pico) e/ou fonte externa <Tensão/Corrente> (se necessário para periféricos).

---

## 🔌 Esquema de Conexões

Conecte os componentes ao Raspberry Pi Pico conforme descrito abaixo. Certifique-se de que as conexões de alimentação (VBUS/VSYS, 3V3, GND) estejam corretas.

**Referência de Pinagem do Pico:** [Link para Pinagem Oficial do Pico](https://datasheets.raspberrypi.com/pico/Pico-R3-A4-Pinout.pdf)

### <Nome do Componente 1: Ex: Display OLED SSD1306>

| Pino Pico | Função       | Pino Display |
| :-------- | :----------- | :----------- |
| GP<Número>  | I2C1 SDA     | SDA          |
| GP<Número>  | I2C1 SCL     | SCL          |
| 3V3 (OUT) | Alimentação  | VCC          |
| GND       | Terra        | GND          |

*(Observação: Verifique o endereço I2C do seu display. O padrão comum é `0x3C`)*

### <Nome do Componente 2: Ex: Matriz LED WS2812>

| Pino Pico | Função       | Pino Matriz |
| :-------- | :----------- | :---------- |
| GP<Número>  | PIO Data Out | DIN         |
| VSYS/VBUS | Alimentação +| 5V / VCC    |
| GND       | Terra        | GND         |

*(**Importante:** Para matrizes WS2812, considere usar uma fonte externa se o consumo for alto e conecte o GND da fonte externa ao GND do Pico.)*

### <Nome do Componente 3: Ex: LEDs Individuais>

| Pino Pico | Função         | Conexão                                    |
| :-------- | :------------- | :----------------------------------------- |
| GP<Número>  | LED <Cor 1>    | Anodo (+) do LED <Cor 1> (via Resistor <Valor>Ω) |
| GP<Número>  | LED <Cor 2>    | Anodo (+) do LED <Cor 2> (via Resistor <Valor>Ω) |
| ...       | ...            | ...                                        |
| GND       | Terra Comum    | Catodo (-) de todos os LEDs                 |

### <Nome do Componente 4: Ex: Botões>

| Pino Pico | Função         | Conexão                                |
| :-------- | :------------- | :------------------------------------- |
| GP<Número>  | Botão <Nome 1> | Um terminal do botão                   |
| GND       | Terra          | Outro terminal do botão (com Pull-up interno ativado no código ou resistor externo para 3V3) |
| ...       | ...            | ...                                    |

### <Nome do Componente 5: Ex: Buzzer>

| Pino Pico | Função      | Pino Buzzer |
| :-------- | :---------- | :---------- |
| GP<Número>  | PWM Out     | Positivo (+) |
| GND       | Terra       | Negativo (-) |

*(Adapte as seções acima conforme os componentes do seu projeto)*

---

## 💻 Configuração do Ambiente

Para compilar e carregar o firmware no Raspberry Pi Pico, você precisará configurar o ambiente de desenvolvimento em seu computador.

1.  **Instalar as Ferramentas Essenciais:**
    *   **Pico SDK:** Siga o guia oficial ["Getting Started with Raspberry Pi Pico"](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf) para o seu sistema operacional (Linux, macOS ou Windows). Isso inclui:
        *   `CMake`
        *   `ARM GCC Compiler Toolchain` (`gcc-arm-none-eabi`)
        *   `Pico SDK` e `Pico Examples` (recomendado)
    *   **(Opcional, Recomendado) Visual Studio Code:** Instale o VS Code com as extensões:
        *   `C/C++` (Microsoft)
        *   `CMake Tools` (Microsoft)
        *   `Cortex-Debug` (marus25) - Para debugging via SWD (requer probe)
        *   **(Se usar VS Code) Raspberry Pi Pico/RP2040:** (Proprietário da Raspberry Pi) - Facilita a configuração e compilação.

2.  **Clonar o Repositório:**
    ```bash
    git clone <URL do seu repositório Git>
    cd <Nome do Diretório do Projeto>
    ```

3.  **Obter Submódulos (se houver):** Se o projeto usar submódulos Git (como bibliotecas externas):
    ```bash
    git submodule update --init --recursive
    ```

4.  **Configurar o Caminho do Pico SDK:** Certifique-se de que a variável de ambiente `PICO_SDK_PATH` esteja definida corretamente ou que o SDK esteja em um local padrão (`~/pico-sdk` ou conforme configurado no CMake).

---

## ▶️ Compilação e Upload

Siga estes passos para compilar o projeto e carregar o firmware no Pico:

1.  **Criar e Acessar o Diretório de Build:**
    ```bash
    mkdir build
    cd build
    ```

2.  **Configurar com CMake:** (Execute a partir do diretório `build`)
    ```bash
    # Certifique-se que PICO_SDK_PATH está definido ou use -DPICO_SDK_PATH=/caminho/para/seu/sdk
    cmake ..
    ```
    *Se estiver usando FreeRTOS ou outras bibliotecas fora do SDK, pode ser necessário adicionar caminhos no `CMakeLists.txt` ou via flags `-D` no comando `cmake` (verifique o `CMakeLists.txt` do projeto).*

3.  **Compilar o Projeto:**
    ```bash
    make -j$(nproc) # Ou simplesmente 'make'
    ```
    Isso gerará vários arquivos dentro do diretório `build`, incluindo `<nome_do_executavel>.uf2`.

4.  **Carregar o Firmware (.uf2) no Pico:**
    *   Desconecte o Pico da porta USB (se conectado).
    *   Pressione e segure o botão `BOOTSEL` no Pico.
    *   Conecte o Pico ao computador via cabo Micro USB enquanto mantém `BOOTSEL` pressionado.
    *   Solte o botão `BOOTSEL`. O Pico aparecerá como um dispositivo de armazenamento USB (como um pendrive) chamado `RPI-RP2`.
    *   Copie o arquivo `<nome_do_executavel>.uf2` (localizado na pasta `build`) para dentro do dispositivo `RPI-RP2`.
    *   O Pico reiniciará automaticamente e começará a executar o novo firmware.

---

## 📁 Estrutura do Código

O código-fonte está organizado da seguinte forma:


.
├── .vscode/ # Configurações do VS Code (launch.json, settings.json)
├── build/ # Diretório de compilação (gerado pelo CMake/Make)
├── lib/ # Bibliotecas locais do projeto
│ ├── Display_Bibliotecas/ # Biblioteca para o Display OLED
│ │ ├── font.h # Definição da fonte para o display
│ │ ├── ssd1306.c # Implementação do driver SSD1306
│ │ └── ssd1306.h # Header do driver SSD1306
│ ├── Matriz_Bibliotecas/ # Biblioteca para a Matriz WS2812
│ │ ├── generated/ # Arquivos gerados pelo pioasm
│ │ │ └── ws2812.pio.h # Header do programa PIO compilado
│ │ ├── matriz_led.c # Implementação do controle da matriz
│ │ ├── matriz_led.h # Header do controle da matriz
│ │ └── ws2812.pio # Código fonte PIO Assembly para WS2812
│ └── FreeRTOSConfig.h # Configurações específicas do FreeRTOS para este projeto
├── .gitignore # Arquivos e diretórios ignorados pelo Git
├── CMakeLists.txt # Script de configuração do CMake para o projeto
├── main.c # Arquivo principal com a lógica da aplicação e inicialização das tarefas
├── pico_sdk_import.cmake # Script padrão do SDK para importação
└── README.md # Este arquivo de documentação

*   **`main.c`:** Contém a função `main`, inicialização do sistema, criação das tarefas (se usar RTOS) e a lógica principal ou ponto de entrada.
*   **`lib/`:** Armazena bibliotecas customizadas ou de terceiros específicas para este projeto. Cada subdiretório geralmente contém um módulo (`.c` e `.h`).
*   **`*.pio`:** Arquivos de programa para o Programmable I/O (PIO) do RP2040. São compilados pelo `pioasm` (geralmente via CMake) para gerar headers (`.pio.h`).
*   **`CMakeLists.txt`:** Define como o projeto é construído, quais arquivos compilar, quais bibliotecas do SDK usar (ex: `pico_stdlib`, `hardware_i2c`, `hardware_pio`, `FreeRTOS-Kernel`) e como gerar o executável final (`.uf2`).
*   **`FreeRTOSConfig.h`:** (Se aplicável) Configurações do kernel FreeRTOS (tamanho da stack, clock, etc.).

---

## 👉 Como Usar

Após compilar e carregar o firmware no Pico:

1.  **Alimentação:** Conecte o Pico via USB. O projeto deve iniciar automaticamente.
2.  **Observar:** <Descreva o comportamento esperado. Ex: "O display OLED mostrará o status atual do semáforo. A matriz de LEDs exibirá um padrão correspondente à cor ativa (verde, amarelo ou vermelho). Os LEDs individuais seguirão a sequência do semáforo.">
3.  **Interagir:** <Descreva como interagir. Ex: "Pressione o Botão A (conectado ao GP<Número>) para alternar entre o modo NORMAL e o modo NOTURNO. O modo atual será exibido no OLED.">
4.  **Feedback Sonoro:** <Descreva o feedback sonoro. Ex: "O buzzer emitirá beeps com padrões diferentes dependendo do estado da luz (contínuo no verde, rápido no amarelo, lento no vermelho, esparso no noturno).">
5.  **(Opcional) Monitor Serial:** Conecte-se à porta serial USB do Pico usando um terminal serial (Putty, Minicom, Monitor Serial do VS Code/Arduino IDE) com baud rate `<Baud Rate, ex: 115200>` para visualizar mensagens de debug (se `printf` estiver habilitado).

---

## 🐛 Debugging e Logs

*   **Saída `printf`:** O projeto está configurado para enviar saída `printf` via USB Serial (verifique `pico_enable_stdio_usb` no `CMakeLists.txt`). Use um monitor serial no seu PC para ver essas mensagens.
    *   **Configuração:** Baud Rate: `115200` (geralmente padrão, mas confirme), Data bits: 8, Parity: None, Stop bits: 1.
    *   **Porta Serial:** No Linux, geralmente `/dev/ttyACM0`. No Windows, `COMx` (verifique no Gerenciador de Dispositivos).
*   **Debugging SWD:** Para debugging avançado (breakpoints, inspeção de memória), você pode usar um probe SWD (como outro Pico rodando Picoprobe, ou um J-Link/ST-Link) e o GDB via VS Code (com a extensão Cortex-Debug) ou linha de comando (`arm-none-eabi-gdb`). A configuração está geralmente em `.vscode/launch.json`.
*   **LEDs de Status:** <Se você usa LEDs específicos para indicar estados ou erros, descreva aqui. Ex: "O LED Azul (GP<Número>) pisca rapidamente se ocorrer um erro na inicialização do I2C.">

---

## 👤 Autor e Contato

*   **Autor:** <Jonas Souza Pinto>
*   **Email:** `<Jonassouza871@hotmail.com>`

