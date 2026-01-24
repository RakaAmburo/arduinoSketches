import network
import socket

# Conectar a Wi-Fi
ssid = "MIWIFI_mcCb"
password = "4ERT3RhP"

wlan = network.WLAN(network.STA_IF)
wlan.active(True)
wlan.connect(ssid, password)

while not wlan.isconnected():
    pass

print("Conectado, IP:", wlan.ifconfig()[0])

# Servidor HTTP simple
addr = socket.getaddrinfo('0.0.0.0', 80)[0][-1]
s = socket.socket()
s.bind(addr)
s.listen(1)

print("Servidor HTTP escuchando...")

while True:
    cl, addr = s.accept()
    request = cl.recv(1024)
    response = b"HTTP/1.0 200 OK\r\nContent-Type: text/plain\r\n\r\nHola"
    cl.send(response)
    cl.close()