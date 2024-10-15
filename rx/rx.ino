#include <LoRaWan_APP.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "../comum.h"

Adafruit_SSD1306 Oled(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RST);

uint8_t ack[1] = { kAck };

// Usado para armazenar a imagem final.
uint8_t buffer[BUFFER_SIZE];
size_t receivedBytes = 0;

bool receiving = false;
enum {
  sConfiguring,
  sAcking,
  sReceiving,
} state;

// Contém a configuração das funções a serem chamadas pela biblioteca da Heltec LoRa
static RadioEvents_t RadioEvents;

// Configuração inicial da imagem
packet_config_t config = { -1, 0, 0 };

// Executado quando um pacote completo for enviado.
void OnTxDone(void) {
  Serial.println("ACK transmitido");
  state = sReceiving;
  receiving = false;
}

void OnTxTimeout(void) {
  Radio.Sleep();
  receiving = false;
}

// Executado quando qualquer pacote for recebido.
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  if (size == 0) {
    Serial.println("Pacote vazio recebido, ignorando...");
    return;
  }

  switch (payload[0]) {
    case kConfig:
      if (state != sConfiguring) {
        Serial.printf("Configuracao ignorada...\n");
        break;
      }

      if (size == sizeof(packet_config_t) + 1) {
        memcpy(&config, payload + 1, size - 1);

        // Erro caso estejamos em versões diferentes
        if (config.version != PROTO_VERSION) {
          Serial.printf("Conexao estabelecida para protocolo com versao diferente (nosso = %d, deles = %d)\n", PROTO_VERSION, config.version);
          config.version = -1;
        } else {
          Serial.printf("Conexao estabelecida! Imagem %d x %d a ser recebida...\n", config.width, config.height);
          state = sAcking;
        }
      }
      break;

    case kData:
      size -= 1;
      payload += 1;

      if (size + receivedBytes > BUFFER_SIZE) {
        size = BUFFER_SIZE - receivedBytes;
      }

      memcpy(&buffer[receivedBytes], payload, size);

      receivedBytes = (receivedBytes + size) % (config.width * config.height / 8);

      Oled.clearDisplay();
      Oled.drawBitmap(0, 0, (const uint8_t *)buffer, config.width, config.height, WHITE);
      Oled.display();
      break;

    default:
      Serial.println("Tipo inválido de pacote recebido.");
      break;
  }

  Radio.Sleep();

  receiving = false;
}

void setup() {
  Serial.begin(115200);
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  receivedBytes = 0;
  receiving = false;

  // Inicializar OLED
  Wire.begin(OLED_SDA, OLED_SCL);

  if (!Oled.begin(SSD1306_SWITCHCAPVCC, 0x3c, true, false)) {
    Serial.println("Falha na inicializacao do OLED");
    while (true)
      ;
  }

  Oled.clearDisplay();

  // Inicializar radio LoRa
  static RadioEvents_t RadioEvents;

  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.RxDone = OnRxDone;

  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);

  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, LORA_IQ_INVERSION_ON, 3000);

  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);
}

void loop() {
  if (!receiving) {
    delay(1000);
    switch (state) {
      case sConfiguring:
        Serial.println("Aguardando configuração...");
        Radio.Rx(0);
        break;

      case sReceiving:
        Serial.println("Recebendo...");
        Radio.Rx(0);
        break;

      case sAcking:
        delay(1000);
        Serial.println("Enviando ACK...");
        Radio.Send(ack, 1);
        break;
    }
    receiving = true;
  }

  Radio.IrqProcess();
}
