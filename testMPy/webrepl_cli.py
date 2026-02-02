import socket
import threading
import sys

def webrepl_windows():
    s = socket.socket()
    s.connect(("192.168.1.179", 8266))
    
    # 1. Handshake
    hs = "GET / HTTP/1.1\r\nHost: 192.168.4.1:8266\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: AQIDBAUGBwgJCgsMDQ4PEA==\r\nSec-WebSocket-Version: 13\r\n\r\n"
    s.send(hs.encode())
    s.recv(1024)
    
    # 2. Contraseña FIJA - CAMBIA "1234" por tu contraseña real
    passwd = "1234"
    s.send(b'\x81' + bytes([len(passwd)]) + passwd.encode())
    
    # 3. Hilo para recibir
    def recibir():
        while True:
            try:
                data = s.recv(1024)
                if data and len(data) > 2:
                    texto = data[2:].decode('utf-8', errors='ignore')
                    sys.stdout.write(texto)
                    sys.stdout.flush()
            except:
                break
    
    threading.Thread(target=recibir, daemon=True).start()
    
    # 4. Comandos
    print(">>> CONECTADO. Escribe comandos (ej: print('hola')):")
    while True:
        cmd = input()
        if cmd == "salir":
            break
        s.send(b'\x81' + bytes([len(cmd)+2]) + (cmd + "\r\n").encode())

if __name__ == "__main__":
    webrepl_windows()