/* 
 * The code is released under the GNU General Public License.
 * Developed by Kristian Lauszus
 * This is a remote translator based on an ATtiny85. 
 * It translates commands from a Panasonic remote to specific JVC commands. 
 * This enables me to turn my JVC stereo on and off, mute it and turn the volume up or down, using my Panasonic remote for my TV.
 * For details, see http://blog.tkjelectronics.dk
 */

#include "IRremote.h"

#define JVCPower              0xC5E8
#define JVCMute               0xC538
#define JVCVolumeUp           0xC578
#define JVCVolumeDown         0xC5F8

//The panasonic protocol is 48-bits long, all the commands has the same address (or pre data) and then followed by a 32 bit code
#define PanasonicAddress      0x4004     // Panasonic address (Pre data)
#define PanasonicPower        0x100BCBD  // Panasonic Power button
#define PanasonicPower2       0x1000E0F  // Red button - alternative power button
#define PanasonicMute         0x1008E8F  // Green button
#define PanasonicVolumeDown   0x1004E4F  // Yellow
#define PanasonicVolumeUp     0x100CECF  // Blue button
#define ATtinyPower           0x190CB5A  // Second power button

#define IRrecv PB2 // pin 7 on ATtiny85

IRsend irsend;

// The amount of time between each measurement in microseconds
#define RESOLUTION 10

// the maximum pulse we'll listen for - maximum pulse is 3502us, so 5000us should be way over the top
#define MAXPULSE (5000/RESOLUTION)

// What percent we will allow in variation to match the same code - this large tolerance will ensure that the data is decoded correct even at a far distance
#define TOLERANCE 40 // 40 % tolerance

// we will store up to 50 pulse pairs  - the Panasonic protocol is 48 bits long or 96 pairs excluding header (2 pairs)
#define commandLength 50
uint16_t pulses[commandLength][2];  // pair is high and low pulse

#define RED PB0 // pin 5 on ATtiny85

uint16_t IRaddress;
uint32_t IRdata;
boolean deactivated;

void setup(void) 
{
  irsend.enableIROut(38); // Enable phase-correct PWM with a frequency of 38kHz
  DDRB |= _BV(RED); // Set as output
  DDRB &= ~(_BV(IRrecv)); // Set as input

  PORTB |= _BV(RED); // Turn LED on
  delay(100);
  PORTB &= ~(_BV(RED)); // Turn LED off 
}

void loop(void) {  
  if(listenForIR())
  {
    if(decodeIR() && IRaddress == PanasonicAddress)
    {
      if(deactivated)
      {
        if(IRdata == ATtinyPower)
        {
          deactivated = false;
          PORTB &= ~(_BV(RED)); // Turn LED off
          delay(250); // delay insures that it's just don't toggle "deactivate" very fast
        }
      }
      else
      {
        switch(IRdata)
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
          delay(3000); // On Panasonic TVs one have to hold the power button to turn the TV on, this delay keeps the ATtinyRemote from toggling the JVC stereo on and off
          break;
        case PanasonicPower2:
          JVCCommand(JVCPower);
          break;
        case ATtinyPower: // This will disable the ATtinyRemote until the button is pressed again
          deactivated = true;
          PORTB |= _BV(RED); // Turn LED on
          delay(250); // delay insures that it's just don't toggle "deactivate" very fast
          break;
        default:
          break;     
        }
      }
    }
  }
}

void JVCCommand(uint16_t data)
{
  irsend.sendJVC(data, 16,0); // hex value, 16 bits, no repeat
  delayMicroseconds(50); // see http://www.sbprojects.com/knowledge/ir/jvc.php for information
  irsend.sendJVC(data, 16,1); // hex value, 16 bits, repeat
}

boolean MATCH(uint16_t measured, uint16_t desired) // True if the condition are met
{
  measured *= RESOLUTION;
  return (measured >= desired-((float)desired*TOLERANCE/100) && measured <= desired+((float)desired*TOLERANCE/100));  
}

boolean decodeIR(void) {  
  unsigned long long data = 0;

  if(!MATCH(pulses[0][1],PANASONIC_HDR_MARK))
    return false;
  if(!MATCH(pulses[0+1][0],PANASONIC_HDR_SPACE))
    return false;
  for (int i=0; i < PANASONIC_BITS; i++) 
  {
    if(!MATCH(pulses[i+1][1],PANASONIC_BIT_MARK))
    {
      if(!MATCH(pulses[i+1][1],PANASONIC_BIT_MARK-252)) // Small hack to make it work at a far distance
        return false;
    }
    if(MATCH(pulses[i+2][0],PANASONIC_ONE_SPACE))
      data = (data << 1) | 1;
    else if (MATCH(pulses[i+2][0],PANASONIC_ZERO_SPACE))
      data <<= 1;
    else // It's neither 0 or 1, so something went wrong
      return false;
  }
  IRaddress = (uint16_t)(data >> 32);
  IRdata = (uint32_t)data;
  return true;
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

    if(currentpulse == commandLength) // Exit when the full command has been received
      return true;
  }
}

