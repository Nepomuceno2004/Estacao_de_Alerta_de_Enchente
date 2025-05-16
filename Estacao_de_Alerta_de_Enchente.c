#include "pico/stdlib.h"
#include "stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>

#define LED_BLUE 12
#define LED_RED 13

#define STEP 0.05 // Passo de variação para cada movimento do joystick

#define ADC_JOYSTICK_X 26
#define ADC_JOYSTICK_Y 27

uint16_t center_x;
uint16_t center_y;

typedef struct
{
    float v_chuva;
    float n_agua;
} estacao_data_t;

typedef struct
{
    uint16_t x_pos;
    uint16_t y_pos;
} joystick_data_t;

QueueHandle_t xQueueEstacaoData;

void vJoystickTask(void *params)
{
    adc_gpio_init(ADC_JOYSTICK_Y);
    adc_gpio_init(ADC_JOYSTICK_X);
    adc_init();

    joystick_data_t joydata;

    // capturar o centro dos dois eixos
    uint32_t sum = 0;
    for (int i = 0; i < 100; i++)
    {
        adc_select_input(0);
        sum += adc_read();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    center_x = sum / 100;

    sum = 0;
    for (int i = 0; i < 100; i++)
    {
        adc_select_input(1);
        sum += adc_read();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    center_y = sum / 100;

    printf("x = %u\n", center_x);
    printf("y = %u\n", center_y);

    estacao_data_t estacao;
    estacao.n_agua = 2.0;
    estacao.v_chuva = 2.0;

    while (true)
    {
        adc_select_input(0); // GPIO 26 = ADC0
        joydata.y_pos = adc_read();

        adc_select_input(1); // GPIO 27 = ADC1
        joydata.x_pos = adc_read();

        int16_t deviation_y = joydata.y_pos - center_y;
        if (abs(deviation_y) > 1000)
        {
            estacao.n_agua += (deviation_y > 0) ? STEP : -STEP;
            printf("Nível de água: %.2f %%\n\n", estacao.n_agua);
        }

        int16_t deviation_x = joydata.x_pos - center_x;
        if (abs(deviation_x) > 1000)
        {
            estacao.v_chuva += (deviation_x > 0) ? STEP : -STEP;
            printf("Volume de chuva: %.2f %%\n\n", estacao.v_chuva);
        }

        xQueueSend(xQueueEstacaoData, &estacao, 0); // Envia o valor do joystick para a fila
        vTaskDelay(pdMS_TO_TICKS(100));             // 10 Hz de leitura
    }
}

void vLedBlueTask(void *params)
{
    gpio_init(LED_BLUE);
    gpio_set_dir(LED_BLUE, GPIO_OUT);
    gpio_put(LED_BLUE, false);

    gpio_init(LED_RED);
    gpio_set_dir(LED_RED, GPIO_OUT);
    gpio_put(LED_RED, false);

    while (true)
    {
        estacao_data_t estacao_data;
        if (xQueueReceive(xQueueEstacaoData, &estacao_data, portMAX_DELAY) == pdTRUE)
        {
            if (estacao_data.n_agua > 3.0)
            {
                gpio_put(LED_BLUE, true);
            }
            else
            {
                gpio_put(LED_BLUE, false);
            }
            
            if (estacao_data.v_chuva > 3.0)
            {
                gpio_put(LED_RED, true);
            }
            else
            {
                gpio_put(LED_RED, false);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Modo BOOTSEL com botão B
#include "pico/bootrom.h"
#define botaoB 6
void gpio_irq_handler(uint gpio, uint32_t events)
{
    reset_usb_boot(0, 0);
}

int main()
{
    // Ativa BOOTSEL via botão
    gpio_init(botaoB);
    gpio_set_dir(botaoB, GPIO_IN);
    gpio_pull_up(botaoB);
    gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    stdio_init_all();

    // Cria a fila para compartilhamento de valor do joystick
    xQueueEstacaoData = xQueueCreate(5, sizeof(estacao_data_t));

    // Criação das tasks
    xTaskCreate(vJoystickTask, "Joystick Task", 256, NULL, 1, NULL);
    xTaskCreate(vLedBlueTask, "LED blue Task", 256, NULL, 1, NULL);

    // Inicia o agendador
    vTaskStartScheduler();
    panic_unsupported();
}
