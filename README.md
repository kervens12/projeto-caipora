# projeto-caipora
Nó sensor inteligente baseado em ESP32 e TinyML (Edge AI) para a detecção acústica em tempo real de motosserras gerando alertas alertas de desmatamento ilegal.

# 🌲 Caipora — Monitoramento Acústico Florestal com TinyML

O **Caipora** é um nó sensor inteligente de monitoramento ambiental desenvolvido para detectar ameaças de desmatamento ilegal em tempo real. Utilizando **Inteligência Artificial na borda (TinyML)**, o dispositivo processa áudio localmente e identifica assinaturas acústicas de motosserras sem depender de conexão com a nuvem ou internet.

## 🚀 Funcionalidades
* **Inferência Local (Edge AI):** Classificação em tempo real diretamente no microcontrolador ESP32.
* **Detecção de 2 Classes:** Ambiente e Motosserra.
* **Filtro Antiruído:** Lógica estrita de threshold para mitigar alertas falsos.
* **Saída Estruturada:** Geração de alertas automáticos em formato JSON via Serial, pronto para integração com rádio LoRa.

---

## 🛠️ Arquitetura de Hardware e Sinal
* **Microcontrolador:** ESP32 (Dual-Core, FreeRTOS)
* **Sensor de Áudio:** Microfone Digital I2S INMP441
* **Protocolo de Áudio:** I2S com bufferização em RAM via DMA (Direct Memory Access)
* **Tratamento de Sinal:** Aplicação de deslocamento de bits (`>> 11`) para ganho e estabilização de amplitude.

---

## 📊 Inteligência Artificial (MFE + CNN)
O modelo foi treinado utilizando a plataforma **Edge Impulse**, convertendo o áudio bruto em espectrogramas através de **Mel-Frequency Energy (MFE)** e processando-o por uma Rede Neural Convolucional (CNN).

* **Acurácia Global:** 95.7%
* **Sensibilidade da Classe "Motosserra":** 94.1%
* **Estabilidade da Classe "Ambiente":** 97.1% (Garantia contra disparos falsos)
* **F1-Score:** 0.96

### Parâmetros do Impulso:
* **Tamanho da Janela:** 1.000 ms
* **Aumento da Janela (Passo):** 400 ms (*Data Augmentation*)
* **Frequência de Amostragem:** ~16.000 Hz (15997 Hz real)

---

## ⚙️ Configuração do Firmware (Regras de Negócio)
Para otimizar o consumo energético do nó sensor e garantir a confiabilidade dos alertas em campo, o firmware aplica as seguintes lógicas:
1.  **Threshold Estrito ($\ge$ 0.80):** O sistema exige 80% ou mais de certeza da IA para ativar o estado de alerta, bloqueando ruídos comuns da floresta (vento, pássaros).
2.  **Cooldown (10s):** Tempo de respiro após uma detecção válida para evitar a inundação de pacotes repetidos na rede de rádio.

### Estrutura do Alerta JSON gerado:
```json
{
  "sensor_id": "CAI-01",
  "lat": -4.599722,
  "lng": -56.846667,
  "tipo": "motosserra",
  "confianca": 0.845,
  "status": "alertando"
}

---

## 📥 Como Rodar o Projeto (Firmware)

Para compilar o código do ESP32 (`firmware.ino`), você precisa importar o modelo de Inteligência Artificial que está compactado na pasta deste repositório.

1. Baixe o arquivo `.zip` da IA que está localizado na pasta `/libs` deste projeto.
2. Abra a **Arduino IDE**.
3. No menu superior, vá em: **Esboço** -> **Incluir Biblioteca** -> **Adicionar Biblioteca .ZIP...** (Sketch -> Include Library -> Add .ZIP Library).
4. Selecione o arquivo `.zip` que você baixou.
5. Pronto! Agora o seu ambiente possui a biblioteca `<Caipora_2.3_inferencing.h>` instalada e pronta para compilar o código para o ESP32.
