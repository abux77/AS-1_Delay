/* 
_____/\\\\\\\\\________/\\\\\\\\\\\______________________/\\\_        
 ___/\\\\\\\\\\\\\____/\\\/////////\\\________________/\\\\\\\_       
  __/\\\/////////\\\__\//\\\______\///________________\/////\\\_      
   _\/\\\_______\/\\\___\////\\\__________/\\\\\\\\\\\_____\/\\\_     
    _\/\\\\\\\\\\\\\\\______\////\\\______\///////////______\/\\\_    
     _\/\\\/////////\\\_________\////\\\_____________________\/\\\_   
      _\/\\\_______\/\\\__/\\\______\//\\\____________________\/\\\_  
       _\/\\\_______\/\\\_\///\\\\\\\\\\\/_____________________\/\\\_ 
        _\///________\///____\///////////_______________________\///_ 

5 Knob Feedback Delay Arcade Sounds

POT 1 = TRIANGLE WAVE OCSILATOR 1 FREQUENCY
POT 2 = TRIANGLE WAVE OCSILATOR 1 FEEDBACK - TO +
POT 3 = FEEDBACK DELAY TIME
POT 4 = TRIANGLE WAVE OCSILATOR 2 FREQUENCY
POT 5 = TRIANGLE WAVE OCSILATOR 2 FEEDBACK - TO +
     ______________________________
   /     ____              ____     \
  |     /    \            /    \     |
  |    ( POT1 )          ( POT4 )    |
  |     \____/            \____/     |
  |               ____               |
  |              /    \              |
  |             ( POT3 )             |
  |              \____/              |
  |      ____              ____      |
  |     /    \            /    \     |
  |    ( POT2 )          ( POT5 )    |
  |     \____/            \____/     |
  |                                  |
   \ _______________________________/

Based on Mozzi example
AudioDelayFeedback
Tim Barrass 2013, CC by-nc-sa
http://sensorium.github.com/Mozzi/
  
Andrew Buckie 2018, CC by-nc-sa
*/

#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/triangle_analogue512_int8.h> // wavetable for audio
#include <tables/triangle512_int8.h> // wavetable for delay sweep
#include <AudioDelayFeedback.h>
#include <mozzi_midi.h> // for mtof
#include "SynthBoxConfig.h"

// Declare mapping variables
int Triangle1Freq;
const int MinTriangle1Freq = 22;
const int MaxTriangle1Freq = 440;
int Feedback1Level;
const int MinFeedback1Level = -128;
const int MaxFeedback1Level = 127;
int DelayFreq;
const int MinDelayFreq = 0;
const int MaxDelayFreq = 10;
int Triangle2Freq;
const int MinTriangle2Freq = 22;
const int MaxTriangle2Freq = 440;
int Feedback2Level;
const int MinFeedback2Level = -128;
const int MaxFeedback2Level = 127;

#define CONTROL_RATE 64 // powers of 2 please

Oscil<TRIANGLE_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aTriangle1(TRIANGLE_ANALOGUE512_DATA); // audio oscillator
Oscil<TRIANGLE_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aTriangle2(TRIANGLE_ANALOGUE512_DATA); // audio oscillator

Oscil<TRIANGLE512_NUM_CELLS, CONTROL_RATE> kDelSamps1(TRIANGLE512_DATA); // for modulating delay time, measured in audio samples
Oscil<TRIANGLE512_NUM_CELLS, CONTROL_RATE> kDelSamps2(TRIANGLE512_DATA); // for modulating delay time, measured in audio samples

AudioDelayFeedback <128> aDel1;
AudioDelayFeedback <128> aDel2;

// the delay time, measured in samples, updated in updateControl, and used in updateAudio 
unsigned int del_samps1;
unsigned int del_samps2;

void setup(){
  startMozzi(CONTROL_RATE);
  Serial.begin(115200); // For Debugging of values
}

