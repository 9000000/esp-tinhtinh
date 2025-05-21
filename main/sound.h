#ifndef SOUND_H
#define SOUND_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

extern QueueHandle_t xQueue; // Khai báo queue

void sound_init(void);

#endif // SOUND_H