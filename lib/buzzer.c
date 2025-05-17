#ifndef BUZZER_H
#define BUZZER_H

#include "pico/stdlib.h"
#include "hardware/pwm.h"

// Inicializa o buzzer (configura o pino como PWM)
void buzzer_init(uint BUZZER_PIN)
{
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
}

// Emite um som no buzzer com a frequência e duração desejadas
void buzzer_play(uint BUZZER_PIN, uint freq, uint duration_ms)
{
    // Obtém o número do slice PWM correspondente ao pino
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);

    // Calcula o valor do contador TOP para alcançar a frequência desejada
    uint top = 125000000 / freq;

    // Define o valor máximo do contador PWM (wrap)
    pwm_set_wrap(slice_num, top);

    // Define o duty cycle
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(BUZZER_PIN), (top * 2) / 10);

    // Ativa o PWM
    pwm_set_enabled(slice_num, true);

    // Toca o som pelo tempo especificado
    sleep_ms(duration_ms);

    // Desativa o PWM
    pwm_set_enabled(slice_num, false);

    // Pausa entre os tons
    sleep_ms(20);
}

// Som contínuo com beeps curtos a cada segundo – sinal verde
void modo_verde(uint BUZZER_PIN)
{
    buzzer_play(BUZZER_PIN, 1500, 100); // Beep único e curto
    sleep_ms(900);
}

// Sinal de atenção – beeps rápidos por 1s
void modo_amarelo(uint BUZZER_PIN)
{
    for (int i = 0; i < 5; i++)
    {
        buzzer_play(BUZZER_PIN, 2000, 100); // Beep de 100ms
        sleep_ms(100);                      // Pausa de 100ms
    }
}

// Sinal de alerta – som grave e longo
void modo_vermelho(uint BUZZER_PIN)
{
    buzzer_play(BUZZER_PIN, 1000, 500); // Tom de 1000 Hz por 500 ms
    sleep_ms(1500);
}

// Sinal de alerta noturno – som mais grave e mais longo
void buzzer_noturno(uint BUZZER_PIN)
{
    buzzer_play(BUZZER_PIN, 2000, 1000); // Tom de 2000 Hz por 1000 ms
    sleep_ms(1000);
}

#endif
