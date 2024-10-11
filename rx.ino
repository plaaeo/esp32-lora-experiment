#include <ESP32_LoRaWAN.h>
#include <SSD1306.h>
#include "comum.h"

SSD1306 Oled(0x3c, OLED_SDA, OLED_SCL, OLED_RST);

// Usado para armazenar a imagem final.
uint8_t buffer[BUFFER_SIZE];
size_t receivedBytes = 0;

// Executado quando um pacote completo for recebido.
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
    if (size + receivedBytes > BUFFER_SIZE) {
        size = BUFFER_SIZE - receivedBytes;
    }

    memcpy(&buffer[receivedBytes], payload, size);
    receivedBytes = (receivedBytes + size) % BUFFER_SIZE;

    Radio.Sleep();
}

void setup() {
    receivedBytes = 0;

    // Inicializar OLED
    Oled.init();
    
    // Inicializar radio LoRa
    static RadioEvents_t RadioEvents;

    RadioEvents.RxDone = OnRxDone;
    
    Radio.Init( &RadioEvents );
    Radio.SetChannel( RF_FREQUENCY );
    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                                   LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                                   LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   0, true, 0, 0, LORA_IQ_INVERSION_ON, true );
}

void loop() {
    Radio.Rx(0);
}
