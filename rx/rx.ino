#include "../comum.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LoRaWan_APP.h>
#include <Wire.h>

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);

button_t button(BUTTON);

Adafruit_SSD1306 Oled(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RST);

// Usado para esperar o fim do recebimento de um pacote.
bool canRx = false;

// Usado para armazenar a imagem final.
uint8_t buffer[BUFFER_SIZE];

// Contem a mensagem de broadcast do transmissor escolhido.
broadcast_t msgBroadcast{0, 0, 0};
image_data_t msgImage{0, 0, NULL, 0};

enum { sIdle, sListening, sReceiving } state = sIdle;

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

    RadioEvents.RxDone = OnRxDone;

    Radio.Init(&RadioEvents);
    Radio.SetChannel(RF_FREQUENCY);

    Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                      LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                      LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON, 0, true,
                      0, 0, LORA_IQ_INVERSION_ON, true);
}

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

            if (selectedRecord == pivot)
                selectedRecord--;
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

void loop() {
    static bool showingImage = false;

    button.update();

    if (button.wasShortPressed()) {
        // Começar a receber a imagem ao selecionar um transmissor
        if (state == sListening) {
            msgBroadcast = records[selectedRecord].latest;
            state = sReceiving;
            // Mostrar/ocultar a imagem sendo recebida do transmissor
        } else if (state == sReceiving) {
            showingImage = !showingImage;
        }
    } else if (button.wasLongPressed()) {
        state = state == sListening ? sReceiving : sListening;
        memset(buffer, 0, BUFFER_SIZE);
    }

    // Começar a receber a imagem
    if (state != sIdle && canRx) {
        Serial.printf(state == sListening ? "Esperando broadcast... "
                                          : "Esperando imagem... ");

        Radio.Rx(0);
        canRx = false;
    }

    Radio.IrqProcess();

    // Remover transmissores ociosos
    purgeRecords();

    if (selectedRecord >= recordsLength)
        selectedRecord = recordsLength - 1;

    // Desenhar status do dispositivo
    Oled.clearDisplay();

    switch (state) {
    case sReceiving:
        if (showingImage) {
            // Desenhar bitmap da imagem sendo recebida no centro do OLED
            Oled.drawBitmap((OLED_WIDTH - msgBroadcast.width) / 2,
                            (OLED_HEIGHT - msgBroadcast.height) / 2, buffer,
                            msgBroadcast.width, msgBroadcast.height);
        } else {
            // Desenhar status do dispositivo
            char nome[] = "Recebendo de 00000000";
            hexName(nome + 13, msgBroadcast.name);

            int16_t x, y, w, h;
            Oled.getTextBounds(nome, 0, 0, &x, &y, &w, &h);
            Oled.setCursor((OLED_WIDTH - w) / 2, (OLED_HEIGHT - h) / 2);
            Oled.println(nome);

            const char label[256];
            sprintf(label, "%d/%d bytes", msgImage.offset, msgImage.length);

            Oled.getTextBounds(label, 0, 0, &x, &y, &w, &h);
            Oled.setCursor((OLED_WIDTH - w) / 2, (OLED_HEIGHT / 2) + h);
            Oled.println(label);
        }
        break;

    case sListening:
        Oled.setCursor(2, 2);
        Oled.println("Dispositivo receptor");

        Oled.setCursor(12, 10);
        Oled.println("Transmissores disponiveis:");

        for (uint8_t i = 0; i < recordsLength; i++) {
            broadcast_t *broadcast = &records[i].latest;

            uint16_t y = 18 + i * 8;

            // Destacar transmissor selecionado
            if (i == selectedRecord) {
                Oled.drawRect(0, y, OLED_WIDTH, y + 8, WHITE);
                Oled.setTextColor(BLACK);
            } else {
                Oled.setTextColor(WHITE);
            }

            Oled.setCursor(12, y);
            Oled.printf("%x", broadcast->name);

            // Imprimir dimensão da imagem na direita
            int16_t x, y, w, h;

            const char label[64];
            sprintf(label, "(%dx%d)", broadcast->width, broadcast->height);

            Oled.getTextBounds(label, 0, 0, &x, &y, &w, &h);
            Oled.setCursor(OLED_WIDTH - w - 2, y);
            Oled.println(label);
        }
        break;
    }

    Oled.display();
}

// Executado quando qualquer pacote for recebido.
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
    if (size == 0) {
        Serial.println("pacote vazio recebido!");
        return;
    }

    Serial.printf("recebido! %d bytes\n", size);

    switch (state) {
    case sListening:
        broadcast_t msg{0, 0, 0};

        // Gravar broadcasts válidos recebidos
        if (msg.fromPayload(payload, size)) {
            insertRecord(msg);
        } else {
            Serial.println("Broadcast invalido recebido!");
        }
        break;

    case sReceiving:
        image_data_t msg{0, 0, NULL, 0};

        if (msg.fromPayload(payload, size)) {
            // Limitar tamanho copiado para evitar buffer overflow
            uint8_t length = min(msg.length, BUFFER_SIZE - msg.offset);
            memcpy(buffer + msg.offset, msg.data, length);
        } else {
            Serial.println("Imagem invalida recebida!");
        }
        break;
    }

    Radio.Sleep();
    canRx = true;
}