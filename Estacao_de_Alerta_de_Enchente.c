#include "pico/stdlib.h"
#include "stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include "lib/font.h"
#include "lib/ssd1306.h"
#include "lib/matrizLed.h"
#include "lib/buzzer.h"

#define LED_RED 13
#define LED_GREEN 11

#define STEP 0.10 // Passo de variação para cada movimento do joystick

#define ALERTA_N_AGUA 70
#define ALERTA_V_CHUVA 80

#define ADC_JOYSTICK_X 26
#define ADC_JOYSTICK_Y 27
#define BUZZER_PIN 21

uint16_t center_x;
uint16_t center_y;

bool leds_Normal[NUM_PIXELS] = {
    0, 1, 1, 1, 0,
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1,
    0, 1, 1, 1, 0};

bool leds_Alerta[NUM_PIXELS] = {
    0, 0, 0, 0, 0,
    1, 1, 1, 1, 1,
    0, 1, 1, 1, 0,
    0, 0, 1, 0, 0,
    0, 0, 0, 0, 0};

typedef struct
{
    float v_chuva;
    float n_agua;
} estacao_data_t;

QueueHandle_t xQueueEstacaoData;
volatile bool atualizar_display = true; // Pode ser controlado de outra task ou comando

void vJoystickTask(void *params)
{
    adc_gpio_init(ADC_JOYSTICK_Y);
    adc_gpio_init(ADC_JOYSTICK_X);
    adc_init();

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

    estacao_data_t estacao;
    estacao.n_agua = 60.0;
    estacao.v_chuva = 70.0;

    uint16_t x_pos;
    uint16_t y_pos;

    while (true)
    {
        adc_select_input(0); // GPIO 26 = ADC0
        y_pos = adc_read();

        adc_select_input(1); // GPIO 27 = ADC1
        x_pos = adc_read();

        int16_t deviation_y = y_pos - center_y;
        if (abs(deviation_y) > 1000)
        {
            if ((deviation_y > 0 && estacao.v_chuva < 100.0) ||
                (deviation_y < 0 && estacao.v_chuva > 0.0))
            {
                estacao.v_chuva += (deviation_y > 0) ? STEP : -STEP;
                // Garante que fique entre 0 e 100
                if (estacao.v_chuva > 100.0)
                    estacao.v_chuva = 100.0;
                if (estacao.v_chuva < 0.0)
                    estacao.v_chuva = 0.0;

                atualizar_display = true;
            }
        }

        int16_t deviation_x = x_pos - center_x;
        if (abs(deviation_x) > 1000)
        {
            if ((deviation_x > 0 && estacao.n_agua < 100.0) ||
                (deviation_x < 0 && estacao.n_agua > 0.0))
            {
                estacao.n_agua += (deviation_x > 0) ? STEP : -STEP;
                // Garante que fique entre 0 e 100
                if (estacao.n_agua > 100.0)
                    estacao.n_agua = 100.0;
                if (estacao.n_agua < 0.0)
                    estacao.n_agua = 0.0;

                atualizar_display = true;
            }
        }

        xQueueSend(xQueueEstacaoData, &estacao, 0); // Envia as condições para a fila
        vTaskDelay(pdMS_TO_TICKS(100));             // 10 Hz de leitura
    }
}

void vDisplayTask(void *params)
{
    ssd1306_t ssd;
    initDisplay(&ssd);

    while (true)
    {
        estacao_data_t estacao_data;
        if (xQueueReceive(xQueueEstacaoData, &estacao_data, portMAX_DELAY) == pdTRUE && atualizar_display)
        {
            char volume[8];
            char nivel[8];

            snprintf(volume, sizeof(volume), "%.2f%%", estacao_data.v_chuva);
            snprintf(nivel, sizeof(nivel), "%.2f%%", estacao_data.n_agua);

            ssd1306_fill(&ssd, false);
            desenhar(&ssd, icones);
            if (estacao_data.v_chuva > ALERTA_V_CHUVA)
            {
                desenhar(&ssd, alerta1);
            }
            if (estacao_data.n_agua > ALERTA_N_AGUA)
            {
                desenhar(&ssd, alerta2);
            }

            ssd1306_draw_string(&ssd, volume, 50, 12);
            ssd1306_draw_string(&ssd, nivel, 50, 45);
            ssd1306_send_data(&ssd); // Atualiza o display
            atualizar_display = false;
        }
    }
    vTaskDelay(pdMS_TO_TICKS(50));
}

void vMatrizTask(void *params)
{
    matriz_init();

    while (true)
    {
        estacao_data_t estacao_data;
        if (xQueueReceive(xQueueEstacaoData, &estacao_data, portMAX_DELAY) == pdTRUE)
        {
            if (estacao_data.v_chuva >= ALERTA_V_CHUVA || estacao_data.n_agua >= ALERTA_N_AGUA)
            {
                set_one_led(10, 0, 0, leds_Alerta);
            }
            else
            {
                set_one_led(0, 10, 0, leds_Normal);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void vLedTask(void *params)
{
    gpio_init(LED_RED);
    gpio_set_dir(LED_RED, GPIO_OUT);
    gpio_put(LED_RED, false);

    gpio_init(LED_GREEN);
    gpio_set_dir(LED_GREEN, GPIO_OUT);
    gpio_put(LED_GREEN, false);

    while (true)
    {
        estacao_data_t estacao_data;
        if (xQueueReceive(xQueueEstacaoData, &estacao_data, portMAX_DELAY) == pdTRUE)
        {
            if (estacao_data.v_chuva >= ALERTA_V_CHUVA || estacao_data.n_agua >= ALERTA_N_AGUA)
            {
                gpio_put(LED_RED, true);
                gpio_put(LED_GREEN, false);
            }
            else
            {
                gpio_put(LED_GREEN, true);
                gpio_put(LED_RED, false);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void vBuzzerTask(void *params)
{
    buzzer_init(BUZZER_PIN);

    while (true)
    {
        estacao_data_t estacao_data;
        if ((xQueueReceive(xQueueEstacaoData, &estacao_data, portMAX_DELAY) == pdTRUE) &&
            (estacao_data.v_chuva >= ALERTA_V_CHUVA || estacao_data.n_agua >= ALERTA_N_AGUA))
        {
            buzzer_play(BUZZER_PIN, 1000, 300);
            buzzer_play(BUZZER_PIN, 1500, 300);
            vTaskDelay(pdMS_TO_TICKS(400));
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
    xTaskCreate(vDisplayTask, "Display Task", 512, NULL, 1, NULL);
    xTaskCreate(vLedTask, "LED Task", 256, NULL, 1, NULL);
    xTaskCreate(vBuzzerTask, "Buzzer Task", 256, NULL, 1, NULL);
    xTaskCreate(vMatrizTask, "Matriz Task", 256, NULL, 1, NULL);

    // Inicia o agendador
    vTaskStartScheduler();
    panic_unsupported();
}
