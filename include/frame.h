#ifndef FRAME_H_
#define FRAME_H_

#include <cstdint>
#include <HardwareSerial.h>

#define SIZE(x) (sizeof(x)/sizeof(x[0]))

const uint8_t STATUS_REQUEST[] = {0x49, 0x40};
#define STATUS_REQUEST_SIZE SIZE(STATUS_REQUEST)

const uint8_t USER_MESSAGE[] = {0x46, 0x40};
#define USER_MESSAGE_SIZE SIZE(USER_MESSAGE)

const uint8_t SINGLE_TRIGGER[] = {0x41, 0x40};
#define SINGLE_TRIGGER_SIZE SIZE(SINGLE_TRIGGER)

class DmmInterface {
  public:
    DmmInterface(int uart_port = 2);
    bool send(const uint8_t *data, unsigned long n);

    uint8_t packet[16];
    unsigned long packet_size = 0;
  private:
    HardwareSerial uart;

    bool writing = false;

    uint8_t read_buffer[16];
    uint8_t write_buffer[16];

    unsigned long read_i = 0;
    unsigned long write_i = 0;
    unsigned long write_size = 0;
};

#endif
