#include <Caipora_2.3_inferencing.h>

#define EIDSP_QUANTIZE_FILTERBANK 0

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"

/*
=========================================================
                CAIPORA 5.0
       IA (2 CLASSES) + SERIAL USB + GATEWAY
=========================================================
*/

// =====================================================
// CONFIGURAÇÕES
// =====================================================

const char* SENSOR_ID = "CAI-01";

float LATITUDE = -4.599722;
float LONGITUDE = -56.846667;

// =====================================================
// CONFIGURAÇÃO DOS PINOS E BARRAMENTO I2S
// =====================================================

#define I2S_WS    33
#define I2S_SD    32
#define I2S_SCK   27
#define I2S_PORT  I2S_NUM_0

#define LED_ALERTA 2

// =====================================================
// VALIDAÇÕES ESTREITAS (THRESHOLDS)
// =====================================================

#define THRESHOLD_MOTOSSERRA 0.80 // Exige 80% ou mais de certeza da IA

// =====================================================
// CONFIGURAÇÃO DE TEMPO DE RECARGA (COOLDOWN)
// =====================================================

unsigned long ultimoAlerta = 0;
const unsigned long cooldown = 10000;

// =====================================================

typedef struct {
    int16_t *buffer;
    uint8_t buf_ready;
    uint32_t buf_count;
    uint32_t n_samples;
} inference_t;

static inference_t inference;

static const uint32_t sample_buffer_size = 1024;

static bool record_status = true;

// =====================================================
// PROTÓTIPOS DAS FUNÇÕES
// =====================================================

static int i2s_init(uint32_t sampling_rate);
static bool microphone_inference_start(uint32_t n_samples);
static bool microphone_inference_record(void);
static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr);
void enviarAlertaSerial(const char* tipo, float confianca);

// =====================================================
// SETUP
// =====================================================

void setup() {

    Serial.begin(115200);

    pinMode(LED_ALERTA, OUTPUT);
    digitalWrite(LED_ALERTA, LOW);

    Serial.println("\n=====================================");
    Serial.println("        CAIPORA 5.0 (2 CLASSES)");
    Serial.println("=====================================");

    if (!microphone_inference_start(EI_CLASSIFIER_RAW_SAMPLE_COUNT)) {
        Serial.println("Erro ao iniciar o microfone I2S");
        while(1);
    }

    Serial.println("Sistema monitorizando a borda.");
}

// =====================================================
// LOOP PRINCIPAL (INFERÊNCIA LOCAL)
// =====================================================

void loop() {

    if (microphone_inference_record()) {

        signal_t signal;
        signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
        signal.get_data = &microphone_audio_signal_get_data;

        ei_impulse_result_t result = { 0 };

        EI_IMPULSE_ERROR r = run_classifier(&signal, &result, false);

        if (r != EI_IMPULSE_OK) {
            Serial.printf("Erro na inferencia: %d\n", r);
            return;
        }

        float p_ambiente = 0;
        float p_motosserra = 0;

        // Mapeamento dinâmico das duas classes do modelo
        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
            const char* label = result.classification[ix].label;
            float valor = result.classification[ix].value;

            if (strcmp(label, "ambiente") == 0)
                p_ambiente = valor;

            else if (strcmp(label, "motosserra") == 0)
                p_motosserra = valor;
        }

        // Output limpo no monitor serial para debug
        Serial.printf("Amb: %.2f | Mot: %.2f\n", p_ambiente, p_motosserra);

        bool podeAlertar = (millis() - ultimoAlerta) > cooldown;

        // =================================================
        // REGRA DE NEGÓCIO: VALIDAÇÃO DA MOTOSSERRA (>= 80%)
        // =================================================
        if (p_motosserra >= THRESHOLD_MOTOSSERRA && p_motosserra > p_ambiente && podeAlertar) {

            Serial.println(">>> ALERTA DE MOTOSSERRA CONFIRMADO <<<");

            digitalWrite(LED_ALERTA, HIGH); // Ativa o sinalizador visual

            enviarAlertaSerial("motosserra", p_motosserra);

            ultimoAlerta = millis();
        }
        else if (p_motosserra < THRESHOLD_MOTOSSERRA) {
            // Mantém o LED apagado se o ruído ambiental predominar
            digitalWrite(LED_ALERTA, LOW);
        }
    }
}

// =====================================================
// ENVIO SERIAL EM FORMATO ESTRUTURADO JSON
// =====================================================

void enviarAlertaSerial(const char* tipo, float confianca) {

    Serial.print("{");

    Serial.print("\"sensor_id\":\"");
    Serial.print(SENSOR_ID);
    Serial.print("\",");

    Serial.print("\"lat\":");
    Serial.print(LATITUDE, 6);
    Serial.print(",");

    Serial.print("\"lng\":");
    Serial.print(LONGITUDE, 6);
    Serial.print(",");

    Serial.print("\"tipo\":\"");
    Serial.print(tipo);
    Serial.print("\",");

    Serial.print("\"confianca\":");
    Serial.print(confianca, 3);
    Serial.print(",");

    Serial.print("\"status\":\"alertando\"");

    Serial.println("}");
}

// =====================================================
// TASK DO FREERTOS PARA CAPTURA DE SINAL VIA DMA
// =====================================================

static void capture_samples(void* arg) {

    const int32_t i2s_bytes_to_read = (uint32_t)arg;
    size_t bytes_read;
    int32_t raw_buffer[sample_buffer_size / 2];

    while (record_status) {

        i2s_read(I2S_PORT, (void*)raw_buffer, i2s_bytes_to_read, &bytes_read, portMAX_DELAY);

        if (bytes_read > 0) {

            for (int i = 0; i < bytes_read / 4; i++) {

                // Deslocamento de bits para calibração de ganho do INMP441
                int16_t sample = (int16_t)(raw_buffer[i] >> 11);

                inference.buffer[inference.buf_count++] = sample;

                if (inference.buf_count >= inference.n_samples) {
                    inference.buf_count = 0;
                    inference.buf_ready = 1;
                }
            }
        }
    }

    vTaskDelete(NULL);
}

// =====================================================
// INICIALIZAÇÃO DOS BUFFERS DE INFERÊNCIA
// =====================================================

static bool microphone_inference_start(uint32_t n_samples) {

    inference.buffer = (int16_t *)malloc(n_samples * sizeof(int16_t));

    if (inference.buffer == NULL)
        return false;

    inference.buf_count = 0;
    inference.n_samples = n_samples;
    inference.buf_ready = 0;

    if (i2s_init(EI_CLASSIFIER_FREQUENCY))
        return false;

    xTaskCreatePinnedToCore(
        capture_samples,
        "Capture",
        1024 * 8,
        (void*)sample_buffer_size,
        10,
        NULL,
        0
    );

    return true;
}

// =====================================================

static bool microphone_inference_record(void) {

    if (inference.buf_ready == 1) {
        inference.buf_ready = 0;
        return true;
    }

    return false;
}

// =====================================================

static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr) {

    numpy::int16_to_float(&inference.buffer[offset], out_ptr, length);
    return 0;
}

// =====================================================
// CONFIGURAÇÃO DOS REGISTOS DO DRIVER I2S DO ESP32
// =====================================================

static int i2s_init(uint32_t sampling_rate) {

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = sampling_rate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 512,
        .use_apll = false
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = -1,
        .data_in_num = I2S_SD
    };

    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    return i2s_set_pin(I2S_PORT, &pin_config);
}
