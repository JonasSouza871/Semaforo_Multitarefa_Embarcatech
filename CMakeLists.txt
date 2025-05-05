
cmake_minimum_required(VERSION 3.13)  # Padrões 
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Definindo a placa
set(PICO_BOARD pico_w CACHE STRING "Board type")

# SDK do Pico
include(pico_sdk_import.cmake)

# Caminho para o FreeRTOS (corrigido)
set(FREERTOS_KERNEL_PATH "C:/Users/Jonas/FreeRTOS-Kernel")
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

# Nome do projeto e executável
project(semaforo C CXX ASM)
pico_sdk_init()

# Diretórios de include do projeto
include_directories(${CMAKE_SOURCE_DIR}/lib)

# Criando o executável
add_executable(semaforo
    main
    lib/Display_Bibliotecas/ssd1306.c
    lib/Matriz_Bibliotecas/matriz_led.c
)

# Adiciona o diretório raiz como include
target_include_directories(semaforo PRIVATE ${CMAKE_SOURCE_DIR})

# Bibliotecas utilizadas
target_link_libraries(semaforo
    pico_stdlib
    hardware_i2c       # <-- driver I²C do pico-sdk
    FreeRTOS-Kernel
    FreeRTOS-Kernel-Heap4
    hardware_pwm
    hardware_pio
)

# Habilita I/O padrão via USB (ou UART)
pico_enable_stdio_usb(semaforo 1)
pico_enable_stdio_uart(semaforo 0)

# Gera arquivos binários/UF2
pico_add_extra_outputs(semaforo)