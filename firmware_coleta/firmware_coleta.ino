#include <driver/i2s.h>

#define I2S_WS    33
#define I2S_SD    32
#define I2S_SCK   27
#define I2S_PORT  I2S_NUM_0

#define SAMPLE_RATE 16000
#define NUM_SAMPLES 16000 // 1 segundo de áudio por bloco

int32_t i2s_buffer[1024];

void init_i2s() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
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
    i2s_set_pin(I2S_PORT, &pin_config);
}

void setup() {
    Serial.begin(115200);
    init_i2s();
}

void loop() {
    // Escuta o comando do script Python para iniciar a gravação
    if (Serial.available() > 0) {
        char comando = Serial.read();
        if (comando == 'G') { // 'G' de Gravar
            
            Serial.println("START_DATA");
            
            size_t bytes_read;
            int amostras_enviadas = 0;

            while (amostras_enviadas < NUM_SAMPLES) {
                i2s_read(I2S_PORT, &i2s_buffer, sizeof(i2s_buffer), &bytes_read, portMAX_DELAY);
                int samples = bytes_read / 4;

                for (int i = 0; i < samples; i++) {
                    // Aplica o deslocamento de bits para calibração do INMP441
                    int16_t sample_16bit = (int16_t)(i2s_buffer[i] >> 11);
                    
                    Serial.print(sample_16bit);
                    amostras_enviadas++;

                    if (amostras_enviadas >= NUM_SAMPLES) {
                        break;
                    }
                    
                    if (i < samples - 1) {
                        Serial.print(",");
                    }
                }
                Serial.println();
            }
            
            Serial.println("END_DATA");
        }
    }
}