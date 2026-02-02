import webrepl
from microdot import Microdot
from machine import Pin
import ujson

relay = Pin(5, Pin.OUT)
relay_state = False
relay.value(0)

webrepl.start()
app = Microdot()

@app.route('/')
def index(request):
    with open('pagina.html', 'r') as f:
        # return f.read()
        return f.read(), 202, {'Content-Type': 'text/html'}

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