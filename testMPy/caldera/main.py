import webrepl
from microdot import Microdot
from machine import Pin
import ujson
import gc

relay = Pin(5, Pin.OUT)
relay_state = False
relay.value(0)

webrepl.start()
app = Microdot()

@app.route('/')
def index(request):
    with open('pagina_min.html', 'r') as f:
        html = f.read()
    
    respuesta = html, 200, {'Content-Type': 'text/html'}
    
    # Liberación manual
    del html  # Elimina referencia
    gc.collect()  # Fuerza limpieza inmediata
    
    return respuesta

@app.route('/status', methods=['GET'])
def estado(request):
    global relay_state
    status = "on" if relay_state else "off"
    return ujson.dumps({'status': status})

@app.route('/control', methods=['POST'])
def control_handler(request):
    global relay_state
    if request.method == 'POST':
        if request.body:
            data = ujson.loads(request.body.decode())
            # Check if JSON has 'status' field
            if 'status' in data:
                status = data['status']
                # Respond based on received status
                if status == 'on':
                    relay_state = True
                    relay.value(1)
                    return ujson.dumps({'status': 'on', 'message': 'Device turned ON'})
                elif status == 'off':
                    relay_state = False
                    relay.value(0)
                    return ujson.dumps({'status': 'off', 'message': 'Device turned OFF'})
            return ujson.dumps({'received': data})
    return {'error': 'resource not found'}, 404    

app.run(port=80)