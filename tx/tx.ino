#include "../comum.h"
#include "imagem.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LoRaWan_APP.h>
#include <Wire.h>
#include <esp_random.h>

#define SENSOR_TEMP 18

float steinhart(int Vo, float A, float B, float C) {
  Serial.printf("%d\n", Vo);

  float Resistance = (40960000 / (float)Vo) - 10000; // for pull-up configuration
  float R2 = Resistance;
  
  float logR2 = log(R2); // Pre-Calcul for Log(R2)
  float T = (1.0 / (A + B * logR2 + C * logR2 * logR2 * logR2)); 
  T = T - 273.15; // convert Kelvin to °C
  
  return T;
}

float readThermistor() {
  int Vo = analogRead(SENSOR_TEMP);
  // float temperatura = steinhart(Vo, 2.198064079e-3, 0.7738933847e-4, 5.149159750e-7);
  float temperatura = steinhart(Vo, 0.001129148, 0.000234125, 0.0000000876741);
  return temperatura;
}

// Define a imagem a ser transmitida, seu comprimento (w) e altura (h)
#define IMAGEM logoUfam
#define IMAGEM_W 128
#define IMAGEM_H 64

void OnTxDone(void);
void OnTxTimeout(void);

button_t button(BUTTON);

Adafruit_SSD1306 Oled(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RST);

// Usado para esperar o fim da transmissão de um pacote.
bool canTx = true;

enum { sIdle, sBroadcasting, sTransmitting } state = sIdle;

// Inicializa o pacote de broadcast com um nome aleatório e as dimensões da
// imagem inicial.
broadcast_t msgBroadcast{esp_random(), 0, 0, IMAGEM_W, IMAGEM_H};

// Inicializa o pacote de imagem com os dados relevantes a imagem.
image_data_t msgImage{msgBroadcast.name, 0, IMAGEM, sizeof(IMAGEM)};

void setup() {
    Serial.begin(115200);
    Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

    analogReadResolution(12);

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
    Oled.setRotation(2);

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
bool enviarBroadcast() {
    static uint32_t lastPacket = 0;
    uint32_t now = millis() + 4000;

    if (now - lastPacket < 4000) {
        return false;
    }

    static uint8_t payload[sizeof(broadcast_t) + 1];
    
    msgBroadcast.temperature = readThermistor();

    uint8_t length = msgBroadcast.toPayload(payload);
    Serial.printf("Enviando broadcast (%d bytes)... ", length);

    Radio.Send(payload, length);
    lastPacket = now;

    return true;
}

// Envia o pacote de imagem atual.
bool enviarImagem() {
    static uint32_t lastImage = 0;
    static bool wasLastPacket = false;
    uint32_t now = millis() + 2000;

    if (now - lastImage < 2000) {
        return false;
    }

    // Reseta o offset da imagem enviada para reenviá-la
    if (wasLastPacket) {
        // sgImage.offset = 0;
    }

    static uint8_t payload[MAX_PAYLOAD_SIZE];

    uint16_t off = msgImage.offset, fullLen = msgImage.length;

    uint8_t length = msgImage.toPayloadAutoOffset(payload, &wasLastPacket);
    Serial.printf("Enviando imagem (%d bytes, %d/%d)... ", length, off,
                  fullLen);

    Radio.Send(payload, length);
    lastImage = now;

    return true;
}

void loop() {
    button.update();

    // Mudar o modo dependendo da ação do botão PRG
    if (button.wasShortPressed()) {
        state = state == sBroadcasting ? sTransmitting : sBroadcasting;
        msgImage.offset = 0;
    } else if (button.wasLongPressed()) {
        state = sIdle;
    }

    // Transmitir algum pacote dependendo do estado do dispositivo
    if (canTx) {
        switch (state) {
        case sBroadcasting:
            canTx = !enviarBroadcast();
            break;
        case sTransmitting:
            canTx = !enviarImagem();
            break;
        }
    }

    Radio.IrqProcess();

    Oled.clearDisplay();

    // Desenhar status do dispositivo
    char nome[] = "Dispositivo 00000000";
    hexName(nome + 12, msgBroadcast.name);

    int16_t x, y;
    uint16_t w, h;
    Oled.getTextBounds(nome, 0, 0, &x, &y, &w, &h);
    Oled.setCursor((OLED_WIDTH - w) / 2, (OLED_HEIGHT - h) / 2);
    Oled.println(nome);

    // Desenhar no monitor OLED dependendo do estado
    switch (state) {
    case sIdle:
    case sBroadcasting: {
        const char *label =
            (state == sBroadcasting) ? "(broadcasting)" : "(ocioso)";

        Oled.getTextBounds(label, 0, 0, &x, &y, &w, &h);
        Oled.setCursor((OLED_WIDTH - w) / 2, (OLED_HEIGHT + h) / 2);
        Oled.println(label);
        break;
    }

    case sTransmitting: {
        char label[256];

        sprintf(label, "(transmitindo, %dx%d)", msgBroadcast.width,
                msgBroadcast.height);

        Oled.getTextBounds(label, 0, 0, &x, &y, &w, &h);
        Oled.setCursor((OLED_WIDTH - w) / 2, (OLED_HEIGHT + h) / 2);
        Oled.println(label);

        sprintf(label, "%d/%d bytes", msgImage.offset, msgImage.length);

        Oled.getTextBounds(label, 0, 0, &x, &y, &w, &h);
        Oled.setCursor((OLED_WIDTH - w) / 2, ((OLED_HEIGHT + h) / 2) + h);
        Oled.println(label);
        break;
    }
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