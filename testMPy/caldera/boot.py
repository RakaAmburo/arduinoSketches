import network, time
import secrets

w = network.WLAN(network.STA_IF)
w.active(True)
w.connect(secrets.WIFI_SSID, secrets.WIFI_PASS)
while not w.isconnected(): time.sleep(0.2)
