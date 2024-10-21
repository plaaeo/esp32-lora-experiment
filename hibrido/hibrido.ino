#include "comum.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LoRaWan_APP.h>
#include <Wire.h>
#include <cstdio>
#include <esp_random.h>

#define MAX_RECORDS 32

button_t button(BUTTON);

// Gera um nome aleatório para este dispositivo
uint32_t name = esp_random();

// Define o tempo de espera entre cada coleta (e transmissão) de temperatura
uint32_t refreshDelay = 1000;

// A resistencia atual do termistor
uint32_t currentResistance = 0;

// A temperatura atual do termistor, em graus Celsius
float currentTemperature = 0.0f;

// Código específico para o receptor, que não se encaixa nos outros namespaces
// abaixo
namespace rx {

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
                Serial.printf("(transmissor %x removido)\n",
                              record->latest.name);
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

        // Inserir o novo transmissor no local de erro da busca binária
        memmove(records + s + 1, records + s,
                sizeof(record_t) * (recordsLength - s));

        Serial.printf("(transmissor %x inserido)\n", key.name);

        records[s].latest = key;
        records[s].lastBroadcastMillis = now;
        recordsLength++;
    }
} // namespace rx

// Contém todo código relacionado a transmissão LoRa
namespace lora {
    struct lora_parameters_t {
        int8_t power;
        uint32_t bandwidth;
        uint32_t spreadingFactor;
        uint8_t codingRate;
        uint16_t preambleLength;
        bool iqInverted;
    } parameters{TX_OUTPUT_POWER, LORA_BANDWIDTH,       LORA_SPREADING_FACTOR,
                 LORA_CODINGRATE, LORA_PREAMBLE_LENGTH, LORA_IQ_INVERSION_ON};

    // Indica a função deste dispositivo (transmissor ou receptor)
    enum { kReceiver, kTransmitter } role = kReceiver;

    // Indica se o módulo LoRa está ocupado
    bool isLoraBusy = false;

    // Indica se o código deve re-inicializar o radio LoRa
    bool refreshParameters = false;

    void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
    void OnTxDone();
    void OnTxTimeout();

    // Inicializa o radio LoRa
    void setup() {
        static RadioEvents_t events;

        events.RxDone = OnRxDone;
        events.TxDone = OnTxDone;
        events.TxTimeout = OnTxTimeout;

        Radio.Init(&events);
        Radio.SetChannel(RF_FREQUENCY);

        Radio.SetTxConfig(MODEM_LORA, parameters.power, 0, parameters.bandwidth,
                          parameters.spreadingFactor, parameters.codingRate,
                          parameters.preambleLength, true, true, false, 0,
                          parameters.iqInverted, 3000);

        Radio.SetRxConfig(MODEM_LORA, parameters.bandwidth,
                          parameters.spreadingFactor, parameters.codingRate, 0,
                          parameters.preambleLength, 0, true, 0, true, 0, 0,
                          parameters.iqInverted, true);
    }

    // Atualiza o estado do radio LoRa
    void update() {
        // Tempo em millisegundos desde o ultimo broadcast
        static uint32_t then = millis();
        uint32_t now = millis();

        Radio.IrqProcess();

        if (isLoraBusy)
            return;

        if (refreshParameters) {
            refreshParameters = false;
            setup();
        }

        if (role == kReceiver) {
            Radio.Rx(0);
            isLoraBusy = true;
        } else if (role == kTransmitter) {
            if (then - now < std::max<uint32_t>(refreshDelay, 500))
                return;

            broadcast_t msg;
            msg.name = name;
            msg.temperature = currentTemperature;
            msg.resistance = currentResistance;

            uint8_t buffer[sizeof(broadcast_t) + 4];
            uint8_t size = msg.toPayload(buffer);
            Radio.Send(buffer, size);

            then = now;
            isLoraBusy = true;
        }
    }

