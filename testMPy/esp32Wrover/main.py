import network, socket, camera, time

# --- Wi-Fi ---
w = network.WLAN(network.STA_IF)
w.active(True)
w.connect("TU_WIFI","TU_PASS")
while not w.isconnected(): time.sleep(0.2)
ip = w.ifconfig()[0]
print("IP:", ip)

camera.init()  # inicializa la cámara

# --- HTML con botón ---
html = """<html>
<body>
<h1>ESP32-CAM</h1>
<form action="/foto">
<button type="submit">Tomar Foto</button>
</form>
<img src="/foto">
</body>
</html>
"""

# --- Servidor HTTP ---
s = socket.socket()
s.bind(("", 80))
s.listen(1)

frame = None

while True:
    c, addr = s.accept()
    req = c.recv(1024)
    req = req.decode()
    if "GET /foto" in req:
        frame = camera.capture()  # toma foto JPEG
        c.send(b"HTTP/1.0 200 OK\r\nContent-Type: image/jpeg\r\n\r\n" + frame)
    else:
        c.send("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n".encode() + html.encode())
    c.close()
