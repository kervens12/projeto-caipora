import serial
import requests
import time
import json
import sys

# ==============================================================================
# CONFIGURAÇÕES DE AUTOMAÇÃO
# ==============================================================================
PORTA_COM = 'COM13' 
BAUD_RATE = 115200
API_KEY = "ei_6b824bdc39fd76a11fd287358ca97e96e4dc263de7faaec8"

LABEL_ATUAL = "motosserra"     # "ambiente" ou "motosserra"
QUANTIDADE_AMOSTRAS = 20     
INTERVALO_SEGUNDOS = 4       
# ==============================================================================

try:
    print(f"Conectando ao ESP32 na porta {PORTA_COM}...")
    ser = serial.Serial(PORTA_COM, BAUD_RATE, timeout=5)
    time.sleep(3) 

    print(f"\n🚀 PIPELINE CONFIGURADO: {QUANTIDADE_AMOSTRAS} amostras de '{LABEL_ATUAL.upper()}'")
    
    for i in range(1, QUANTIDADE_AMOSTRAS + 1):
        ser.reset_input_buffer()

        print("\n" + "="*50)
        print(f" 🎯 [AMOSTRA {i} de {QUANTIDADE_AMOSTRAS}] - Prepare o som...")
        print("="*50)
        
        for segundos in range(3, 0, -1):
            print(f"      [ {segundos} ]...")
            time.sleep(1)

        sys.stdout.write('\a') # Bip no computador
        sys.stdout.flush()
        print("\n🔴 >>> GRAVANDO AGORA! <<< 🔴\n")
        
        ser.write(b'G\n')
        ser.flush()

        amostras = []
        gravando = False
        start_time = time.time()

        while time.time() - start_time < 10:
            if ser.in_waiting > 0:
                linha = ser.readline().decode('utf-8', errors='ignore').strip()
                
                if "START_DATA" in linha:
                    gravando = True
                    continue
                if "END_DATA" in linha:
                    break
                if gravando and len(linha) > 0:
                    partes = linha.split(',')
                    for parte in partes:
                        parte = parte.strip()
                        if parte.isdigit() or (parte.startswith('-') and parte[1:].isdigit()):
                            amostras.append(int(parte))

        total_amostras = len(amostras)
        print(f"-> Captura finalizada. Amostras lidas: {total_amostras}")

        if total_amostras > 1000:
            payload = {
                "protected": {"ver": "v1", "alg": "HS256", "iat": time.time()},
                "signature": "0000000000000000000000000000000000000000000000000000000000000000",
                "payload": {
                    "device_name": "ESP32_Caipora",
                    "device_type": "ESP32",
                    "interval_ms": 0.0625, 
                    "sensors": [{ "name": "audio", "units": "wav" }],
                    "values": amostras
                }
            }

            headers = {
                "x-api-key": API_KEY,
                "x-label": LABEL_ATUAL,
                "x-file-name": f"{LABEL_ATUAL}_auto_{int(time.time())}.wav",
                "Content-Type": "application/json"
            }
            
            r = requests.post("https://ingestion.edgeimpulse.com/api/training/data", headers=headers, data=json.dumps(payload))
            print(f"✅ Upload concluído (Status {r.status_code})")
        else:
            print("⚠️ [ERRO] Nenhuma amostra válida capturada.")

        if i < QUANTIDADE_AMOSTRAS:
            time.sleep(INTERVALO_SEGUNDOS)

    ser.close()
    print("\n🎉 PROCESSO CONCLUÍDO COM SUCESSO!")

except Exception as e:
    print(f"\n💥 Erro operacional: {e}")
