/*
  N32B Hi Res Firmware v3.5.2
  MIT License

  Copyright (c) 2022 SHIK
*/

#include "functions.h"

void onUsbMessage(const midi::Message<128> &message)
{
  MIDICoreSerial.send(message);
  n32b_display.blinkDot(2);
}

void onSerialMessage(const midi::Message<128> &message)
{
  // MIDICoreUSB.sendControlChange(message.data1, message.data2, message.channel);
  if (MIDICoreSerial.getType() != midi::MidiType::ActiveSensing)
  {
    MIDICoreUSB.send(message.type, message.data1, message.data2, message.channel);
    n32b_display.blinkDot(2);
  }
}

void updateKnob(uint8_t index, bool inhibit)
{
  if (
      (knobValues[index][0] != knobValues[index][1]) &&
      (knobValues[index][0] != knobValues[index][2]) &&
      (knobValues[index][0] != knobValues[index][3]))
  {
    uint16_t shiftedValue = map(knobValues[index][0], 0, 1019, 0, 16383);
    uint8_t MSBValue = shiftedValue >> 7;
    uint8_t LSBValue = lowByte(shiftedValue) >> 1;
    Knob_t &currentKnob = activePreset.knobInfo[index];

    if (!inhibit)
    {
      switch (currentKnob.MODE)
      {
      case 0:
        sendCCMessage(currentKnob, MSBValue, LSBValue);
        break;

      case 2:
        sendNRPM(currentKnob, MSBValue, LSBValue);
        break;

      default:
        break;
      }
    }

    knobValues[index][3] = knobValues[index][2];
    knobValues[index][2] = knobValues[index][1];
    knobValues[index][1] = knobValues[index][0];
  }
}

void sendCCMessage(const struct Knob_t &currentKnob, uint8_t MSBvalue, uint8_t LSBvalue)
{
  // Serial.println("sendCCMessage");
  // Serial.print("currentKnob.MSB: ");
  // Serial.println(currentKnob.MSB);
  // Serial.print("currentKnob.LSB: ");
  // Serial.println(currentKnob.LSB);
  // Serial.print("currentKnob.CHANNEL: ");
  // Serial.println(currentKnob.CHANNEL);
  // Serial.print("currentKnob.highResolution: ");
  // Serial.println(currentKnob.highResolution);
  // Serial.print("MSBvalue: ");
  // Serial.println(MSBvalue);
  // Serial.print("LSBvalue: ");
  // Serial.println(LSBvalue);
  // Serial.println("-------------");

  midi::Channel channel = currentKnob.CHANNEL > 0 && currentKnob.CHANNEL < 17 ? currentKnob.CHANNEL : activePreset.channel;

  if (currentKnob.highResolution)
  {
    // Serial.print("sendCC, channel: ");
    // Serial.println(channel);
    // Serial.println(">>>>>>>>>>>>>>>>>>>");

    MIDICoreSerial.sendControlChange(currentKnob.MSB, MSBvalue, channel);
    MIDICoreSerial.sendControlChange(currentKnob.LSB, LSBvalue, channel);

    MIDICoreUSB.sendControlChange(currentKnob.MSB, MSBvalue, channel);
    MIDICoreUSB.sendControlChange(currentKnob.LSB, LSBvalue, channel);
  }
  MIDICoreSerial.sendControlChange(currentKnob.MSB, MSBvalue, channel);
  MIDICoreUSB.sendControlChange(currentKnob.MSB, MSBvalue, channel);
  n32b_display.blinkDot(1);
}

void sendNRPM(const struct Knob_t &currentKnob, uint8_t MSBvalue, uint8_t LSBvalue)
{
  midi::Channel channel = currentKnob.CHANNEL > 0 && currentKnob.CHANNEL < 17 ? currentKnob.CHANNEL : activePreset.channel;

  MIDICoreSerial.sendControlChange(99, currentKnob.MSB & 0x7F, channel); // NRPN MSB
  MIDICoreUSB.sendControlChange(99, currentKnob.MSB & 0x7F, channel);    // NRPN MSB

  MIDICoreSerial.sendControlChange(98, currentKnob.LSB & 0x7F, channel); // NRPN LSB
  MIDICoreUSB.sendControlChange(98, currentKnob.LSB & 0x7F, channel);    // NRPN LSB

  MIDICoreSerial.sendControlChange(6, MSBvalue, channel); // Data Entry MSB
  MIDICoreUSB.sendControlChange(6, MSBvalue, channel);    // Data Entry MSB

  if (currentKnob.highResolution)
  {
    MIDICoreSerial.sendControlChange(38, MSBvalue, channel); // LSB for Control 6 (Data Entry)
    MIDICoreUSB.sendControlChange(38, MSBvalue, channel);    // LSB for Control 6 (Data Entry)
  }
  n32b_display.blinkDot(1);
}

void changeChannel(bool direction)
{
  if (direction)
  {
    // Next Channel
    if (activePreset.channel < 16)
      activePreset.channel++;
    else
      activePreset.channel = 1;
  }
  else
  {
    // Previous Channel
    if (activePreset.channel > 1)
      activePreset.channel--;
    else
      activePreset.channel = 16;
  }
}

void changePreset(bool direction)
{
  if (direction)
  {
    // Next Preset
    if (currentPresetNumber < 4)
      loadPreset(currentPresetNumber + 1);
    else
      loadPreset(0);
  }
  else
  {
    // Previous Preset
    if (currentPresetNumber > 0)
      loadPreset(currentPresetNumber - 1);
    else
      loadPreset(4);
  }
  // MIDICoreSerial.sendProgramChange(currentPresetNumber, 1);
  // MIDICoreUSB.sendProgramChange(currentPresetNumber, 1);
}

void buttonReleaseAction(bool direction)
{
  direction ? isPressingAButton = false : isPressingBButton = false;

  if (millis() - pressedTime < SHORT_PRESS_TIME)
  {
    if (isPresetMode)
    {
      changePreset(direction);
      n32b_display.showPresetNumber(currentPresetNumber, disableKnobs);
    }
    else
    {
      changeChannel(direction);
      n32b_display.showChannelNumber(activePreset.channel, disableKnobs);
    }
  }

  MIDICoreUSB.turnThruOn();
  MIDICoreSerial.turnThruOn();
}

void buttonPressAction(bool direction)
{
  pressedTime = millis();
  MIDICoreSerial.turnThruOff();
  MIDICoreUSB.turnThruOff();
}

void renderButtonFunctions()
{
  // Must call the loop() function first
  buttonA.loop();
  buttonB.loop();

  if (buttonA.isPressed())
  {
    isPressingAButton = true;
    buttonPressAction(1);
  }

  if (buttonB.isPressed())
  {
    isPressingBButton = true;
    buttonPressAction(0);
  }

  if (buttonA.isReleased())
  {
    buttonReleaseAction(1);
  }

  if (buttonB.isReleased())
  {
    buttonReleaseAction(0);
  }

  // Switch between channelMode and presetMode
  if (
      (isPressingAButton || isPressingBButton) &&
      (millis() - pressedTime > (unsigned int)(SHORT_PRESS_TIME << 2)))
  {
    if (isPressingAButton)
    {
      isPresetMode = false;
      n32b_display.showChannelNumber(activePreset.channel, disableKnobs);
    }
    if (isPressingBButton)
    {
      isPresetMode = true;
      n32b_display.showPresetNumber(currentPresetNumber, disableKnobs);
    }
  }
}

void doMidiRead()
{
  MIDICoreSerial.read();
  MIDICoreUSB.read();
}
