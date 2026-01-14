import json, serial, threading, time
from http.server import BaseHTTPRequestHandler, HTTPServer

ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=0.01)
ser.reset_input_buffer()

dados = {
    "acel": 0.0,
    "curva": "Reta",
    "dist": -1.0,
    "estado": "Parado",
    "vel": 0.0
}

velocidade = 0.0
ultimo_tempo = time.time()

def ler_serial():
    global velocidade, ultimo_tempo, dados

    while True:
        try:
            linha = ser.readline().decode().strip()
            if not linha:
                continue

            acel, curva, dist, estado = linha.split(',')
            acel = float(acel)
            dist = float(dist)

            agora = time.time()
            dt = agora - ultimo_tempo
            ultimo_tempo = agora

            velocidade += acel * dt

            # Controle de drift
            if abs(acel) < 0.2:
                velocidade *= 0.98
            if velocidade < 0:
                velocidade = 0
            if velocidade > 60:
                velocidade = 60

            dados.update({
                "acel": acel,
                "curva": curva,
                "dist": dist,
                "estado": estado,
                "vel": velocidade
            })
        except:
            pass

class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/dados":
            self.send_response(200)
            self.send_header("Content-type","application/json")
            self.send_header("Cache-Control","no-store")
            self.end_headers()
            self.wfile.write(json.dumps(dados).encode())
        else:
            self.send_response(200)
            self.send_header("Content-type","text/html")
            self.end_headers()
            self.wfile.write(open("index.html","rb").read())

threading.Thread(target=ler_serial, daemon=True).start()
print("Servidor: http://localhost:8000")
HTTPServer(("0.0.0.0",8000), Handler).serve_forever()
