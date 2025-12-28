#include "gerunds.h"
#include "esp_random.h"

const char* get_random_gerund(void) {
    uint32_t random_index = esp_random() % GERUNDS_COUNT;
    return gerunds[random_index];
}
