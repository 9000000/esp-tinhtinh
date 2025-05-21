#include "bank.h"
#include "esp_log.h"
#include <string.h>
#include <sys/types.h>
#include <regex.h>
#include "sound.h"

#define TAG "KTM_BANK"

/* MBBank

I (133077) KTM_BANK: =============================== notification:
I (133087) KTM_BANK: Identifier: com.mbmobile
I (133087) KTM_BANK: Title: Thông báo biến động số dư
I (133097) KTM_BANK: Message: TK 98xxx666|GD: +100,000VND 13/05/25 10:41 |SD: 818,007VND|ND: NGUYEN DUC TIN chuyen tien

*/

int32_t atoi_nocomma(const char *str)
{
    int32_t result = 0;
    while (*str)
    {
        if (*str >= '0' && *str <= '9')
        {
            result = result * 10 + (*str - '0');
        }
        str++;
    }
    return result;
}

void notification_handling(notification_message *noti)
{
    if (strcmp(noti->identifier, "com.mbmobile") == 0)
    {
        ESP_LOGI(TAG, "Handling notification:");
        ESP_LOGI(TAG, "Identifier: %s", noti->identifier);
        ESP_LOGI(TAG, "Title: %s", noti->title);
        ESP_LOGI(TAG, "Message: %s", noti->message);

        regex_t regex;
        regmatch_t matches[2]; // [0]: toàn bộ; [1]: phần trong dấu ()

        const char *pattern = "GD: \\+([[:digit:],]*)VND";
        int ret = regcomp(&regex, pattern, REG_EXTENDED);

        if (ret != 0)
        {
            printf("Lỗi biên dịch regex\n");
            return;
        }

        ret = regexec(&regex, (const char *)noti->message, 2, matches, 0);
        if (ret == 0)
        {
            char match[32];
            int start = matches[1].rm_so;
            int end = matches[1].rm_eo;

            strncpy(match, (const char *)noti->message + start, end - start);
            match[end - start] = '\0';

            int32_t value = atoi_nocomma(match);
            xQueueSend(xQueue, &value, portMAX_DELAY);
        }
        else
        {
            printf("Không tìm thấy số tiền\n");
        }

        regfree(&regex);
    }
    else
    {
        ESP_LOGI(TAG, "=============================== notification:");
        ESP_LOGI(TAG, "Identifier: %s", noti->identifier);
        ESP_LOGI(TAG, "Title: %s", noti->title);
        ESP_LOGI(TAG, "Message: %s", noti->message);
    }
}