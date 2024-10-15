#include <LoRaWan_APP.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "../comum.h"

Adafruit_SSD1306 Oled(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RST);

size_t transmittedBytes = 0;

/// Imagem completa a ser enviada ao receptor.
const unsigned char Abuffer[] PROGMEM = {
  0xff, 0xff, 0xff, 0xc0, 0x03, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x7f, 0xff, 0xff,
  0xff, 0xff, 0xf0, 0x3f, 0xfc, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xc3, 0xff, 0xff, 0xc3, 0xff, 0xff,
  0xff, 0xff, 0x0f, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xfc, 0x3f, 0xff, 0xff, 0xfc, 0x3f, 0xff,
  0xff, 0xf8, 0xff, 0xff, 0xff, 0xff, 0x1f, 0xff, 0xff, 0xf1, 0xff, 0xff, 0xff, 0xff, 0x8f, 0xff,
  0xff, 0xe7, 0xff, 0xff, 0xff, 0xff, 0xe7, 0xff, 0xff, 0xcf, 0xff, 0xff, 0xff, 0xff, 0xf3, 0xff,
  0xff, 0x9f, 0xff, 0xff, 0xff, 0xff, 0xf9, 0xff, 0xff, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xfc, 0xff,
  0xfe, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x7f, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3f,
  0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3f, 0xf9, 0xf0, 0x7f, 0xff, 0x00, 0x1f, 0x0f, 0x9f,
  0xf9, 0xf0, 0x7f, 0xff, 0x00, 0x08, 0x03, 0x9f, 0xf3, 0xf0, 0x7f, 0xff, 0x00, 0x08, 0x03, 0xcf,
  0xf3, 0xf0, 0x7f, 0xff, 0x00, 0x00, 0x03, 0xcf, 0xe7, 0xf0, 0x7f, 0xff, 0x00, 0x00, 0x03, 0xe7,
  0xef, 0xf0, 0x7c, 0x0f, 0x00, 0x10, 0x33, 0xf7, 0xcf, 0xf0, 0x78, 0x03, 0xe0, 0xf0, 0x3f, 0xf3,
  0xcf, 0xf0, 0x70, 0x03, 0xe0, 0xf0, 0x1f, 0xf3, 0xcf, 0xf0, 0x70, 0x01, 0xe0, 0xf0, 0x0f, 0xf3,
  0x9f, 0xf0, 0x60, 0x01, 0xe0, 0xf8, 0x07, 0xf9, 0x9f, 0xf0, 0x60, 0x81, 0xe0, 0xf8, 0x03, 0xf9,
  0x9f, 0xf0, 0x60, 0xc0, 0xe0, 0xfc, 0x01, 0xf9, 0x9f, 0xf0, 0x60, 0xc0, 0xe0, 0xfe, 0x01, 0xf9,
  0x9f, 0xf0, 0x60, 0xc0, 0xe0, 0xff, 0x80, 0xf9, 0x9f, 0xf0, 0x60, 0x80, 0xe0, 0xfb, 0x80, 0xf9,
  0x3f, 0xf0, 0x60, 0x01, 0xe0, 0xf0, 0x00, 0xfc, 0x3f, 0xf0, 0x60, 0x01, 0xe0, 0xf0, 0x01, 0xfc,
  0xbf, 0xf0, 0x70, 0x03, 0xe0, 0xf0, 0x01, 0xfc, 0x3f, 0xf0, 0x70, 0x00, 0x00, 0xf0, 0x03, 0xfd,
  0x9f, 0xf0, 0x70, 0x00, 0x00, 0x00, 0x07, 0xf9, 0x9f, 0xf0, 0x40, 0x1e, 0x00, 0x00, 0x1f, 0xf9,
  0x9f, 0xf0, 0x07, 0x0f, 0xff, 0xff, 0xff, 0xf9, 0x9f, 0xf0, 0x1f, 0x0f, 0xff, 0xff, 0xff, 0xf9,
  0x9f, 0xf0, 0x3f, 0x0f, 0xff, 0xff, 0xff, 0xf9, 0x9f, 0xf0, 0x7f, 0x07, 0xff, 0xff, 0xff, 0xf9,
  0xcf, 0xf0, 0x87, 0xe3, 0xff, 0xff, 0xff, 0xf3, 0xcf, 0xf1, 0x00, 0x00, 0xff, 0xff, 0xff, 0xf3,
  0xcf, 0xe3, 0x20, 0x00, 0x1f, 0xff, 0xff, 0xf3, 0xef, 0xe7, 0x07, 0x00, 0x03, 0xff, 0xff, 0xf7,
  0xe7, 0xe3, 0xdf, 0x07, 0xe0, 0xff, 0xff, 0xe7, 0xf3, 0xf0, 0xff, 0x1f, 0xfc, 0xff, 0xff, 0xcf,
  0xf3, 0xf8, 0x7f, 0x1f, 0xf9, 0xff, 0xff, 0xcf, 0xf9, 0xfc, 0x3f, 0xff, 0xe3, 0xff, 0xff, 0x9f,
  0xf9, 0xff, 0x03, 0xff, 0x07, 0xff, 0xff, 0x9f, 0xfc, 0xff, 0x80, 0x00, 0x1f, 0xff, 0xff, 0x3f,
  0xfc, 0xff, 0xf0, 0x00, 0x7f, 0xff, 0xff, 0x3f, 0xfe, 0x7f, 0xff, 0x03, 0xff, 0xff, 0xfe, 0x7f,
  0xff, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xfc, 0xff, 0xff, 0x9f, 0xff, 0xff, 0xff, 0xff, 0xf9, 0xff,
  0xff, 0xcf, 0xff, 0xff, 0xff, 0xff, 0xf3, 0xff, 0xff, 0xe7, 0xff, 0xff, 0xff, 0xff, 0xe7, 0xff,
  0xff, 0xf1, 0xff, 0xff, 0xff, 0xff, 0x8f, 0xff, 0xff, 0xf8, 0xff, 0xff, 0xff, 0xff, 0x1f, 0xff,
  0xff, 0xfc, 0x3f, 0xff, 0xff, 0xfc, 0x3f, 0xff, 0xff, 0xff, 0x1f, 0xff, 0xff, 0xf0, 0xff, 0xff,
  0xff, 0xff, 0xc3, 0xff, 0xff, 0xc3, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x3f, 0xfc, 0x0f, 0xff, 0xff,
  0xff, 0xff, 0xfc, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x03, 0xff, 0xff, 0xff
};

