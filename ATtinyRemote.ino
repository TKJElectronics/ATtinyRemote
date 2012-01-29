#include "IRremote.h"

#define JVCPower              0xC5E8
#define JVCMute               0xC538
#define JVCVolumeUp           0xC578
#define JVCVolumeDown         0xC5F8

//The panasonic protocol actually is 48-bits long, but we only use the bottom 32
#define PanasonicPower        0x1000E0F  // Red button
#define PanasonicMute         0x1008E8F  // Green button
#define PanasonicVolumeUp     0x100CECF  // Blue button
#define PanasonicVolumeDown   0x1004E4F  // Yellow

#define IRrecv PB2 // pin 7 on ATtiny85

IRsend irsend;

#define RESOLUTION 10

// the maximum pulse we'll listen for - maximum pulse is 3502us, so 5000us should be way over the top
#define MAXPULSE (5000/RESOLUTION)

// What percent we will allow in variation to match the same code
#define TOLERANCE 25 // 25% tolerance

// we will store up to 50 pulse pairs  - the Panasonic protocol is 48 bits long or 96 pairs excluding header (2 pairs)
#define commandLength 50
uint16_t pulses[commandLength][2];  // pair is high and low pulse

#define RED PB0

void setup(void) 
{
  irsend.enableIROut(38);
  DDRB |= _BV(RED); // Set as output
  DDRB &= ~(_BV(IRrecv)); // Set as input

  PORTB |= _BV(RED); // Turn LED on
  delay(1000);
  PORTB &= ~(_BV(RED)); // Turn LED off
  JVCCommand(JVCPower); // Turn stereo on at startup
}

void loop(void) {
  if(listenForIR())
  {
    switch(decodeIR())
    {      
    case PanasonicVolumeUp:   
      JVCCommand(JVCVolumeUp);        
      break;
    case PanasonicVolumeDown:    
      JVCCommand(JVCVolumeDown);  
      break;
    case PanasonicMute:
      JVCCommand(JVCMute);
      break;
    case PanasonicPower:
      JVCCommand(JVCPower);
      break;
    default:
      break;     
    }
  }
  PORTB ^= _BV(RED);
}

void JVCCommand(unsigned int data)
{
  irsend.sendJVC(data, 16,0); // hex value, 16 bits, not repeat
  for(byte i = 0; i < 10;i++)
    irsend.sendJVC(data, 16,1); // hex value, 16 bits, repeat   
}

boolean MATCH(int measured, int desired) // True if the condition are not met
{
  measured *= RESOLUTION;
  return(!(measured < desired-(desired*TOLERANCE/100) || measured > desired+(desired*TOLERANCE/100)));
}

unsigned long decodeIR(void) {  
  unsigned long long data = 0;

  if(!MATCH(pulses[0][1],PANASONIC_HDR_MARK))
    return false;
  if(!MATCH(pulses[0+1][0],PANASONIC_HDR_SPACE))
    return false;
  for (int i=0; i < PANASONIC_BITS; i++) 
  {
    if(!MATCH(pulses[i+1][1],PANASONIC_BIT_MARK))
      return false;
      
    if(MATCH(pulses[i+2][0],PANASONIC_ONE_SPACE))
      data = (data << 1) | 1;
    else if (MATCH(pulses[i+2][0],PANASONIC_ZERO_SPACE))
      data <<= 1;
    else // It's neither 0 or 1, so something went wrong
    return false;
  }
  return (unsigned long)data;
}

boolean listenForIR(void) {
  uint8_t currentpulse = 0; // index for pulses we're storing
  while (1) {    
    uint16_t highpulse, lowpulse;  // temporary storage timing
    highpulse = lowpulse = 0; // start out with no pulse length

    while (PINB & _BV(IRrecv)) {
      // pin is still HIGH
      highpulse++;// count off another few microseconds
      delayMicroseconds(RESOLUTION);
      // If the pulse is too long, we 'timed out' - either nothing
      // was received or the code is finished, so print what
      // we've grabbed so far, and then reset
      if (highpulse >= MAXPULSE && currentpulse != 0)
        return false;        
    }
    // we didn't time out so lets stash the reading
    pulses[currentpulse][0] = highpulse;

    // same as above
    while (!(PINB & _BV(IRrecv))) {
      // pin is still LOW
      lowpulse++;
      delayMicroseconds(RESOLUTION);
      if (lowpulse >= MAXPULSE && currentpulse != 0)
        return false;
    }
    pulses[currentpulse][1] = lowpulse;

    // we read one high-low pulse successfully, continue!
    currentpulse++;

    if(currentpulse >= commandLength) // Exit when the full command has been received
      return true;
  }
}

