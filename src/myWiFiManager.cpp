#include "myWiFiManager.h"
#include <EEPROM.h>

static wifi_state_t s_state = WIFI_STATE_DISCONNECTED;
static char s_ssid[33] = {0};
static char s_pass_saved[65] = {0};
static uint8_t s_wifi_channel = 0;  /* 0 = auto (any channel), 1-13 = forced */
static unsigned long s_last_attempt = 0;
static unsigned long s_portal_start = 0;
static bool s_portal_active = false;

static bool s_reconnect_active = false;
static unsigned long s_reconnect_deadline = 0;
#if !defined(ARDUINO_ARCH_ESP32)
  static WiFiManager s_wm;
#endif

static bool sh_creds_load(char *ssid, char *pass) {
    EEPROM.begin(EEPROM_SIZE);
    bool valid = false;
    int pos = 0;
    for (int i = 0; i < EEPROM_WIFI_SSID_SIZE; i++) {
        uint8_t c = EEPROM.read(EEPROM_WIFI_SSID_OFFSET + i);
        if (c == 0) { valid = pos > 0; break; }
        if (c < 32 || c > 126) break;
        ssid[pos++] = (char)c;
    }
    ssid[pos] = '\0';
    for (int i = 0; i < EEPROM_WIFI_PASS_SIZE; i++)
        pass[i] = EEPROM.read(EEPROM_WIFI_PASS_OFFSET + i);
    pass[EEPROM_WIFI_PASS_SIZE - 1] = '\0';
    s_wifi_channel = EEPROM.read(EEPROM_WIFI_CHANNEL_OFFSET);
    EEPROM.end();
    return valid && strlen(ssid) > 0;
}

static void sh_net_load(int *mode, char *ip, char *gw, char *mask, char *dns) {
    EEPROM.begin(EEPROM_SIZE);
    *mode = EEPROM.read(EEPROM_WIFI_MODE_OFFSET) == WIFI_MODE_STATIC ? WIFI_MODE_STATIC : WIFI_MODE_DHCP;
    auto read_str = [](int off, int size, char *buf) {
        int pos = 0;
        for (int i = 0; i < size - 1; i++) {
            uint8_t c = EEPROM.read(off + i);
            if (c == 0) break;
            if (c < 32 || c > 126) break;
            buf[pos++] = (char)c;
        }
        buf[pos] = '\0';
    };
    read_str(EEPROM_WIFI_IP_OFFSET, EEPROM_WIFI_IP_SIZE, ip);
    read_str(EEPROM_WIFI_GW_OFFSET, EEPROM_WIFI_GW_SIZE, gw);
    read_str(EEPROM_WIFI_MASK_OFFSET, EEPROM_WIFI_MASK_SIZE, mask);
    read_str(EEPROM_WIFI_DNS_OFFSET, EEPROM_WIFI_DNS_SIZE, dns);
    EEPROM.end();
}

static void apply_static_ip() {
    int mode = WIFI_MODE_DHCP;
    char ip[16], gw[16], mask[16], dns[16];
    sh_net_load(&mode, ip, gw, mask, dns);
    if (mode != WIFI_MODE_STATIC || strlen(ip) == 0) return;
    IPAddress ipa, gwa, maska, dnsa;
    if (!ipa.fromString(ip)) return;
    maska = IPAddress(255, 255, 255, 0);
    if (mask[0]) maska.fromString(mask);
    gwa = INADDR_NONE;
    if (gw[0]) gwa.fromString(gw);
    dnsa = gwa;
    if (dns[0]) dnsa.fromString(dns);
    WiFi.config(ipa, gwa, maska, dnsa);
}

