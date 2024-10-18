/**
 * Constantes e definições comuns para o transmissor e receptor.
 */

// #include <cstdint>
// #include <cstring>

// Pinagem da placa Heltec WiFi LoRa 32 (V3)

#define OLED_SDA 17
#define OLED_SCL 18
#define OLED_RST 21

#define BUTTON GPIO_NUM_0

#define RF_FREQUENCY 915000000

// Determina a potência da transmissão em dBm
// [https://www.thethingsnetwork.org/docs/lorawan/modulation-data-rate/]
#define TX_OUTPUT_POWER 15

// Determina o canal de banda larga utilizado.
// [ 0 -> 125 kHz   ]
// [ 1 -> 250 kHz   ]
// [ 2 -> 500 kHz   ]
// [ 3 -> Reservado ]
#define LORA_BANDWIDTH 0

// Afeta o tempo usado para transmitir cada símbolo (SF7..SF12)
// [https://www.thethingsnetwork.org/docs/lorawan/spreading-factors/]
#define LORA_SPREADING_FACTOR 7

// Determina a proporção entre os dados codificados em cada pacote e a
// quantidade de dados de correção de erro adicionados.
//
// [ 1 -> 4/5 ]
// [ 2 -> 4/6 ]
// [ 3 -> 4/7 ]
// [ 4 -> 4/8 ]
#define LORA_CODINGRATE 1

// Tamanho do preâmbulo em cada pacote.
#define LORA_PREAMBLE_LENGTH 8

#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define LORA_SYMBOL_TIMEOUT 0
#define RX_TIMEOUT_VALUE 1000

// Dimensão da tela OLED.
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

// Tamanho do buffer usado para armazenar a imagem final.
#define BUFFER_SIZE (OLED_WIDTH * OLED_HEIGHT / 8)

// Tamanho máximo do payload num pacote LoRa
#define MAX_PAYLOAD_SIZE 255

// Define a duração em millisegundos de um pressionamento longo em um botão
#define LONG_PRESS_DURATION 300

// Pacote enviado pelo transmissor para anunciar sua presença.
class broadcast_t {
public:
    // Armazena este pacote em um payload e retorna quantos bytes foram
    // escritos.
    //
    // O buffer passado deve ter no mínimo `sizeof(broadcast_t) + 1` bytes.
    uint8_t toPayload(uint8_t *dest) {
        dest[0] = 0;
        memcpy(dest + 1, this, sizeof(broadcast_t));

        return sizeof(broadcast_t) + 1;
    }

    // Inicializa o `broadcast_t` a partir de um payload recebido.
    // Retorna `false` caso o payload não seja um `broadcast_t` válido.
    bool fromPayload(const uint8_t *payload, uint16_t size) {
        if (size != sizeof(broadcast_t) + 1 || payload[0] != 0) {
            return false;
        }

        memcpy(&this->name, payload + 1, size - 1);

        return true;
    }

    // Nome único do transmissor.
    uint32_t name;

    // Comprimento da imagem a ser transmitida/recebida.
    uint8_t width;

    /// Altura da imagem a ser transmitida/recebida.
    uint8_t height;
};

class image_data_t {
public:
    // Armazena este pacote em um payload e retorna quantos bytes foram
    // escritos. Caso o tamanho do pedaço da imagem seja maior que o máximo
    // transmissível pelo LoRa, apenas parte da imagem será transmitida.
    // Use `toPayloadAutoOffset` para utilizar contínuamente.
    //
    // O buffer passado deve ter no mínimo `min(length, 248) + 6` bytes.
    uint8_t toPayload(uint8_t *dest) {
        uint16_t length = std::min<uint16_t>(length, MAX_PAYLOAD_SIZE - 6);

        memcpy(dest, this, 6);
        memcpy(dest + 6, this->data, length);

        return length + 6;
    }

    // Armazena este pacote em um payload e retorna quantos bytes foram
    // escritos. Atualiza o field `offset` automaticamente caso o payload seja
    // largo demais. Armazena true em `*last` caso o este seja o último pacote
    // enviado.
    uint8_t toPayloadAutoOffset(uint8_t *dest, bool *last) {
        uint16_t length = this->length - this->offset;

        length = std::min<uint16_t>(length, MAX_PAYLOAD_SIZE - 6);

        memcpy(dest, this, 6);
        memcpy(dest + 6, this->data + this->offset, length);

        if (last != NULL)
            *last = length < (MAX_PAYLOAD_SIZE - 6);

        this->offset += length;

        return length + 6;
    }

    // Inicializa o `image_data_t` a partir de um payload recebido.
    // Retorna `false` caso o payload não seja um `image_data_t` válido.
    bool fromPayload(const uint8_t *payload, uint16_t size) {
        // Deve ser no mínimo 6 bytes (cabeçalho).
        if (size < 6)
            return false;

        memcpy(&this->name, payload, 6);

        // Nome do transmissor deve ser válido.
        if (this->name == 0)
            return false;

        this->data = payload + 6;
        this->length = size - 6;

        return true;
    }

    // Nome do transmissor responsável por essa imagem.
    uint32_t name;

    // A posição inicial de `data` na imagem completa.
    uint16_t offset;

    // O tamanho da imagem/pedaço de imagem transmitido.
    const uint8_t *data;

    // Tamanho do buffer `data`, não é transmitido no payload.
    uint16_t length;
};

class button_t {
public:
    button_t(uint8_t pin)
        : start(0), state(bsIdle), buffered(false), pin(pin) {};

    void setup() {
        pinMode(pin, INPUT_PULLUP);
    }

    void update() {
        switch (state) {
        case bsIdle:
            // Começar a processar o evento caso o botão seja apertado.
            if (pressed()) {
                start = millis();
                state = bsProcessing;
            }
            break;
        case bsIgnore:
            // Continuar ignorando o evento até que o botão seja solto.
            state = pressed() ? bsIgnore : bsIdle;
            break;
        case bsProcessing:
            // Finalizar o processamento caso o botão seja solto.
            if (!pressed()) {
                duration = millis() - start;
                buffered = true;
                start = 0;
                state = bsIdle;
            }
            break;
        }
    }

    // Retorna true caso o botão esteja pressionado.
    bool pressed() {
        return digitalRead(pin) == LOW;
    }

    // Retorna true caso o botão tenha sido pressionado por um curto período de
    // tempo.
    bool wasShortPressed() {
        bool result = buffered && duration < LONG_PRESS_DURATION;
        return (consume(), result);
    }

    // Retorna true caso o botão tenha sido pressionado por mais de 2 segundos.
    bool wasLongPressed() {
        if (state == bsIgnore || state == bsIdle)
            return false;

        if (buffered)
            return (consume(), duration >= LONG_PRESS_DURATION);

        // Ignorar resultado caso os 2 segundos passem sem que o usuário solte o
        // botão.
        bool result = millis() - start >= LONG_PRESS_DURATION;

        if (result)
            state = bsIgnore;

        return result;
    }

    void consume() {
        buffered = false;
    }

private:
    uint8_t pin;
    uint32_t start;
    uint32_t duration;
    bool buffered;

    enum {
        bsIdle,
        bsProcessing,
        bsIgnore,
    } state;
};

// Converte um nome de um peer para uma string ASCII.
// O buffer passado deve ter no mínimo 8 bytes.
void hexName(char *dest, uint32_t name) {
    const char alphabet[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                             '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

    // Insere cada digito hexadecimal no buffer, ao contrário
    for (uint8_t i = 0; i < 8; i++) {
        dest[7 - i] = alphabet[name & 0xF];
        name >>= 4;
    }
}
