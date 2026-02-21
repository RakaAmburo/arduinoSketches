import network
import time
from umqtt.simple import MQTTClient
import secrets

# CONFIGURACIÓN
MQTT_BROKER = "192.168.1.157"  # IP de tu PC
MQTT_PORT = 1883
CLIENT_ID = "esp32_client"
TOPIC_IN = "comando"      # Escucha comandos (lo que envía publicador.py)
TOPIC_OUT = "respuesta"    # Publica respuestas (lo que escucha publicador.py)

def conectar_wifi():
    sta = network.WLAN(network.STA_IF)
    sta.active(True)
    sta.connect(secrets.WIFI_SSID, secrets.WIFI_PASS)
    
    print("Conectando WiFi...", end="")
    while not sta.isconnected():
        time.sleep(0.5)
        print(".", end="")
    print("\nWiFi conectado!")
    print("IP:", sta.ifconfig()[0])

def callback(topic, msg):
    mensaje = msg.decode()
    print(f"Received: {mensaje}")
    
    # Responde al publicador
    client.publish(TOPIC_OUT, f"ESP32 received {mensaje}")

# Conectar WiFi
conectar_wifi()

# Conectar MQTT
client = MQTTClient(CLIENT_ID, MQTT_BROKER, MQTT_PORT)
client.set_callback(callback)
client.connect()
client.subscribe(TOPIC_IN.encode())
print(f"Conectado a broker MQTT, escuchando {TOPIC_IN}")

# Bucle principal
while True:
    client.check_msg()  # Revisa si llegó mensaje
    time.sleep(0.1)


# Usando mpremote
#mpremote mip install umqtt.simple

#cd "C:\Program Files\mosquitto"
#.\mosquitto.exe -v