# Estação de Alerta de Enchente com Simulação por Joystick

Este projeto implementa uma estação de monitoramento de cheias utilizando a placa BitDog Lab com RP2040. O sistema simula a leitura do nível da água e volume de chuva através de um joystick, processa esses dados e gera alertas visuais e sonoros quando os valores ultrapassam limites pré-definidos. O sistema foi desenvolvido utilizando FreeRTOS para gerenciamento de tarefas e filas para comunicação entre elas.

## Funcionalidades
- Simulação de dados: O joystick controla os valores simulados de nível da água e volume de chuva.
- Modos de operação:
  - Modo Normal: Exibe os valores atuais no display OLED e acende LED verde.
  - Modo Alerta: Ativado quando:
    - Nível da água ≥ 70%
    - Volume de chuva ≥ 80%
  - No modo alerta:
  - LED RGB acende em vermelho
  - Buzzer emite sinais sonoros intermitentes
  - Matriz de LEDs exibe um padrão de alerta
  - Display OLED mostra os dados com destaque

 ## Componentes Utilizados
 - Placa BitDog Lab com RP2040
 - Joystick analógico (para simulação de entrada)
 - Display OLED
 - Matriz de LEDs
 - Buzzer
 - LEDs RGB
 - Botão para modo BOOTSEL

## Como Usar
1. Hardware:
    - Conecte todos os componentes conforme definido no código:
    - Joystick nos ADCs 0 (GPIO 26) e 1 (GPIO 27)
    - Buzzer no GPIO 21
    - LED vermelho no GPIO 13
    - LED verde no GPIO 11
    - Botão B no GPIO 6 (para modo BOOTSEL)
  
2. Funcionamento:
    - O sistema inicia automaticamente no modo normal
    - Movimente o joystick:
      - Eixo Y controla o volume de chuva (0-100%)
      - Eixo X controla o nível da água (0-100%)
    - Quando algum valor atingir o limiar de alerta, o sistema entrará automaticamente no modo de alerta

3. Personalização:
    - Os limiares de alerta podem ser ajustados modificando:
```c
#define ALERTA_N_AGUA 70
#define ALERTA_V_CHUVA 80
```

## Autor
## Matheus Nepomuceno Souza
