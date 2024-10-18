#include <LoRaWan_APP.h>
#include <Wire.h>
#include <esp_random.h>
#include <heltec.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#include "comum.h"

button_t button(BUTTON);

// Gera um nome aleatório para este dispositivo
uint32_t name = esp_random();

// Código específico para o receptor, que não se encaixa nos outros namespaces abaixo
namespace rx {
    #define MAX_RECORDS 32

    struct record_t {
        // Contem o ultimo broadcast deste transmissor.
        broadcast_t latest;

        // Contem o tempo em que este broadcast foi recebido.
        uint32_t lastBroadcastMillis;
    } records[MAX_RECORDS] = {};

    uint8_t recordsLength = 0;
    uint8_t selectedRecord = 0;

    // Remove transmissores ociosos da lista
    void purgeRecords() {
        uint32_t now = millis();

        // Invalidar transmissores inativos a mais de 10 segundos
        uint8_t invalidated = 0;

        for (uint8_t i = 0; i < recordsLength; i++) {
            record_t *record = &records[i];

            if (now - record->lastBroadcastMillis > 10000) {
                Serial.printf("(transmissor %x removido)\n", record->latest.name);
                record->latest.name = 0;
                invalidated++;
            }
        }

        if (invalidated == 0)
            return;

        // Remover transmissores invalidos
        uint8_t pivot = 0;
        for (uint8_t i = 0; i < recordsLength; i++) {
            if (pivot != i)
                records[pivot] = records[i];

            if (records[pivot].latest.name != 0)
                pivot++;
        }

        recordsLength -= invalidated;
    }

    // Insere um novo transmissor na lista caso não esteja presente
    void insertRecord(broadcast_t &key) {
        uint32_t now = millis();

        // Realizar busca binária para encontrar transmissor
        uint8_t s = 0, e = recordsLength;

        while (s < e) {
            uint8_t m = (s + e) / 2;
            record_t *candidate = &records[m];

            if (candidate->latest.name < key.name) {
                s = m + 1;
            } else if (candidate->latest.name > key.name) {
                e = m;
            } else {
                Serial.printf("(transmissor %x atualizado)\n", key.name);

                // Atualizar o transmissor caso seja encontrado
                records[m].latest = key;
                records[m].lastBroadcastMillis = now;
                return;
            }
        }

        // Ignorar caso tenhamos atingido o limite do vetor
        if (recordsLength == MAX_RECORDS)
            return;

        if (selectedRecord >= s)
            selectedRecord++;

        // Inserir o novo transmissor no locaPayload l de erro da busca binária
        memmove(records + s + 1, records + s,
                sizeof(record_t) * (recordsLength - s));

        Serial.printf("(transmissor %x inserido)\n", key.name);

        records[s].latest = key;
        records[s].lastBroadcastMillis = now;
        recordsLength++;
    }
}

// Contém todo código relacionado a transmissão LoRa
namespace lora {
    // Indica a função deste dispositivo (transmissor ou receptor)
    enum { kReceiver, kTransmitter } role = kReceiver;

    // Indica se o módulo LoRa está ocupado
    bool isLoraBusy = false;

    void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
    void OnTxDone();
    void OnTxTimeout();

    // Inicializa o radio LoRa
    void setup() {
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

    // Atualiza o estado do radio LoRa
    void update() {
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
}

// Contém todo código relacionado a interface de usuário
namespace gui {
    Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RST);

    // Especifica a função a ser executada ao rodar `updateGui()`
    void (*stateFn)() = NULL;

    enum alignment_t { kStart, kCenter, kEnd };

    // Função útil para desenhar texto com alinhamento na tela
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

    void prepareDrawItem(int16_t y, bool selected) {
        if (selected) {
            Oled.fillRect(0, y, OLED_WIDTH, 8, WHITE);
            Oled.setTextColor(BLACK);
        } else {
            Oled.setTextColor(WHITE);
        }
    }

    void stateReceiverMain();
    void stateReceiverInfo();
    void stateTransmitterMain();
    void stateSettings();

    void stateReceiverMain() {
        static bool isSettingsSelected = true;

        // Descer a seleção caso o usuário esteja segurando o botão
        if (button.holding()) {
            if (isSettingsSelected) {
                isSettingsSelected = false;
            } else {
                rx::selectedRecord = (rx::selectedRecord + 1) % rx::recordsLength;
                isSettingsSelected = rx::selectedRecord == 0;
            }
        } else if (button.wasShortPressed()) {
            // Pular para outro menu caso pressionado
            if (isSettingsSelected) {
                stateFn = stateSettings;
            } else if (rx::selectedRecord < rx::recordsLength) {
                stateFn = stateReceiverInfo;
            }

            return stateFn();
        }

        // Desenhar título do menu
        char label[128];
        sprintf(label, "Receptor %x", name);

        drawAlignedText(label, 0, 0, kCenter, kStart);

        // Desenhar menu
        prepareDrawItem(8, isSettingsSelected);
        drawAlignedText("Configura\x78\x38o", 2, 8);

        for (uint8_t i = 0; i < rx::recordsLength; i++) {
            broadcast_t *broadcast = &records[i].latest;

            // Imprimir caixa de seleção
            uint16_t yOff = (i + 2) * 8;
            prepareDrawItem(yOff, !isSettingsSelected && i == rx::selectedRecord);

            // Imprimir nome do transmissor na esquerda
            sprintf(label, "%x", broadcast->name);
            drawAlignedText(label, 2, yOff);
            
            // Imprimir temperatora na direita
            sprintf(label, "%f " "\x7f" "C", broadcast->temperature);
            drawAlignedText(label, 2, yOff, alignment_t::kEnd);
        }
    }

    void stateTransmitterMain() {

    }

    void stateSettings() {

    }

    // Inicializa o módulo OLED
    void setup() {
        stateFn = lora::role == lora::kReceiver ? stateReceiverMain : stateTransmitterMain;

        Wire.begin(OLED_SDA, OLED_SCL);

        if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3c, true, false)) {
            Serial.println("Falha na inicializacao do OLED");

            while (true) { };
        }

        oled.setRotation(180);
    }

    // Atualiza o estado da interface
    void update() {
        display.clearDisplay();
        
        if (stateFn)
            stateFn();

        display.display();
    }
}

void setup() {
    Serial.begin(115200);
    Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

    // Configura o botão PRG
    button.setup();

    gui::setup();
    lora::setup();
}

void loop() {
    button.update();

    lora::update();
    gui::update();
}
