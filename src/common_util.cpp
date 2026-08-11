#include "common_util.h"

const char* getDeviceId() {
    static char s_device_id[32] = {0};
    if (s_device_id[0] == '\0') {
        #ifdef ESPNOW_ENABLED
          snprintf(s_device_id, sizeof(s_device_id), "%s%s_%06x", PLATFORM_PREFIX,"EN", chip_id());
        #elif defined(LORA_ENABLED)
          snprintf(s_device_id, sizeof(s_device_id), "%s%s_%06x", PLATFORM_PREFIX,"LR", chip_id());
        #elif defined(TCP_ENABLED)  
          snprintf(s_device_id, sizeof(s_device_id), "%s%s_%06x", PLATFORM_PREFIX,"TP", chip_id());
        #else
        snprintf(s_device_id, sizeof(s_device_id), "%s_%06x", PLATFORM_PREFIX, chip_id());
        #endif
    }
    return s_device_id;
}

void uptime_to_str(unsigned long ms, char *buf, size_t len) {
    unsigned long s = ms / 1000;
    unsigned long d = s / 86400;
    unsigned long h = (s % 86400) / 3600;
    unsigned long m = (s % 3600) / 60;
    unsigned long sec = s % 60;
    if (d > 0) snprintf(buf, len, "%lud %luh%lum%lus", d, h, m, sec);
    else if (h > 0) snprintf(buf, len, "%luh%lum%lus", h, m, sec);
    else if (m > 0) snprintf(buf, len, "%lum%lus", m, sec);
    else snprintf(buf, len, "%lus", sec);
}

void uptime_to_str(unsigned long ms, String &out) {
    char buf[32];
    uptime_to_str(ms, buf, sizeof(buf));
    out = buf;
}
