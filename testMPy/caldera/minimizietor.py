import re
import os

ruta = os.path.join(os.path.dirname(__file__), 'pagina.html')
with open(r'C:\repos\arduinoSketches\testMPy\caldera\pagina.html', 'r') as f:
    html = f.read()

html = re.sub(r'<!--.*?-->|\s+', ' ', html, flags=re.DOTALL)
html = re.sub(r'>\s+<', '><', html).strip()

with open('pagina_min.html', 'w') as f:
    f.write(html)