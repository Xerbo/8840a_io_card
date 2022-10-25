#include "frame.h"

#include <cstring>
#include <HardwareSerial.h>

// Read buffer, used in other files
uint8_t packet[16];
unsigned long packet_size = 0;
uint8_t tmp_packet[16];
unsigned long packet_i = 0;

// from 8840a_gpib.ino
extern HardwareSerial dmm;

// Write buffers, dont directly manipulate, use `send_packet`
uint8_t write_buffer[16];
unsigned long write_len = 0;
unsigned long write_i = 0;
bool writing = false;

void receive_callback() {
  uint8_t x = dmm.read();

  if (writing) {
    if (write_i == write_len) {
      writing = false;
      write_i = 0;
    } else {
      dmm.write(write_buffer[write_i++]);
    }
  } else {
    dmm.write(x);

    uint8_t header = (x >> 4) & 0b111;
    if (header == 0b110 || packet_i > 0) {
      tmp_packet[packet_i++] = x;
    }
    if (x == 0xE0 || x == 0x40 || packet_i == 16) {
      memcpy(packet, tmp_packet, 16);
      packet_size = packet_i;
      packet_i = 0;
    }
  }
}

bool send_packet(Stream &stream, const uint8_t *data, unsigned long n) {
  if (n > 16) return false;

  memcpy(write_buffer, data, n);
  write_len = n;
  writing = true;

  stream.write(write_buffer[write_i++]);

  return true;
}
