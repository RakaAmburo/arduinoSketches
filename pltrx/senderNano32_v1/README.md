# senderNano32_v1 — Emisor pltrx (KQ-130F)

## Estado
✅ Funcional — OTA, MQTT, botón físico, envío por powerline

## Hardware

| Componente | Detalle |
|---|---|
| Placa | Arduino Nano ESP32 |
| Alimentación | USB (VBUS = 5V disponible) |
| KQ-130F (pines 3+5) | 5V desde VBUS |
| Convertidor lógico (HV) | 5V desde VBUS |
| Convertidor lógico (LV) | 3.3V del pin 3V3 del Nano |
| Botón | Pin D7, señal a 3.3V |
| GND | Compartido para todo |

## Conexión convertidor lógico (BSS138 bidireccional)
- LV = 3.3V (Nano 3V3)
- HV = 5V (VBUS)
- GND compartido
- Canal A: D3 (TX Nano) ↔ RX del KQ-130F (pin 6)
- Canal B: D4 (RX Nano) ↔ TX del KQ-130F (pin 7)

## Pines Nano ESP32
| Pin | Función |
|---|---|
| D7 | Botón (INPUT_PULLUP, activo LOW) |
| D4 | RX Serial1 ← TX KQ-130F (via conv. lógico) |
| D3 | TX Serial1 → RX KQ-130F (via conv. lógico) |

## UART
-  — hardware UART, pines custom
- KQ-130F en modo transparente (MODE flotante)
- Interfaz microcontrolador ↔ módulo: 9600 bps
- Velocidad interna del módulo sobre línea AC: 960 bps (transparente al sketch)

## OTA
- Hostname:  (192.168.1.192)
- Password: ver secrets.h
- Primera subida: USB por COM6
- Siguientes: 

## MQTT (broker: oldrasp 192.168.1.135:1883)
| Topic | Dirección | Uso |
|---|---|---|
|  | → ESP32 | Enviar comandos |
|  | ← ESP32 | Ver logs |

### Comandos


## Funcionalidades
- ✅ Envío de señal  por powerline (botón físico D7)
- ✅ Envío de señal  por MQTT ()
- ✅ OTA — updates remotos sin USB
- ✅ Log de resultados por MQTT
- ⏳ ACK () del receptor — pendiente (problema conocido en receiver_v2)

## Librerías
-  2.8.0 (Nick O'Leary)
-  — incluida en board ESP32
-  — incluida en board ESP32

## FQBN


## Archivos
-  — sketch principal
-  — credenciales WiFi y OTA (no subir a git)