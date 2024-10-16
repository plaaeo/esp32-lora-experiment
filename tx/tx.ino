#include "../comum.h"
#include "imagem.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LoRaWan_APP.h>
#include <Wire.h>
#include <esp_random.h>

#define STRINGIFY(...) #__VA_ARGS__

// Define a imagem a ser transmitida, seu comprimento (w) e altura (h)
#define IMAGEM logoLab
#define IMAGEM_W 64
#define IMAGEM_H 64

void OnTxDone(void);
void OnTxTimeout(void);

button_t button(BUTTON);

Adafruit_SSD1306 Oled(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RST);

// Usado para esperar o fim da transmissão de um pacote.
bool canTx = false;

enum { sIdle, sBroadcasting, sTransmitting } state = sIdle;

// Inicializa o pacote de broadcast com um nome aleatório e as dimensões da
// imagem inicial.
broadcast_t msgBroadcast{esp_random(), IMAGEM_W, IMAGEM_H};

// Inicializa o pacote de imagem com os dados relevantes a imagem.
image_data_t msgImage{msgBroadcast.name, 0, IMAGEM, sizeof(IMAGEM)};

void setup() {
    Serial.begin(115200);
    Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

    // Configura o botão PRG
    button.setup();

    // Inicializar OLED
    Wire.begin(OLED_SDA, OLED_SCL);

    if (!Oled.begin(SSD1306_SWITCHCAPVCC, 0x3c, true, false)) {
        Serial.println("Falha na inicializacao do OLED");
        while (true)
            ;
    }

    Oled.clearDisplay();
    Oled.setTextColor(WHITE);

    // Inicializar radio LoRa
    static RadioEvents_t RadioEvents;

    RadioEvents.TxDone = OnTxDone;
    RadioEvents.TxTimeout = OnTxTimeout;

    Radio.Init(&RadioEvents);
    Radio.SetChannel(RF_FREQUENCY);

    Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                      LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                      LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON, true, 0,
                      0, LORA_IQ_INVERSION_ON, 3000);
}

// Envia o payload broadcast via LoRa.
void enviarBroadcast() {
    static uint8_t payload[sizeof(broadcast_t) + 1];

    uint8_t length = msgBroadcast.toPayload(payload);
    Serial.printf("Enviando broadcast (%d bytes)... ", length);

    Radio.Send(payload, length);
}

// Envia o pacote de imagem atual.
void enviarImagem() {
    static uint8_t payload[MAX_PAYLOAD_SIZE];

    uint16_t off = msgImage.offset, fullLen = msgImage.length;

    uint8_t length = msgImage.toPayloadAutoOffset(payload);
    Serial.printf("Enviando imagem (%d bytes, %d/%d)... ", length, off,
                  fullLen);

    Radio.Send(payload, length);
}

void loop() {
    button.update();

    // Mudar o modo dependendo da ação do botão PRG
    if (button.wasShortPressed()) {
        state = state == sBroadcasting ? sTransmitting : sBroadcasting;
    } else if (button.wasLongPressed()) {
        state = sIdle;
    }

    // Transmitir algum pacote dependendo do estado do dispositivo
    if (state != sIdle && canTx) {
        delay(1000);

        switch (state) {
        case sBroadcasting:
            enviarBroadcast();
            break;
        case sTransmitting:
            enviarImagem();
            break;
        }

        canTx = false;
    }

    Radio.IrqProcess();

    Oled.clearDisplay();

    // Desenhar status do dispositivo
    char nome[] = "Dispositivo 00000000";
    hexName(nome + 12, msgBroadcast.name);

    int16_t x, y, w, h;
    Oled.getTextBounds(nome, 0, 0, &x, &y, &w, &h);
    Oled.setCursor((OLED_WIDTH - w) / 2, (OLED_HEIGHT - h) / 2);
    Oled.println(nome);

    // Desenhar no monitor OLED dependendo do estado
    switch (state) {
    case sIdle:
    case sBroadcasting:
        const char *label =
            (state == sBroadcasting) ? "(broadcasting)" : "(ocioso)";

        Oled.getTextBounds(label, 0, 0, &x, &y, &w, &h);
        Oled.setCursor((OLED_WIDTH - w) / 2, (OLED_HEIGHT + h) / 2);
        Oled.println(label);
        break;

    case sTransmitting:
        const char label[256];

        sprintf(label, "(transmitindo \"%s\", %dx%d)", STRINGIFY(IMAGEM),
                msgBroadcast.width, msgBroadcast.height);

        Oled.getTextBounds(label, 0, 0, &x, &y, &w, &h);
        Oled.setCursor((OLED_WIDTH - w) / 2, (OLED_HEIGHT + h) / 2);
        Oled.println(label);

        sprintf(label, "%d/%d bytes", msgImage.offset, msgImage.length);

        Oled.getTextBounds(label, 0, 0, &x, &y, &w, &h);
        Oled.setCursor((OLED_WIDTH - w) / 2, (OLED_HEIGHT / 2) + h);
        Oled.println(label);
        break;
    }

    Oled.display();
}

// Executado quando um pacote completo for enviado.
void OnTxDone(void) {
    Serial.println("enviado!");
    canTx = true;
}

void OnTxTimeout(void) {
    Serial.println("timeout!");
    Radio.Sleep();
    canTx = true;
}