#include "config.h"

#ifdef WIFI
#include <WiFi.h>
#include <AsyncTCP.h>
#endif
#include <HardwareSerial.h>
#include <set>

#include "frame.h"
#include "decode.h"
#include "scpi.h"

#ifdef WIFI
// Bare minimum to capture the output of SCPI_Parser
class TextStream : public Stream {
  public:
    String text;
    size_t write(uint8_t x) {
      text += (char)x;
      return 1;
    }
    int available() { }
    int read() { }
    int peek() { }
};

std::set<AsyncClient *> clients;
void on_connect(void *arg, AsyncClient *client) {
  if (client == NULL) return;

  client->onData([](void *arg, AsyncClient *client, void *data, size_t len) {
    char *_data = (char *)data;
    // Remove last character and null terminate
    _data[len-1] = '\0';

    TextStream stream;
    ScpiParser *scpi = (ScpiParser*)arg;
    scpi->Execute(_data, stream);
    client->write(stream.text.c_str(), stream.text.length());
    clients.insert(client);
  }, arg);

  client->onDisconnect([](void *arg, AsyncClient *client) {
    clients.erase(client);
  });
}
#endif

void setup() {
  Serial.begin(115200);

  //pinMode(BEGIN_SAMPLE_PIN, INPUT);
  //attachInterrupt(digitalPinToInterrupt(BEGIN_SAMPLE_PIN), trigger, RISING);

  pinMode(SAMPLE_COMPLETE_PIN, OUTPUT);
  digitalWrite(SAMPLE_COMPLETE_PIN, LOW);
  const esp_timer_create_args_t end_pulse_timer_args = {
    .callback = [](void *arg) { digitalWrite(SAMPLE_COMPLETE_PIN, LOW); },
    .arg = NULL,
    .name = "end-pulse"
  };

  esp_timer_handle_t end_pulse_timer;
  esp_timer_create(&end_pulse_timer_args, &end_pulse_timer);

#ifdef BEEPER
  ledcAttachPin(BEEPER_PIN, BEEPER_CHANNEL);
  ledcSetup(BEEPER_CHANNEL, 1000, 8);
  ledcWrite(BEEPER_CHANNEL, 0);
#endif

  Function function = VDC;
  Range range = 1;
  Speed speed = Slow;
  TriggerMode trigger_mode = Internal;
  float reading = 0.0f;

  DmmInterface dmm;
  ScpiParser scpi(dmm, reading, function, range);

#ifdef WIFI
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  AsyncServer server(5025);
  server.begin();
  server.onClient(on_connect, (void *)&scpi);
#endif

  // Main loop, processes unimportant things
  // Namely: decoding packets and parsing SCPI commands
  while (true) {
    scpi.ProcessInput(Serial, "\n");
    if (dmm.packet_size == 0 || !check_parity(dmm.packet, dmm.packet_size)) {
      continue;
    }

    switch (dmm.packet[0]) {
      case 0x67:
        if (dmm.packet_size == 9 && decode_reading_packet(dmm.packet, &reading, &range)) {
          if (scpi.wanted_samples != 0) {
            String x = notation(reading, function);
            Serial.println(x);
#ifdef WIFI
            for (AsyncClient *client : clients) {
              client->write(x.c_str(), x.length());
            }
#endif
          }
          if (scpi.wanted_samples > 0) {
            scpi.wanted_samples--;
          }
        }
        break;
      case 0xE5:
        if (dmm.packet_size == 7) {
          decode_status_packet(dmm.packet, &function, &speed, &trigger_mode);
        }
        break;
      case 0x61:
        if (dmm.packet_size == 2 && dmm.packet[1] == 0xE0) {
          // Official GPIB card uses a 2.5us pulse
          // Left at 1ms for testing purposes
          digitalWrite(SAMPLE_COMPLETE_PIN, HIGH);
          esp_timer_start_once(end_pulse_timer, 1000);
        }
        break;
      default:
        break;
    }

    dmm.packet_size = 0;
  }
}

void loop() { }
