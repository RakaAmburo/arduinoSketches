import machine, time, os, urequests

# --- OTA ---
URL = "http://192.168.1.157:8000/main.py"
def ota_update(url=URL):
    try:
        r = urequests.get(url)
        code = r.text
        r.close()
        if not code.strip():
            print("Error: archivo vacío")
            return False
        tmp_file = "main_tmp.py"
        with open(tmp_file,"w") as f:
            f.write(code)
        size = os.stat(tmp_file)[6]
        os.remove("main.py") if "main.py" in os.listdir() else None
        os.rename(tmp_file,"main.py")
        print("main.py reemplazado:", size, "bytes")
        time.sleep(0.5)
        machine.reset()
        return True
    except Exception as e:
        print("Error OTA:", e)
        return False