    // Executado ao receber um payload pelo LoRa
    void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
        if (Serial)
            Serial.printf("[ rx %d bytes, %d dBm RSSI, %d dB SNR ]\n", size,
                          rssi, snr);

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
} // namespace lora

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

        // Alinhar a posição do texto horizontalmente, de acordo com a regra
        // dada
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
            oled.fillRect(0, y, OLED_WIDTH, 8, WHITE);
            oled.setTextColor(BLACK);
        } else {
            oled.setTextColor(WHITE);
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
                rx::selectedRecord =
                    (rx::selectedRecord + 1) % rx::recordsLength;
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
        sprintf(label, "\xf7 Receptor %x", name);

        drawAlignedText(label, 0, 0, kCenter, kStart);

        // Desenhar menu
        prepareDrawItem(8, isSettingsSelected);
        drawAlignedText("\x2a Configura\x87\x83o", 2, 8);

        for (uint8_t i = 0; i < rx::recordsLength; i++) {
            broadcast_t *broadcast = &rx::records[i].latest;

            // Imprimir caixa de seleção
            uint16_t yOff = (i + 2) * 8;
            prepareDrawItem(yOff,
                            !isSettingsSelected && i == rx::selectedRecord);

            // Imprimir nome do transmissor na esquerda
            sprintf(label, "%x", broadcast->name);
            drawAlignedText(label, 2, yOff);

            // Imprimir temperatora na direita
            sprintf(label,
                    "%f "
                    "\x7f"
                    "C",
                    broadcast->temperature);
            drawAlignedText(label, 2, yOff, alignment_t::kEnd);
        }
    }

    void stateTransmitterMain() {
    }

    void stateReceiverInfo() {
    }

    void stateSettings() {
        struct setting_t {
            const char *label;
            void (*value)(char *dst);
            void (*onPress)();
        };

        const setting_t settings[] = {
            {
                "Nome",
                [](char *dst) { sprintf(dst, "%x", name); },
                []() { name = esp_random(); },
            },
            {
                "Cargo",
                [](char *dst) {
                    sprintf(dst, lora::role == lora::kReceiver ? "Receptor"
                                                         : "Transmissor");
                },
                []() {
                    lora::role =
                        lora::role == lora::kReceiver ? lora::kTransmitter : lora::kReceiver;
                },
            },
            {
                "Delay",
                [](char *dst) {
                    sprintf(dst, "%.1fs", ((float)refreshDelay) / 1000.0f);
                },
                []() {
                    refreshDelay += 500;
                    refreshDelay = std::max<uint32_t>(refreshDelay % 5500, 500);
                },
            },
            {
                "Pot\x88ncia",
                [](char *dst) {
                    sprintf(dst, "%ddBm", lora::parameters.power);
                },
                []() {
                    lora::parameters.power++;
                    lora::parameters.power =
                        std::max<uint32_t>(lora::parameters.power % 16, 1);
                },
            },
            {
                "Larg. de Banda",
                [](char *dst) {
                    const uint32_t values[] = {125, 250, 500};
                    sprintf(dst, "%dkHz", values[lora::parameters.bandwidth]);
                },
                []() {
                    lora::parameters.bandwidth =
                        (lora::parameters.bandwidth + 1) % 3;
                },
            },
            {
                "Coding Rate",
                [](char *dst) {
                    sprintf(dst, "4/%d", 5 + lora::parameters.codingRate);
                },
                []() {
                    lora::parameters.codingRate++;
                    lora::parameters.codingRate =
                        std::max<uint32_t>(lora::parameters.codingRate % 5, 1);
                },
            },
            {
                "Spreading Factor",
                [](char *dst) {
                    sprintf(dst, "SF%d", lora::parameters.spreadingFactor);
                },
                []() {
                    lora::parameters.spreadingFactor++;
                    lora::parameters.spreadingFactor =
                        std::max<uint32_t>(lora::parameters.codingRate % 13, 7);
                },
            },
            {
                "\xb1 Voltar",
                [](char *dst) {},
                []() {
                    stateFn = lora::role == lora::kReceiver ? stateReceiverMain
                                                      : stateTransmitterMain;
                },
            },
        };

        const uint32_t settingsLength = sizeof(settings) / sizeof(setting_t);
        const int32_t maxStartSetting =
            std::min<int32_t>(settingsLength - 8, 0);

        static uint32_t selectedSetting = 0;
        static uint32_t startSetting = 0;

        // Descer a seleção caso o usuário esteja segurando o botão
        if (button.holding()) {
            selectedSetting = (selectedSetting + 1) % settingsLength;

            startSetting = std::max<int32_t>(selectedSetting - 6, 0);
            startSetting = std::min<int32_t>(startSetting, maxStartSetting);
        } else if (button.wasShortPressed()) {
            settings[selectedSetting].onPress();
        }

        // Desenhar título do menu
        char label[128] = "\xa2 Configura\x78\x38o";
        drawAlignedText(label, 0, 0, kCenter, kStart);

        uint16_t yOff = 8;
        for (uint8_t i = startSetting; i < settingsLength - startSetting; i++) {
            if (i == 2 && lora::role == lora::kReceiver)
                continue;

            const setting_t *setting = &settings[i];

            // Imprimir caixa de seleção
            prepareDrawItem(yOff, i == selectedSetting);

            // Imprimir nome e valor da configuração
            drawAlignedText(setting->label, 2, yOff);

            setting->value(label);
            drawAlignedText(label, 2, yOff, alignment_t::kEnd);

            yOff += 8;
        }
    }

    // Inicializa o módulo OLED
    void setup() {
        stateFn = lora::role == lora::kReceiver ? stateReceiverMain
                                                : stateTransmitterMain;

        Wire.begin(OLED_SDA, OLED_SCL);

        if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3c, true, false)) {
            Serial.println("Falha na inicializacao do OLED");

            while (true) {
            };
        }

        oled.setRotation(2);
    }

    // Atualiza o estado da interface
    void update() {
        oled.clearDisplay();
        oled.setTextColor(WHITE);

        if (stateFn)
            stateFn();

        oled.display();
    }
} // namespace gui

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
