#include "scpi.h"

#include "decode.h"
#include "frame.h"

unsigned long wanted_samples = 0;
SCPI_Parser scpi;

// from 8840a_io_card.ino
extern HardwareSerial dmm;
extern Function function;
extern Range range;
extern Speed speed;
extern float reading;
extern SCPI_Parser scpi;

#define PARAMETER_CHECK(x) \
  if (parameters.Size() < x) { \
    interface.println("-109,Missing parameter"); \
    return; \
  } \
  if (parameters.Size() > x) { \
    interface.println("-108,Parameter not allowed"); \
    return; \
  }

// TODO: can fasely return true
bool compare_command(String a, String command) {
  return (command == a || command.startsWith(a));
}

// IEE 488.2 "mandatory" commands
void Identify(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  PARAMETER_CHECK(0);
  interface.println("Agilent,34405A,00000000,v0.0.1");
}
void OPC(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  PARAMETER_CHECK(0);
  interface.println("1");
};
void Reset(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  PARAMETER_CHECK(0);
  interface.println("1");
  wanted_samples = 0;
}

// Other
void Init(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  PARAMETER_CHECK(0);
  wanted_samples = 0;
}
void Abort(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  PARAMETER_CHECK(0);
  interface.println("1");
  wanted_samples = 0;
}
void Measure(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  PARAMETER_CHECK(0);
}

// Acquisition commands
void Read(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  PARAMETER_CHECK(0);
  wanted_samples = 1;
}
void Fetch(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  PARAMETER_CHECK(0);
  interface.println(notation(reading, function));
}

// Function set comamnds
void SetFunction(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  PARAMETER_CHECK(0);

  uint8_t function;
  if (compare_command(commands[1], "VOLTage")) {
    function = compare_command(commands[2], "DC") ? 1 : 2;
  } else if (compare_command(commands[1], "CURRent")) {
    function = compare_command(commands[2], "DC") ? 5 : 6;
  } else if (compare_command(commands[1], "RESistance")) {
    function = 3;
  } else if (compare_command(commands[1], "FRESistance")) {
    function = 4;
  }

  uint8_t packet[6];
  encode_mode_packet(packet, function);
  send_packet(dmm, packet, 6);
}
void Config(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  PARAMETER_CHECK(0);
  interface.println(mode_names[function] + " " + String(notation(pow(10, range - 2) * 2)) + ",+1.00000E-06");
};

// Non standard commands :(
void SetRangeAuto(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  PARAMETER_CHECK(0);
  interface.println("1");

  uint8_t packet[6];
  encode_mode_packet(packet, 0, 1);
  send_packet(dmm, packet, 6);
}

void init_scpi() {
  // IEE 488.2 "mandatory" commands
  //scpi.RegisterCommand("*CLS",  &CLS);
  //scpi.RegisterCommand("*ESE",  &ESE);
  //scpi.RegisterCommand("*ESE?", &ESE);
  //scpi.RegisterCommand("*ESR?", &ESR);
  scpi.RegisterCommand("*IDN?", &Identify);
  //scpi.RegisterCommand("*OPC",  &OPC);
  scpi.RegisterCommand("*OPC?", &OPC);
  scpi.RegisterCommand("*RST",  &Reset);
  //scpi.RegisterCommand("*SRE",  &SRE);
  //scpi.RegisterCommand("*SRE?", &SRE);
  //scpi.RegisterCommand("*STB?", &STB);
  //scpi.RegisterCommand("*TST?", &TST);
  //scpi.RegisterCommand("*WAI",  &WAI);

  // Other
  scpi.RegisterCommand("INITiate", &Init);
  scpi.RegisterCommand("ABORT",    &Abort);
  scpi.RegisterCommand("MEASure",  &Measure);

  // Acquisition commands
  scpi.RegisterCommand("READ?",  &Read);
  scpi.RegisterCommand("FETCh?", &Fetch);

  // Function set comamnds
  scpi.RegisterCommand("CONFigure:VOLTage:DC",  &SetFunction);
  scpi.RegisterCommand("CONFigure:VOLTage:AC",  &SetFunction);
  scpi.RegisterCommand("CONFigure:CURRent:DC",  &SetFunction);
  scpi.RegisterCommand("CONFigure:CURRent:AC",  &SetFunction);
  scpi.RegisterCommand("CONFigure:RESistance",  &SetFunction);  
  scpi.RegisterCommand("CONFigure:FRESistance", &SetFunction);
  scpi.RegisterCommand("CONFigure?",            &Config);

  // Non standard commands :(
  scpi.RegisterCommand("VOLTage:DC:RANGE:AUTO?",  &SetRangeAuto);
  scpi.RegisterCommand("VOLTage:AC:RANGE:AUTO?",  &SetRangeAuto);
  scpi.RegisterCommand("CURRent:DC:RANGE:AUTO?",  &SetRangeAuto);
  scpi.RegisterCommand("CURRent:AC:RANGE:AUTO?",  &SetRangeAuto);
  scpi.RegisterCommand("RESistance:RANGE:AUTO?",  &SetRangeAuto);  
  scpi.RegisterCommand("FRESistance:RANGE:AUTO?", &SetRangeAuto);
}
