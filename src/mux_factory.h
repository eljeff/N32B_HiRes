/*
  N32B Hi Res Firmware v3.0.2
  MIT License

  Copyright (c) 2021 SHIK
*/

#include <Arduino.h>

#include "definitions.h"
#include "functions.h"

#ifndef MUX_FACTORY_h
#define MUX_FACTORY_h

class MUX_FACTORY
{
public:
    MUX_FACTORY();
    void init(uint8_t channel1, uint8_t channel2, uint8_t channel3, int8_t channel4);
    void setSignalPin(bool muxIndex, uint8_t pin);
    void update();

private:
    uint8_t currentChannel;
    uint8_t channels[4];
    uint8_t signalPin[2];
    unsigned long timeout;
    void read();
};

#endif