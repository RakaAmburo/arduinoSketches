# pltrx TODO

## Migración a Nano AVR (fase final)

- [ ] Activar Watchdog Timer (WDT) en sender y receiver
  - Include: #include <avr/wdt.h>
  - En setup(): wdt_enable(WDTO_8S);  (ajustar timeout segun necesidad)
  - En cada iteracion del loop(): wdt_reset();
  - Si el loop se atasca (ej: waitAck() bloqueante), el Nano reinicia solo
  - Timeouts disponibles: WDTO_1S, WDTO_2S, WDTO_4S, WDTO_8S
