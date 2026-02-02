import uasyncio as asyncio
from machine import Pin
from otatools import ota_update

# --- Rele ---
relay = Pin(5, Pin.OUT)
relay_state = False
relay.value(0)


# --- HTML ---
def get_html():
    status = "ACTIVO" if relay_state else "INACTIVO"
    btn_text = "DESACTaVAR RELE" if relay_state else "ACTaVAR RELE"
    action = "/off" if relay_state else "/on"
    color = "#e74c3c" if relay_state else "#2ecc71"
    return f"""<!DOCTYPE html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body{{font-family:sans-serif;text-align:center;background:#1a1a2e;color:white;padding-top:50px}}
.card{{background:#16213e;padding:30px;border-radius:20px;display:inline-block}}
.status{{font-size:1.5em;margin:20px 0;color:{color}}}
.btn{{padding:15px 30px;font-size:1.2rem;color:white;text-decoration:none;border-radius:10px}}
</style></head><body>
<div class="card">
<h1>NodeMCU Rele</h1>
<div class="status">ESTADO: {status}</div>
<a href="{action}" class="btn">{btn_text}</a>
</div></body></html>"""

# --- Servidor HTTP usando uasyncio ---
async def http_handler(reader, writer):
    global relay_state
    try:
        req = await reader.read(1024)
        req = req.decode()
        if "GET /on" in req:
            relay_state = True
            relay.value(1)
        elif "GET /off" in req:
            relay_state = False
            relay.value(0)     
        elif "GET /restart" in req:
            ota_update()
        resp = "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n" + get_html()
        await writer.awrite(resp)
    except Exception as e:
        print("Error HTTP:", e)
    finally:
        await writer.aclose()

async def run_server():
    server = await asyncio.start_server(http_handler, "0.0.0.0", 80)
    while True:
        await asyncio.sleep(1)

# --- Loop principal ---
asyncio.run(run_server())
