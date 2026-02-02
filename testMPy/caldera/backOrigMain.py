import network, socket, time, machine, urequests, os
from machine import Pin
import secrets

# --- Rele ---
relay = Pin(5, Pin.OUT)   # D1 = GPIO5
relay_state = False
relay.value(0)


URL = "http://192.168.1.157:8000/main.py"
def ota_update(url=URL):
    try:
        print("Descargando main.py...")
        r = urequests.get(url)
        code = r.text
        r.close()

        # Verificamos que el contenido no esté vacío
        if not code.strip():
            print("Error: archivo vacío")
            return False

        # Guardamos en main.py temporalmente
        tmp_file = "main_tmp.py"
        with open(tmp_file, "w") as f:
            f.write(code)

        # Confirmamos tamaño antes de reemplazar
        size = os.stat(tmp_file)[6]
        print("Tamaño descargado:", size)

        # Reemplazamos main.py
        os.remove("main.py") if "main.py" in os.listdir() else None
        os.rename(tmp_file, "main.py")
        print("main.py reemplazado correctamente.")

        # Reinicio para correr la nueva versión
        time.sleep(0.5)
        print("Reiniciando ESP...")
        machine.reset()
        return True

    except Exception as e:
        print("Error OTA:", e)
        return False

# --- HTML ---
def get_html():
    status = "ACTIVO" if relay_state else "INACTIVO"
    btn_text = "DESACTIVAR 1 RELE" if relay_state else "ACTIVAR 1 RELE"
    action = "/off" if relay_state else "/on"
    color = "#e74c3c" if relay_state else "#2ecc71"

    return f"""<!DOCTYPE html>
<html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body{{font-family:sans-serif;text-align:center;background:#1a1a2e;color:white;padding-top:50px}}
.card{{background:#16213e;padding:30px;border-radius:20px;display:inline-block}}
.status{{font-size:1.5em;margin:20px 0;color:{color}}}
.btn{{padding:15px 30px;font-size:1.2rem;color:white;text-decoration:none;border-radius:10px}}
</style></head>
<body>
<div class="card">
<h1>NodeMCU Rele</h1>
<div class="status">ESTADO: {status}</div>
<a href="{action}" class="btn">{btn_text}</a>
</div>
</body></html>"""

# --- Server ---
s = socket.socket()
s.bind(("", 80))
s.listen(1)

while True:
    c, _ = s.accept()
    req = c.recv(1024).decode()

    if "GET /on" in req:
        relay_state = True
        relay.value(1)
    elif "GET /off" in req:
        relay_state = False
        relay.value(0)

    c.send("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n")
    c.send(get_html())
    c.close()
