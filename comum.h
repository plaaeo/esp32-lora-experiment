/**
 * Constantes e definições comuns para o transmissor e receptor.
 */

#define OLED_SDA 17
#define OLED_SCL 18
#define OLED_RST 21

#define RF_FREQUENCY 915000000

// Determina a potência da transmissão em dBm [https://www.thethingsnetwork.org/docs/lorawan/modulation-data-rate/]
#define TX_OUTPUT_POWER 15

// Determina o canal de banda larga utilizado.
// [ 0 -> 125 kHz   ]
// [ 1 -> 250 kHz   ]
// [ 2 -> 500 kHz   ]
// [ 3 -> Reservado ]
#define LORA_BANDWIDTH 0

// Afeta o tempo usado para transmitir cada símbolo (SF7..SF12) [https://www.thethingsnetwork.org/docs/lorawan/spreading-factors/]
#define LORA_SPREADING_FACTOR 7

// Determina a proporção entre os dados codificados em cada pacote e a quantidade de
// dados de correção de erro adicionados.
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

#define PROTO_VERSION 0

typedef enum {
    // (t -> r) Configura o receptor para receber determinada dimensão de imagem (3 bytes).
    kConfig,

    // (r -> t) Avisa o transmissor que a configuração foi aceita (0 bytes).
    kAck,

    // (t -> r) Dado da imagem.
    kData,
} packet_type_t;

typedef struct {
    // Versão do protocolo, deve ser igual a `PROTO_VERSION`.
    uint8_t version;

    // Comprimento da imagem a ser transmitida.
    uint8_t width;

    // Altura da imagem a ser transmitida.
    uint8_t height;
} packet_config_t;
