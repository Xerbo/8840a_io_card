#include "frame.h"

#include <cstring>

DmmInterface::DmmInterface(int uart_port) : uart(uart_port) {
  uart.begin(62500, SERIAL_8N2);
  uart.setRxFIFOFull(1);
  uart.onReceive([this]() {
    uint8_t x = uart.read();

    if (writing) {
      if (write_i == write_size) {
        writing = false;
        write_i = 0;
      } else {
        uart.write(write_buffer[write_i++]);
      }
    } else {
      uart.write(x);

      uint8_t header = (x >> 4) & 0b111;
      if (header == 0b110 || read_i > 0) {
        read_buffer[read_i++] = x;
      }
      if (x == 0xE0 || x == 0x40 || read_i == 16) {
        memcpy(packet, read_buffer, 16);
        packet_size = read_i;
        read_i = 0;
      }
    }
  });
}

bool DmmInterface::send(const uint8_t *data, unsigned long n) {
  if (n > 16 || writing) return false;

  memcpy(write_buffer, data, n);
  write_size = n;
  writing = true;

  uart.write(write_buffer[write_i++]);

  return true;
}