void updateControl(){

  // Read digital inputs
  ExpPot1 = digitalRead(DIP_1);   // DIP switch 1 - 1 = Off / 0 = On
  ExpPot2 = digitalRead(DIP_2);   // DIP switch 2 - 1 = Off / 0 = On
  ExpPot3 = digitalRead(DIP_3);   // DIP switch 3 - 1 = Off / 0 = On
  ExpPot4 = digitalRead(DIP_4);   // DIP switch 4 - 1 = Off / 0 = On
  ExpPot5 = digitalRead(DIP_5);   // DIP switch 5 - 1 = Off / 0 = On
  ExpDetct = digitalRead(ExpPlug); // 1 = Expression pedal plugged in / 0 = Expression pedal unplugged

  // Map to expression pedal if selected
  if (ExpPot1 == 0 & ExpDetct ==1){
    Pot1 = Exp;    
  }
  else{
    Pot1 = Pot1Default;
  }
  if (ExpPot2 == 0 & ExpDetct ==1){
    Pot2 = Exp;    
  }
  else{
    Pot2 = Pot2Default;
  }
  if (ExpPot3 == 0 & ExpDetct ==1){
    Pot3 = Exp;    
  }
  else{
    Pot3 = Pot3Default;
  }
  if (ExpPot4 == 0 & ExpDetct ==1){
    Pot4 = Exp;    
  }
  else{
    Pot4 = Pot4Default;
  }
  if (ExpPot5 == 0 & ExpDetct ==1){
    Pot5 = Exp;    
  }
  else{
    Pot5 = Pot5Default;
  }
  
  // Read all the analog inputs
  Pot1Val = mozziAnalogRead(Pot1); // value is 0-1023
  Pot2Val = mozziAnalogRead(Pot2); // value is 0-1023
  Pot3Val = mozziAnalogRead(Pot3); // value is 0-1023
  Pot4Val = mozziAnalogRead(Pot4); // value is 0-1023
  Pot5Val = mozziAnalogRead(Pot5); // value is 0-1023
  ExpVal  = mozziAnalogRead(Exp);  // value is 0-1023

  // Map Values
  Triangle1Freq = map(Pot1Val, 0, 1023, MinTriangle1Freq, MaxTriangle1Freq);
  Feedback1Level = map(Pot2Val, 0, 1023, MinFeedback1Level, MaxFeedback1Level);
  DelayFreq = map(Pot3Val, 0, 1023, MinDelayFreq, MaxDelayFreq); // Use a float for the LFO sub 1hz frequencies
  Triangle2Freq = map(Pot4Val, 0, 1023, MinTriangle2Freq, MaxTriangle2Freq);
  Feedback2Level = map(Pot5Val, 0, 1023, MinFeedback2Level, MaxFeedback2Level);

  // Print Debug values
  Serial.print("Triangle1Freq = ");
  Serial.print(Triangle1Freq);
  Serial.print("   Feedback1Level = ");
  Serial.print(Feedback1Level);
  Serial.print("   DelayFreq = ");
  Serial.print(DelayFreq);
  Serial.print("   Triangle2Freq = ");
  Serial.print(Triangle2Freq);
  Serial.print("   Feedback2Level = ");
  Serial.print(Feedback2Level);
  Serial.println(); // Carriage return

  aTriangle1.setFreq(Triangle1Freq);
  kDelSamps1.setFreq(DelayFreq); // set the delay time modulation frequency (ie. the sweep frequency)
  aDel1.setFeedbackLevel(Feedback1Level); // can be -128 to 127

  aTriangle2.setFreq(Triangle2Freq);
  kDelSamps2.setFreq(DelayFreq); // set the delay time modulation frequency (ie. the sweep frequency)
  aDel2.setFeedbackLevel(Feedback2Level); // can be -128 to 127
  
  // delay time range from 0 to 127 samples, @ 16384 samps per sec = 0 to 7 milliseconds
  del_samps1 = 64+kDelSamps1.next();

  // delay time range from 1 to 33 samples, @ 16384 samps per sec = 0 to 2 milliseconds
  del_samps2 = 17+kDelSamps2.next()/8; 
}

int updateAudio(){
  char asig1 = aTriangle1.next(); // get this so it can be used twice without calling next() again
  int aflange1 = (asig1>>3) + aDel1.next(asig1, del_samps1); // mix some straignt signal with the delayed signal

  char asig2 = aTriangle2.next(); // get this so it can be used twice without calling next() again
  int aflange2 = (asig2>>3) + aDel2.next(asig2, del_samps2); // mix some straignt signal with the delayed signal
  return (aflange1 + aflange2)>>1;
  //return (aflange1);
}

void loop(){
  audioHook();
}




