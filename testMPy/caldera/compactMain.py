import uasyncio as asyncio
from machine import Pin
import webrepl, urequests, os, machine, time

# --- Rele ---
relay = Pin(5, Pin.OUT)
relay_state = False
relay.value(0)

# --- HTML ---
def get_html():
    s = "ACTIVO" if relay_state else "INACTIVO"
    b = "DESACTIVAR" if relay_state else "ACTIVAR"
    a = "/off" if relay_state else "/on"
    c = "#e74c3c" if relay_state else "#2ecc71"
    return f"""HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n
<html><body>
<h1>Rele</h1>
<div style="color:{c}">Estado: {s}</div>
<a href="{a}">{b}</a>
</body></html>"""

# --- OTA simple ---
URL = "http://192.168.1.157:8000/main.py"
def ota_update(url=URL):
    r = urequests.get(url)
    code = r.text
    r.close()
    with open("main.py.tmp","w") as f: f.write(code)
    os.remove("main.py") if "main.py" in os.listdir() else None
    os.rename("main.py.tmp","main.py")
    machine.reset()

# --- HTTP server async ---
async def handler(reader, writer):
    global relay_state
    req = (await reader.read(1024)).decode()
    if "GET /on" in req: relay_state, relay.value(1) = True, 1
    if "GET /off" in req: relay_state, relay.value(0) = False, 0
    await writer.awrite(get_html())
    await writer.aclose()

async def main():
    webrepl.start()  # arrancar WebREPL
    await asyncio.start_server(handler,"0.0.0.0",80)
    while True: await asyncio.sleep(1)

asyncio.run(main())
