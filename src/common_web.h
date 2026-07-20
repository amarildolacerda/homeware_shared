#ifndef HW_SHARED_WEB_H
#define HW_SHARED_WEB_H

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

// Serve PROGMEM HTML page chunked (avoids heap fragmentation on ESP8266).
// Call from your route handler:
//   serve_pgm_page(s_server, PAGE_DASHBOARD);
static inline void serve_pgm_page(ESP8266WebServer &server, const char *page)
{
    size_t total = strlen_P(page);
    WiFiClient cl = server.client();
    cl.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: "));
    cl.print(total);
    cl.print(F("\r\nConnection: close\r\n\r\n"));
    PGM_P src = page;
    char buf[256];
    while (total > 0)
    {
        size_t chunk = total > sizeof(buf) ? sizeof(buf) : total;
        memcpy_P(buf, src, chunk);
        cl.write((const uint8_t *)buf, chunk);
        src += chunk;
        total -= chunk;
        yield();
    }
}

// Read/write a GPIO pin via JSON API.
// GET  /api/pin?gpio=N  — read pin state
// POST /api/pin         — {"gpio":N, "state":0|1}
static inline void handle_api_pin(ESP8266WebServer &server)
{
    if (server.method() == HTTP_GET)
    {
        int pin = server.arg("gpio").toInt();
        pinMode(pin, INPUT_PULLUP);
        int state = digitalRead(pin);
        String json;
        JsonDocument doc;
        doc["gpio"] = pin;
        doc["state"] = state;
        serializeJson(doc, json);
        server.send(200, "application/json", json);
    }
    else if (server.method() == HTTP_POST)
    {
        String body = server.arg("plain");
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, body);
        if (err)
        {
            server.send(400, "application/json", "{\"error\":\"invalid JSON\"}");
            return;
        }
        int pin = doc["gpio"] | -1;
        if (pin < 0 || pin > 16)
        {
            server.send(400, "application/json", "{\"error\":\"invalid gpio\"}");
            return;
        }
        int state = doc["state"] | -1;
        if (state != 0 && state != 1)
        {
            server.send(400, "application/json", "{\"error\":\"state must be 0 or 1\"}");
            return;
        }
        pinMode(pin, OUTPUT);
        digitalWrite(pin, state);
        String json;
        JsonDocument resp;
        resp["gpio"] = pin;
        resp["state"] = state;
        resp["status"] = "ok";
        serializeJson(resp, json);
        server.send(200, "application/json", json);
    }
}

#endif
