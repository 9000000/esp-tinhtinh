#ifndef BANK_H
#define BANK_H

#include <inttypes.h>

typedef struct notification_message
{
    char identifier[32];
    uint8_t title[64];
    uint8_t message[128];
} notification_message;

void notification_handling(notification_message *noti);

#endif // BANK_H