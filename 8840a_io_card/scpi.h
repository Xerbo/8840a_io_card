#ifndef SCPI_H_
#define SCPI_H_

#include <Vrekrer_scpi_parser.h>

#include "decode.h"
#include "frame.h"

class ScpiParser {
  public:
    ScpiParser(DmmInterface &dmm, float &reading, Function &function, Range &range);

    void ProcessInput(Stream &interface, const char* term_chars) {
      parser.ProcessInput(interface, term_chars);
    }
    void Execute(char* message, Stream& interface) {
      parser.Execute(message, interface);
    }

    int wanted_samples = 0;

    float &_reading;
    Function &_function;
    Range &_range;
  private:
    SCPI_Parser parser;
    DmmInterface &_dmm;
};

#endif
