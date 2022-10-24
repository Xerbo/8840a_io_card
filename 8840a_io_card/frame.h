#ifndef FRAME_H_
#define FRAME_H_

#include <cstdint>
#include <Stream.h>

#define SIZE(x) (sizeof(x)/sizeof(x[0]))

const uint8_t STATUS_REQUEST[] = {0x49, 0x40};
#define STATUS_REQUEST_SIZE SIZE(STATUS_REQUEST)

const uint8_t USER_MESSAGE[] = {0x46, 0x40};
#define USER_MESSAGE_SIZE SIZE(USER_MESSAGE)

const uint8_t SINGLE_TRIGGER[] = {0x41, 0x40};
#define SINGLE_TRIGGER_SIZE SIZE(SINGLE_TRIGGER)

// onReceive function for `dmm`
void receive_callback();

/// Send a packet while managing error correction asynchronously
/// @returns if the transfer could be performed
bool send_packet(Stream &stream, const uint8_t *data, unsigned long n);

#endif