bool mywifi_begin(bool force_portal) {
    char saved_ssid[EEPROM_WIFI_SSID_SIZE];
    char saved_pass[EEPROM_WIFI_PASS_SIZE];
    bool have_creds = sh_creds_load(saved_ssid, saved_pass);
    if (saved_ssid[0]) strncpy(s_ssid, saved_ssid, sizeof(s_ssid) - 1);
    if (saved_pass[0]) strncpy(s_pass_saved, saved_pass, sizeof(s_pass_saved) - 1);

#if defined(ARDUINO_ARCH_ESP32)
    WiFi.mode(WIFI_STA);
    apply_static_ip();
    /* Step 1: Try saved credentials */
    if (!force_portal && have_creds) {
        Serial.printf("[wifi] Step 1: Connecting to saved: %s\n", saved_ssid);
        WiFi.begin(saved_ssid, saved_pass);
        s_state = WIFI_STATE_CONNECTING;
        s_last_attempt = millis();
        return true;
    }
#ifdef STATIC_WIFI
    /* Step 2: Try STATIC_WIFI */
    if (strlen(WIFI_SSID) > 0) {
        Serial.printf("[wifi] Step 2: Trying STATIC_WIFI: %s\n", WIFI_SSID);
        strncpy(s_ssid, WIFI_SSID, sizeof(s_ssid) - 1);
        s_ssid[sizeof(s_ssid) - 1] = '\0';
        strncpy(s_pass_saved, WIFI_PASS, sizeof(s_pass_saved) - 1);
        s_pass_saved[sizeof(s_pass_saved) - 1] = '\0';
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        s_state = WIFI_STATE_CONNECTING;
        s_last_attempt = millis();
        return true;
    }
#endif
    /* Step 3: AP mode */
    s_state = WIFI_STATE_PORTAL;
    s_portal_active = true;
    s_portal_start = millis();
    return false;
#else
    /* Step 1: Try saved credentials */
    if (!force_portal && have_creds) {
        Serial.printf("[wifi] Step 1: Connecting to saved: %s\n", saved_ssid);
        WiFi.mode(WIFI_STA);
        apply_static_ip();
        WiFi.begin(saved_ssid, saved_pass);
        s_state = WIFI_STATE_CONNECTING;
        s_last_attempt = millis();
        return true;
    }
#ifdef STATIC_WIFI
    /* Step 2: Try STATIC_WIFI */
    if (strlen(WIFI_SSID) > 0) {
        Serial.printf("[wifi] Step 2: Trying STATIC_WIFI: %s\n", WIFI_SSID);
        WiFi.mode(WIFI_STA);
        apply_static_ip();
        strncpy(s_ssid, WIFI_SSID, sizeof(s_ssid) - 1);
        s_ssid[sizeof(s_ssid) - 1] = '\0';
        strncpy(s_pass_saved, WIFI_PASS, sizeof(s_pass_saved) - 1);
        s_pass_saved[sizeof(s_pass_saved) - 1] = '\0';
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        s_state = WIFI_STATE_CONNECTING;
        s_last_attempt = millis();
        return true;
    }
#endif
    /* Step 3: AP mode (blocking portal for ESP8266) */
    s_state = WIFI_STATE_PORTAL;
    s_portal_active = true;
    s_portal_start = millis();
    s_wm.setConfigPortalTimeout(300);
    s_wm.startConfigPortal(WIFI_CONFIG_PORTAL_SSID, WIFI_CONFIG_PORTAL_PASS);
    return false;
#endif
}

