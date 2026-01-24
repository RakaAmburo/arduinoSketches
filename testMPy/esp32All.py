import network, bluetooth, uasyncio as asyncio, time
from bluetooth import BLE

# --- Wi-Fi ---
w = network.WLAN(network.STA_IF)
w.active(True)
w.connect("MIWIFI_mcCb","4ERT3RhP")
while not w.isconnected(): time.sleep(0.2)
ip = w.ifconfig()[0]
print("IP del ESP32:", ip)

# --- BLE ---
ble = BLE()
ble.active(True)
name = b'ESP32-BLE'
ble.gap_advertise(100_000, b'\x02\x01\x06'+bytes((len(name)+1,0x09))+name)

# --- HTTP con uasyncio ---
async def handle(reader, writer):
    t = time.localtime()
    msg = f"Hola desde ESP32 - {t[4]:02d}:{t[5]:02d}"
    writer.write(f"HTTP/1.0 200 OK\r\nContent-Type: text/plain\r\n\r\n{msg}".encode())
    await writer.drain()
    await writer.aclose()

async def main():
    server = await asyncio.start_server(handle, "0.0.0.0", 80)
    await server.wait_closed()

asyncio.run(main())
