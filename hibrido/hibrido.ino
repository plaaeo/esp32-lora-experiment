#include "comum.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LoRaWan_APP.h>
#include <Wire.h>
#include <cstdint>
#include <esp_random.h>
#include <heltec.h>

button_t button(BUTTON);
Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RST);

// Gera um nome aleatório para este dispositivo
uint32_t name = esp_random();

// Indica a função deste dispositivo (transmissor ou receptor)
enum { kReceiver, kTransmitter } role = kReceiver;

// Indica se o módulo LoRa está ocupado
bool isLoraBusy = false;

// Executado ao receber um payload pelo LoRa
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
    if (Serial)
        Serial.printf("[ rx %d bytes, %d dBm RSSI, %d dB SNR ]\n", size, rssi,
                      snr);

    isLoraBusy = false;
    Radio.Sleep();
}

// Executado ao enviar um payload pelo LoRa com sucesso
void OnTxDone() {
    if (Serial)
        Serial.printf("[ tx done ]\n");

    isLoraBusy = false;
    Radio.Sleep();
}

// Executado quando ocorre um timeout no envio do payload
void OnTxTimeout() {
    if (Serial)
        Serial.printf("[ tx timeout ]\n");

    isLoraBusy = false;
    Radio.Sleep();
}

void setup() {
    Serial.begin(115200);
    Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

    // Configura o botão PRG
    button.setup();

    // Inicializar OLED
    Wire.begin(OLED_SDA, OLED_SCL);

    if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3c, true, false)) {
        Serial.println("Falha na inicializacao do OLED");

        while (true) {
        };
    }

    oled.setRotation(180);

    // Inicializar LoRa
    static RadioEvents events;

    events.RxDone = OnRxDone;
    events.TxDone = OnTxDone;
    events.TxTimeout = OnTxTimeout;

    Radio.Init(&events);
    Radio.SetChannel(RF_FREQUENCY);

    Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                      LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                      LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON, true, 0,
                      0, LORA_IQ_INVERSION_ON, 3000);

    Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                      LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                      LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON, 0, true,
                      0, 0, LORA_IQ_INVERSION_ON, true);
}

void updateLora() {
    Radio.IrqProcess();

    if (isLoraBusy)
        return;

    if (role == kReceiver) {
        Radio.Rx(0);
        isLoraBusy = true;
    } else if (role == kTransmitter) {
        // TODO: Transmit current temperature
    }
}

enum alignment_t { kStart, kCenter, kEnd };

void drawAlignedText(const char *text, int16_t x, int16_t y,
                     alignment_t horizontal = alignment_t::kStart,
                     alignment_t vertical = alignment_t::kStart) {
    int16_t _x, _y;
    uint16_t w, h;

    oled.getTextBounds(text, 0, 0, &_x, &_y, &w, &h);

    // Alinhar a posição do texto horizontalmente, de acordo com a regra dada
    switch (horizontal) {
    case kCenter:
        x += (OLED_WIDTH - w) / 2;
        break;
    case kEnd:
        x = OLED_WIDTH - w - x;
        break;
    }

    // Alinhar a posição do texto verticalmente, de acordo com a regra dada
    switch (vertical) {
    case kCenter:
        y += (OLED_HEIGHT - h) / 2;
        break;
    case kEnd:
        y = OLED_WIDTH - h - y;
        break;
    }

    oled.setCursor(x, y);
    oled.println(text);
}

void updateGui() {
    static enum { kSettings } currentGui;
    oled.clearDisplay();
}

void loop() {
    button.update();

    updateLora();
    updateGui();
}
