#ifndef BUZZER_H
#define BUZZER_H

#include "pico/stdlib.h"
#include "hardware/pwm.h"

void buzzer_init(uint BUZZER_PIN);
void buzzer_play(uint BUZZER_PIN, uint freq, uint duration_ms);
void modo_verde(uint BUZZER_PIN);
void modo_amarelo(uint BUZZER_PIN);
void modo_vermelho(uint BUZZER_PIN);
void buzzer_noturno(uint BUZZER_PIN);

#endif
