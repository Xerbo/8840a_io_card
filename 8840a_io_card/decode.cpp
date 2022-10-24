#include "decode.h"

bool decode_status_packet(uint8_t *packet, Function *mode, Speed *speed, TriggerMode *trigger_mode) {
  uint8_t _mode = packet[1] & 0b1111;
  if (_mode >= 1 && _mode <= 6) {
    *mode = (Function)(_mode - 1);
  } else {
    return false;    
  }

  uint8_t _speed = packet[3] & 0b111;
  if (_speed >= 1 && _speed <= 3) {
    *speed = (Speed)(_speed - 1);
  } else {
    return false;    
  }

  bool setting_delay = (packet[3] >> 3) & 0b1;
  bool external_trigger = (packet[4] >> 0) & 0b1;
  if (external_trigger) {
    *trigger_mode == setting_delay ? ExternalDelay : External;
  } else {
    *trigger_mode = Internal;
  }

  return true;
}

bool decode_reading_packet(uint8_t *packet, float *reading, Range *range) {
  // First stage, reading will be 0 -> 0.99999
  float _reading = 0.0;
  float decade = 0.1;
  for (unsigned long i = 0; i < 5; i++) {
    uint8_t digit = packet[2 + i] & 0b1111;
    _reading += digit*decade;
    decade *= 0.1f;
  }

  // Apply the half digit and sign, reading is now -1.99999 -> 1.99999
  bool sign  = (packet[1] >> 3) & 0b1;
  bool digit = (packet[1] >> 0) & 0b1;
  if (digit) {
    _reading += 1.0f;
  }
  if (sign) {
    _reading = -_reading;
  }

  // Apply range, reading is now -19.9999M -> 19.9999M
  uint8_t _range = packet[7] & 0b1111;
  switch (_range) {
    case 1: _reading *= 0.1f; break;
    case 2: _reading *= 1.0f; break;
    case 3: _reading *= 10.0f; break;
    case 4: _reading *= 100.0f; break;
    case 5: _reading *= 1000.0f; break;
    case 6: _reading *= 10000.0f; break;
    default: return false;
  }

  *range   = _range;
  *reading = _reading;
  return true;
}

bool check_parity(uint8_t *packet, unsigned long n) {
  for (unsigned long i = 0; i < n; i++) {
    bool expected_parity = (packet[i] >> 7) & 0b1;
    bool calculated_parity = compute_parity(packet[i] & 0b1111111);

    if (calculated_parity != expected_parity) {
      return false;
    }
  }

  return true;
}

unsigned long encode_mode_packet(uint8_t *out, uint8_t mode, uint8_t range, uint8_t speed, uint8_t trigger) {
  out[0] = 0xCB;
  out[1] = apply_parity(mode    & 0b1111);
  out[2] = apply_parity(range   & 0b1111);
  out[3] = apply_parity(speed   & 0b1111);
  out[4] = apply_parity(trigger & 0b1111);
  out[5] = 0x40;
  return 6;
}

String notation(float val) {
  int expontent = 0;
  while (val > 10.0f) {
    val /= 10.0f;
    expontent++;
  }

  return (val >= 0.0f ? "+" : "") + String(val, 5) + "E+" + String(expontent);
}

String notation(float val, Function function) {
  if (function == O2W || function == O4W) {
    val *= 1000.0f;
  }
  if (function == IDC || function == IAC) {
    val /= 1000.0f;
  }
  return notation(val);
}