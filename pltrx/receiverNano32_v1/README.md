# receiverNano32_v1

Receptor powerline KQ-130F sobre Arduino Nano ESP32.
Equivalente al senderNano32_v1 pero en el lado receptor.

## Hardware
- Board: Arduino Nano ESP32
- Módulo powerline: KQ-130F
- Bocina: pin D2 (activa en LOW)
- UART módulo: RX=D4, TX=D3 (Serial1, 9600bps) — igual que el sender
- Sin botón físico

## Topics MQTT
- pltrx/receiver/cmd — recibe comandos
- pltrx/receiver/log — publica logs
- El sender usa: pltrx/cmd y pltrx/log

## Comandos disponibles
- "send B" → el receiver envía B por powerline (para debug/pruebas)

## Flujo normal (cuando esté todo conectado)
1. Sender envía B por powerline
2. Receiver recibe B → suena bocina → envía K por powerline
3. Sender recibe K → loguea "ACK recibido ok"

## Estrategia de debug (PENDIENTE implementar en sender)

Para verificar que el powerline funciona antes de probar el flujo completo:

1. Escuchar logs del sender:
   mosquitto_sub -h 192.168.1.135 -t pltrx/log

2. Escuchar logs del receiver:
   mosquitto_sub -h 192.168.1.135 -t pltrx/receiver/log

3. Ordenar al receiver que envíe B:
   mosquitto_pub -h 192.168.1.135 -t pltrx/receiver/cmd -m "send B"

4. Si el sender recibe el B → debe loguearlo en pltrx/log

### Para que esto funcione hay que agregar handlePowerline() en el sender

- handlePowerline() lee Serial1 en cualquier momento (no bloqueante)
- Loguea por MQTT cualquier byte que llegue por powerline
- NO interrumpe el funcionamiento normal (botón + MQTT)
- El único solapamiento posible es si llega algo durante waitAck() — en ese caso waitAck() ya lo captura
- Pendiente: agregar handlePowerline() en loop() de senderNano32_v1 y subir OTA

## OTA
- Hostname: pltrx-receiver
- Upload desde oldrasp:
  arduino-cli upload --fqbn arduino:esp32:nano_nora -p pltrx-receiver.local /home/pablo/repos/arduinoSketches/pltrx/receiverNano32_v1
