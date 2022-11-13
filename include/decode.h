#ifndef DECODE_H_
#define DECODE_H_

#include <cstdint>
#include <WString.h>

typedef enum {
  VDC,
  VAC,
  O2W,
  O4W,
  IDC,
  IAC
} Function;

typedef enum {
  Slow,
  Medium,
  Fast
} Speed;

typedef enum {
  Internal, // T0
  External, // T3/T4
  ExternalDelay // T1/T2
} TriggerMode;

typedef uint8_t Range;

const String mode_names[] = {
  "VOLT",
  "VOLT:AC",
  "RES",
  "FRES",
  "CURR",
  "CURR:AC"
};
const String speed_names[] = {
  "Slow",
  "Medium",
  "Fast"
};

inline bool compute_parity(uint8_t x) {
  return !(__builtin_popcount(x) % 2);
}
inline uint8_t apply_parity(uint8_t x) {
  x &= 0b1111111;
  return (compute_parity(x) << 7) | x;
}

/// Checks the parity of each byte in a packet
/// @returns if the parity was correct
bool check_parity(uint8_t *packet, unsigned long n);

bool decode_status_packet(uint8_t *packet, Function *mode, Speed *speed, TriggerMode *trigger_mode);

bool decode_reading_packet(uint8_t *packet, float *reading, Range *range);

unsigned long encode_mode_packet(uint8_t *out, uint8_t mode, uint8_t range = 0, uint8_t speed = 0, uint8_t trigger = 0);

String notation(float val);
String notation(float val, Function function);

#endif
