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

HardwareSerial dmm(2);
#ifdef WIFI
AsyncServer server(5025);
#endif
esp_timer_handle_t end_pulse_timer;

Function function;
Range range;
Speed speed;
TriggerMode trigger_mode;
float reading;

// from scpi.cpp
extern SCPI_Parser scpi;
extern unsigned long wanted_samples;

// From proto.cpp
extern uint8_t packet[16];
extern unsigned long packet_size;

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
void on_data(void *arg, AsyncClient *client, void *data, size_t len) {
  char *_data = (char *)data;

  // Remove last character and null terminate
  _data[len-1] = '\0';

  TextStream stream;
  scpi.Execute(_data, stream);
  client->write(stream.text.c_str(), stream.text.length());
  clients.insert(client);
}

void on_disconnect(void *arg, AsyncClient *client) {
  clients.erase(client);
}

void on_connect(void *arg, AsyncClient *client) {
  if (client == NULL) return;

  client->onData(on_data);
  client->onDisconnect(on_disconnect);
}
#endif

void setup() {
  Serial.begin(500000);

  dmm.begin(62500, SERIAL_8N2);
  dmm.setRxFIFOFull(1);
  dmm.onReceive(receive_callback);

  init_scpi();

  //pinMode(BEGIN_SAMPLE_PIN, INPUT);
  //attachInterrupt(digitalPinToInterrupt(BEGIN_SAMPLE_PIN), trigger, RISING);

  pinMode(SAMPLE_COMPLETE_PIN, OUTPUT);
  const esp_timer_create_args_t end_pulse_timer_args = {
    .callback = [](void *arg) { digitalWrite(SAMPLE_COMPLETE_PIN, LOW); },
    .arg = NULL,
    .name = "end-pulse"
  };
  esp_timer_create(&end_pulse_timer_args, &end_pulse_timer);

#ifdef BEEPER
  ledcAttachPin(BEEPER_PIN, BEEPER_CHANNEL);
  ledcSetup(BEEPER_CHANNEL, 1000, 8);
  ledcWrite(BEEPER_CHANNEL, 0);
#endif

#ifdef WIFI
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  server.begin();
  server.onClient(on_connect, NULL);
#endif
}

void loop() {
  scpi.ProcessInput(Serial, "\n");
  if (packet_size == 0 || !check_parity(packet, packet_size)) {
    return;
  }

  switch (packet[0]) {
    case 0x67:
      if (packet_size == 9 && decode_reading_packet(packet, &reading, &range)) {
       if (wanted_samples != 0) {
          String x = notation(reading, function);
          Serial.println(x);
#ifdef WIFI
          for (AsyncClient *client : clients) {
            client->write(x.c_str(), x.length());
          }
#endif
        }
        if (wanted_samples > 0) {
          wanted_samples--;
        }
      }
      break;
    case 0xE5:
      if (packet_size == 7) {
        decode_status_packet(packet, &function, &speed, &trigger_mode);
      }
      break;
    case 0x61:
      if (packet_size == 2 && packet[1] == 0xE0) {
        // Official GPIB card uses a 2.5us pulse
        // Left at 1ms for testing purposes
        digitalWrite(SAMPLE_COMPLETE_PIN, HIGH);
        esp_timer_start_once(end_pulse_timer, 1000);
      }
      break;
    default:
      break;
  }

  packet_size = 0;
}