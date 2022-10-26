#include "scpi.h"

#define PARAMETER_CHECK(x) \
  if (parameters.Size() < x) { \
    interface.println("-109,Missing parameter"); \
    return; \
  } \
  if (parameters.Size() > x) { \
    interface.println("-108,Parameter not allowed"); \
    return; \
  }

#define CALLBACK_ARGUMENTS SCPI_C commands, SCPI_P parameters, Stream &interface

// TODO: can fasely return true
bool compare_command(String a, String command) {
  return (command == a || command.startsWith(a));
}

ScpiParser::ScpiParser(DmmInterface &dmm, float &reading, Function &function, Range &range)
  : _dmm(dmm),
    _reading(reading),
    _function(function),
    _range(range) {

  std::function<void(CALLBACK_ARGUMENTS)> Identify = [this](CALLBACK_ARGUMENTS) {
    PARAMETER_CHECK(0);
    interface.println("Agilent,34405A,00000000,v0.0.1");
  };
  std::function<void(CALLBACK_ARGUMENTS)> OPC = [this](CALLBACK_ARGUMENTS) {
    PARAMETER_CHECK(0);
    interface.println("1");
  };
  std::function<void(CALLBACK_ARGUMENTS)> Reset = [this](CALLBACK_ARGUMENTS) {
    PARAMETER_CHECK(0);
    interface.println("1");
    wanted_samples = 0;
  };
  std::function<void(CALLBACK_ARGUMENTS)> Init = [this](CALLBACK_ARGUMENTS) {
    PARAMETER_CHECK(0);
    wanted_samples = 0;
  };
  std::function<void(CALLBACK_ARGUMENTS)> Abort = [this](CALLBACK_ARGUMENTS) {
    PARAMETER_CHECK(0);
    interface.println("1");
    wanted_samples = 0;
  };
  std::function<void(CALLBACK_ARGUMENTS)> Measure = [this](CALLBACK_ARGUMENTS) {
    PARAMETER_CHECK(0);
  };
  std::function<void(CALLBACK_ARGUMENTS)> Read = [this](CALLBACK_ARGUMENTS) {
    PARAMETER_CHECK(0);
    wanted_samples = 1;
  };
  std::function<void(CALLBACK_ARGUMENTS)> Fetch = [this](CALLBACK_ARGUMENTS) {
    PARAMETER_CHECK(0);
    interface.println(notation(_reading, _function));
  };
  std::function<void(CALLBACK_ARGUMENTS)> SetFunction = [this](CALLBACK_ARGUMENTS) {
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
    _dmm.send(packet, 6);
  };
  std::function<void(CALLBACK_ARGUMENTS)> Config = [this](CALLBACK_ARGUMENTS) {
    PARAMETER_CHECK(0);
    interface.println(mode_names[_function] + " " + String(notation(pow(10, _range - 2) * 2)) + ",+1.00000E-06");
  };
  std::function<void(CALLBACK_ARGUMENTS)> SetRangeAuto = [this](CALLBACK_ARGUMENTS) {
    PARAMETER_CHECK(0);
    interface.println("1");

    uint8_t packet[6];
    encode_mode_packet(packet, 0, 1);
    _dmm.send(packet, 6);
  };

  // IEE 488.2 mandatory commands
  parser.RegisterCommand("*IDN?", Identify);
  parser.RegisterCommand("*OPC?", OPC);
  parser.RegisterCommand("*RST",  Reset);

  // Other
  parser.RegisterCommand("INITiate", Init);
  parser.RegisterCommand("ABORT",    Abort);
  parser.RegisterCommand("MEASure",  Measure);

  // Acquisition commands
  parser.RegisterCommand("READ?",  Read);
  parser.RegisterCommand("FETCh?", Fetch);

  // Function set comamnds
  parser.RegisterCommand("CONFigure:VOLTage:DC",  SetFunction);
  parser.RegisterCommand("CONFigure:VOLTage:AC",  SetFunction);
  parser.RegisterCommand("CONFigure:CURRent:DC",  SetFunction);
  parser.RegisterCommand("CONFigure:CURRent:AC",  SetFunction);
  parser.RegisterCommand("CONFigure:RESistance",  SetFunction);
  parser.RegisterCommand("CONFigure:FRESistance", SetFunction);
  parser.RegisterCommand("CONFigure?",            Config);

  // Non standard commands :(
  parser.RegisterCommand("VOLTage:DC:RANGE:AUTO?",  SetRangeAuto);
  parser.RegisterCommand("VOLTage:AC:RANGE:AUTO?",  SetRangeAuto);
  parser.RegisterCommand("CURRent:DC:RANGE:AUTO?",  SetRangeAuto);
  parser.RegisterCommand("CURRent:AC:RANGE:AUTO?",  SetRangeAuto);
  parser.RegisterCommand("RESistance:RANGE:AUTO?",  SetRangeAuto);
  parser.RegisterCommand("FRESistance:RANGE:AUTO?", SetRangeAuto);
}
