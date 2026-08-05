#ifndef HW_SHARED_WATCHDOG_H
#define HW_SHARED_WATCHDOG_H

#include <Arduino.h>

// Watchdog estavel (a prova de flip-flop de WiFi/heartbeat):
// o timer de restart so e (re)armado apos um periodo CONTiNUO e saudavel
// (stable_reset_ms). Reconexoes breves (< stable_reset_ms) nao estendem o
// timer, entao um device que fica ciclando conecta/desconecta nunca desarma
// o watchdog.
//
// Uso:
//   StableWatchdog wd;
//   wd.init(60000, 300000); // 60s estaveis p/ armar, 5min p/ restart
//   no loop: if (wd.check(healthy)) { ESP.restart(); }
//
// arm_from_start: quando true, o timer parte armado desde a init() — um
// device que NUNCA fica saudavel (ex: nunca conecta no WiFi) reinicia mesmo
// assim apos restart_ms. Use para watchdogs de auto-recuperacao; deixe false
// para preservar "so reinicia depois de ter ficado estavel ao menos uma vez".
class StableWatchdog
{
public:
    StableWatchdog()
        : m_stable_reset_ms(0), m_restart_ms(0)
        , m_stable_since(0), m_last_reset(0), m_arm_from_start(false)
    {
    }

    // stable_reset_ms: tempo saudavel continuo para armar o timer (ex: 60000)
    // restart_ms:      tempo "quebrado" acumulado para disparar o restart
    void init(unsigned long stable_reset_ms, unsigned long restart_ms, bool arm_from_start = false)
    {
        m_stable_reset_ms = stable_reset_ms;
        m_restart_ms = restart_ms;
        m_arm_from_start = arm_from_start;
        reset();
    }

    // Chamar a cada loop com o estado atual (true = saudavel).
    // Retorna true quando o restart deve ser executado (ESP.restart()).
    bool check(bool healthy)
    {
        return check(healthy, millis());
    }

    // Variante com tempo explicito (para testes / clock customizado).
    bool check(bool healthy, unsigned long now)
    {
        if (healthy)
        {
            if (m_stable_since == 0)
            {
                m_stable_since = now;
            }
            else if (now - m_stable_since >= m_stable_reset_ms)
            {
                // saudavel de forma continua por tempo suficiente: arma/estende
                m_last_reset = now;
            }
        }
        else
        {
            // quebrou: reinicia a contagem de estabilidade
            m_stable_since = 0;
        }
        return (m_last_reset > 0 && now - m_last_reset > m_restart_ms);
    }

    // Reinicia o watchdog (ex: no setup ou apos restart manual)
    void reset()
    {
        m_stable_since = 0;
        m_last_reset = m_arm_from_start ? millis() : 0;
    }

private:
    unsigned long m_stable_reset_ms;
    unsigned long m_restart_ms;
    unsigned long m_stable_since; // inicio da sequencia saudavel atual
    unsigned long m_last_reset;   // ultima vez que o timer foi armado
    bool m_arm_from_start;
};

#endif // HW_SHARED_WATCHDOG_H