void mywifi_loop() {
    if (WiFi.status() == WL_CONNECTED) {
        if (s_wifi_channel > 0) {
            /* After connection, verify channel matches configured. If not, the
               node likely connected to an extender on a different channel.
               Scan for the correct BSSID and force-reconnect. */
            static bool s_channel_verified = false;
            if (!s_channel_verified) {
                s_channel_verified = true;
                int ch = WiFi.channel();
                if (ch != s_wifi_channel) {
                    Serial.printf("[wifi] Channel mismatch: got %d, expected %d — scanning...\n", ch, s_wifi_channel);
                    int n = WiFi.scanNetworks();
                    for (int i = 0; i < n; i++) {
                        if (WiFi.SSID(i) == s_ssid && WiFi.channel(i) == s_wifi_channel) {
                            uint8_t bssid[6];
                            memcpy(bssid, WiFi.BSSID(i), 6);
                            char bssid_str[18];
                            snprintf(bssid_str, sizeof(bssid_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                                     bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
                            Serial.printf("[wifi] Found %s on ch %d (BSSID %s) — reconnecting\n",
                                          s_ssid, s_wifi_channel, bssid_str);
                            WiFi.disconnect();
                            delay(100);
                            WiFi.begin(s_ssid, s_pass_saved, s_wifi_channel, bssid);
                            s_channel_verified = false;
                            break;
                        }
                    }
                    WiFi.scanDelete();
                }
            }
        }
        if (s_state != WIFI_STATE_CONNECTED) s_state = WIFI_STATE_CONNECTED;
        s_portal_active = false;
        return;
    }

    if (s_state == WIFI_STATE_PORTAL) {
        if (s_portal_active && millis() - s_portal_start > 300000)
            s_portal_active = false;
#if !defined(ARDUINO_ARCH_ESP32)
        #if defined(WIFIMANAGER_VERSION)
        s_wm.process();
        #endif
#endif
        return;
    }

    static unsigned long last_attempt = 0;
    if (!s_reconnect_active) {
        if (millis() - last_attempt < 30000) return;
        last_attempt = millis();
        WiFi.mode(WIFI_STA);
        apply_static_ip();
        /* Step 1: Try saved EEPROM creds */
        char ssid[EEPROM_WIFI_SSID_SIZE];
        char pass[EEPROM_WIFI_PASS_SIZE];
        if (sh_creds_load(ssid, pass)) {
            Serial.printf("[wifi] Reconnecting (step 1 - EEPROM): %s\n", ssid);
            WiFi.begin(ssid, pass);
        }
#ifdef STATIC_WIFI
        /* Step 2: Try STATIC_WIFI */
        else if (strlen(WIFI_SSID) > 0) {
            Serial.printf("[wifi] Reconnecting (step 2 - STATIC_WIFI): %s\n", WIFI_SSID);
            WiFi.begin(WIFI_SSID, WIFI_PASS);
        }
#endif
        else {
            WiFi.begin();
        }
        s_reconnect_active = true;
        s_reconnect_deadline = millis() + 15000;
    } else if (millis() >= s_reconnect_deadline) {
        s_reconnect_active = false;
    }
}

wifi_state_t mywifi_state() { return s_state; }
const char* mywifi_ssid() { return s_ssid; }
int mywifi_channel() { return WiFi.channel(); }
int mywifi_configured_channel() { return s_wifi_channel; }

void mywifi_save_channel(uint8_t ch) {
    s_wifi_channel = ch;
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.write(EEPROM_WIFI_CHANNEL_OFFSET, ch);
    EEPROM.commit();
    EEPROM.end();
    Serial.printf("[wifi] Channel saved: %d\n", ch);
}

bool mywifi_portal(char *name_buf, size_t name_size, void (*on_name)(const char*)) {
#if defined(ARDUINO_ARCH_ESP32)
    return false;
#else
    WiFiManager wm;
    wm.setConfigPortalTimeout(300);
    WiFiManagerParameter custom_dev_name("dev_name", "Device Name",
                                         name_buf ? name_buf : "", name_size > 0 ? (int)name_size : 32);
    wm.addParameter(&custom_dev_name);
    if (wm.startConfigPortal(WIFI_CONFIG_PORTAL_SSID, WIFI_CONFIG_PORTAL_PASS)) {
        if (name_buf && on_name) {
            const char *v = custom_dev_name.getValue();
            if (v && v[0]) {
                strncpy(name_buf, v, name_size - 1);
                name_buf[name_size - 1] = '\0';
                on_name(name_buf);
            }
        }
        return true;
    }
    return false;
#endif
}

void mywifi_espnow_mac(uint8_t *out) {
    uint8_t wifi_mac[6];
    WiFi.macAddress(wifi_mac);
    memcpy(out, wifi_mac, 6);
    out[0] ^= 0x02;
}

/* ---- Dynamic AP discovery ----
   When a node can't pair on the current WiFi channel (connected to an
   extender on a different channel than the hub), scan for other APs
   with the same SSID and try each one until pairing succeeds. */

static uint8_t s_tried_bssids[8][6];
static int s_tried_count = 0;
static int s_scan_index = 0;
static int s_scan_count = 0;

static bool bssid_in_list(const uint8_t *bssid) {
    for (int i = 0; i < s_tried_count; i++) {
        if (memcmp(s_tried_bssids[i], bssid, 6) == 0) return true;
    }
    return false;
}

static void bssid_add(const uint8_t *bssid) {
    if (s_tried_count < 8) {
        memcpy(s_tried_bssids[s_tried_count++], bssid, 6);
    }
}

bool mywifi_try_next_bssid() {
    if (!s_ssid[0]) return false;

    Serial.printf("[wifi] Scanning for other APs with SSID '%s'...\n", s_ssid);
    int n = WiFi.scanNetworks();

    /* Collect unseen BSSIDs for our SSID */
    int found = 0;
    for (int i = 0; i < n; i++) {
        if (WiFi.SSID(i) == s_ssid) {
            uint8_t bssid[6];
            memcpy(bssid, WiFi.BSSID(i), 6);
            if (!bssid_in_list(bssid)) {
                found++;
            }
        }
    }
    WiFi.scanDelete();

    if (found == 0) {
        Serial.printf("[wifi] No other APs found for '%s'\n", s_ssid);
        s_tried_count = 0; /* reset for next cycle */
        return false;
    }

    /* Re-scan and connect to the first unseen AP */
    n = WiFi.scanNetworks();
    for (int i = 0; i < n; i++) {
        if (WiFi.SSID(i) == s_ssid) {
            uint8_t bssid[6];
            memcpy(bssid, WiFi.BSSID(i), 6);
            int ch = WiFi.channel(i);
            if (!bssid_in_list(bssid)) {
                bssid_add(bssid);
                WiFi.scanDelete();

                char bssid_str[18];
                snprintf(bssid_str, sizeof(bssid_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                         bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
                Serial.printf("[wifi] Trying AP %s on channel %d\n", bssid_str, ch);

                WiFi.disconnect();
                delay(100);
                WiFi.begin(s_ssid, s_pass_saved, ch, bssid);
                s_state = WIFI_STATE_CONNECTING;
                return true;
            }
        }
    }
    WiFi.scanDelete();
    return false;
}