const unsigned char buffer [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x90, 0x0b, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x10, 0x0b, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79, 0x7f, 0xfe, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe1, 0xff, 0xff, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xc7, 0xfd, 0xff, 0x9b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x8f, 0xf8, 0x3f, 0xf3, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x3e, 0x18, 0x00, 0xf8, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x78, 0x08, 0x10, 0x38, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x60, 0x1f, 0x30, 0x1c, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0xe0, 0x3f, 0x3c, 0x0e, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0xc0, 0xff, 0xff, 0x07, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x33, 0xc1, 0xff, 0xff, 0x83, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x23, 0x07, 0xff, 0xfd, 0xc1, 0xac, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x0f, 0xff, 0xfb, 0xe1, 0x8c, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x66, 0x0f, 0x9f, 0xf7, 0xe0, 0xc4, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x66, 0x1f, 0xf7, 0xef, 0xf0, 0xee, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x1f, 0xff, 0xc7, 0xf0, 0xe6, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xec, 0x1b, 0xff, 0x07, 0x78, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xcc, 0x1e, 0xff, 0xef, 0xf8, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xdc, 0x3f, 0x9f, 0xff, 0xf8, 0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xec, 0x3f, 0xf7, 0xff, 0xf8, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xcc, 0x3f, 0xfb, 0xf6, 0xf8, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x3f, 0xfb, 0xff, 0x78, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x3f, 0xfb, 0xef, 0xf8, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x1f, 0xfb, 0xef, 0xf8, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x1f, 0xf7, 0xff, 0xf0, 0xe6, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xc6, 0x1f, 0xf7, 0xff, 0xf0, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x6e, 0x0f, 0xff, 0xbf, 0xf0, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x0f, 0xff, 0xff, 0xe1, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x07, 0xbf, 0xff, 0xc1, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x83, 0xff, 0xff, 0x81, 0x9c, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x81, 0xfd, 0xff, 0x03, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0xc0, 0x7b, 0xfe, 0x07, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0xe0, 0x3f, 0xf8, 0x0e, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0x70, 0x1f, 0xf8, 0x1c, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x3c, 0x0f, 0xf0, 0x7c, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x1e, 0x08, 0x1f, 0xf8, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x8f, 0xf8, 0x1f, 0xe3, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xcf, 0xfb, 0xff, 0xc3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe1, 0xff, 0xff, 0x2e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x3f, 0xf8, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x08, 0x02, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xc8, 0x03, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x11, 0xf8, 0x30, 0x78, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x11, 0x80, 0x78, 0x78, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x11, 0x80, 0x48, 0x68, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x11, 0x80, 0x88, 0x69, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x11, 0x80, 0x8c, 0x69, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x11, 0x80, 0xcc, 0x67, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x11, 0x81, 0x86, 0x66, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x31, 0x81, 0x02, 0x66, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xc0, 0x00, 0x02, 0x22, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Usado para desligar a transmissão da imagem quando um pacote estiver em transporte.
bool transmitting = false;
enum {
  sConfiguring,
  sAcking,
  sTransmitting,
} state;

// Configuração inicial da imagem
packet_config_t config = { PROTO_VERSION, 128, 64 };

// Executado quando um pacote completo for enviado.
void OnTxDone(void) {
  Serial.println("Pacote transmitido");

  if (state == sConfiguring) {
    state = sAcking;
  }

  transmitting = false;
}

void OnTxTimeout(void) {
  Radio.Sleep();
  transmitting = false;
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  if (size == 0) {
    Serial.println("Pacote vazio recebido, ignorando...");
    return;
  }

  switch (payload[0]) {
    case kAck:
      Serial.println("ACK recebido! Conexao estabelecida.");
      state = sTransmitting;
      break;

    default:
      Serial.println("Tipo inválido de pacote recebido.");
      break;
  }

  Radio.Sleep();

  transmitting = false;
}

void setup() {
  Serial.begin(115200);
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  transmittedBytes = 0;

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

// Envia uma parte da imagem para o receptor,
void tickEnviarImagem() {
  static uint8_t packet[255] = { kData };
  static size_t transmittedBytes = 0;

  // Desenhar a logo no transmissor como referência
  Oled.drawBitmap(0, 0, buffer, 64, 64, WHITE);
  Oled.display();

  delay(1000);

  if (transmittedBytes < sizeof(buffer)) {
    // Calcula a quantidade de bytes para enviar (no máximo 254)
    size_t bytes = sizeof(buffer) - transmittedBytes;
    bytes = bytes < 254 ? bytes : 254;
    
    memcpy(packet + 1, buffer + transmittedBytes, bytes);
    Radio.Send(packet, bytes + 1);

    // Soma a quantidade de bytes enviada num offset
    transmittedBytes += bytes;
    transmitting = true;
  } else {
    transmittedBytes = 0;
  }
}

void loop() {
  static uint8_t ackCount = 0;
  
  if (!transmitting) {
    switch (state) {
      case sConfiguring:
        Serial.println("Enviando configuração...");
        
        static uint8_t buffer[4] = { kConfig };
        memcpy(buffer + 1, &config, sizeof(packet_config_t));
        
        Radio.Send(buffer, 4);
        break;
      case sAcking:
        delay(1000);
        Serial.printf("Aguardando ACK (%d/5)...\n", ackCount++);
        
        // Voltar ao estado de configuração caso nenhum ACK seja recebido
        if (ackCount > 5) {
          state = sConfiguring; 
          ackCount = 0;
        }

        Radio.Rx(0);
        break;
      case sTransmitting:
        delay(1000);
        Serial.println("Transmitindo imagem...");
        tickEnviarImagem();
        break;
    }
  }

  Radio.IrqProcess();
}
