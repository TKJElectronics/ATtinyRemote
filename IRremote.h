/*
 * IRremote
 * Version 0.1 July, 2009
 * Copyright 2009 Ken Shirriff
 * For details, see http://arcfn.com/2009/08/multi-protocol-infrared-remote-library.htm http://arcfn.com
 *
 * Interrupt code based on NECIRrcv by Joe Knapp
 * http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1210243556
 * Also influenced by http://zovirl.com/2008/11/12/building-a-universal-remote-with-an-arduino/
 *
 * Modified by Kristian Lauszus to work with ATTiny85
 * For details, see http://blog.tkjelectronics.dk
 */

#ifndef IRremote_h
#define IRremote_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#define IRLED PB1 // (Used as OC0B) - pin 6 on ATtiny85

// Ensures that the compiler knows it's running at 16MHz, if it's not defined in the makefile or not using the Arduino IDE
#ifndef F_CPU
#define F_CPU 16000000UL  // 16 MHz
#endif

// pulse parameters in usec - I tweaked the values for my setup
#define PANASONIC_HDR_MARK 3502
#define PANASONIC_HDR_SPACE 1750
#define PANASONIC_BIT_MARK 502-50
#define PANASONIC_ONE_SPACE 1244
#define PANASONIC_ZERO_SPACE 400+100

#define JVC_HDR_MARK 8000
#define JVC_HDR_SPACE 4000
#define JVC_BIT_MARK 600
#define JVC_ONE_SPACE 1600
#define JVC_ZERO_SPACE 550

#define PANASONIC_BITS 48
#define TOPBIT 0x80000000

class IRsend
{
public:
  IRsend() {}
  void sendJVC(uint32_t data, int16_t nbits, int16_t repeat); // *Note instead of sending the REPEAT constant if you want the JVC repeat signal sent, send the original code value and change the repeat argument from 0 to 1. JVC protocol repeats by skipping the header NOT by sending a separate code value like NEC does.
  void enableIROut(int16_t khz);
private:
  void mark(int16_t usec);
  void space(int16_t usec);
};

#endif
