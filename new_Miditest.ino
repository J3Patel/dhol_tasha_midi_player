// Test playing a succession of MIDI files from the SD card.
// Example program to demonstrate the use of the MIDFile library
// Just for fun light up a LED in time to the music.
//
// Hardware required:
//  SD card interface - change SD_SELECT for SPI comms
//  3 LEDs (optional) - to display current status and beat.
//  Change pin definitions for specific hardware setup - defined below.

#include <SdFat.h>
#include <MD_MIDIFile.h>

#define USE_MIDI  1  // set to 1 to enable MIDI output, otherwise debug output

#if USE_MIDI // set up for direct MIDI serial output

#define DEBUG(s, x)
#define DEBUGX(s, x)
#define DEBUGS(s)
#define SERIAL_RATE 57600

#else // Don't use MIDI to allow printing debug statements

#define DEBUG(s, x)  do { Serial.print(F(s)); Serial.print(x); } while(false)
#define DEBUGX(s, x) do { Serial.print(F(s)); Serial.print(F("0x")); Serial.print(x, HEX); } while(false)
#define DEBUGS(s)    do { Serial.print(F(s)); } while (false)
#define SERIAL_RATE 57600

#endif // USE_MIDI


const uint8_t SD_SELECT = SS;

const uint16_t WAIT_DELAY = 2000; // ms

#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

const char *tuneList[] =
{
  "2.MID",
  "mid_new_2.MID",
  "new_manual.MID",  // simplest and shortest file
//  "midi3n.MID"
};

SDFAT	SD;
MD_MIDIFile SMF;

class TNJDevice {
  bool shouldPlayAllAvailable =  true;
  int *dPins;
  int _reqOnDelay, _reqOffDelay;
  int devices;
  bool dstat[20] = {off, off, off, off, off, off,off, off, off,off, off, off,off, off, off,off, off, off,off, off};
  int msAfterOn[20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  int msAfterOff[20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  int _concurrent = 1;
  bool on = LOW;
  bool off = HIGH;

public:
  TNJDevice(int d[], int dsize, int concurrent, int reqOnDelay, int reqOffDelay) {
    devices = dsize;
    dPins = d;
    _reqOnDelay = reqOnDelay;
    _reqOffDelay = reqOffDelay;
    _concurrent = concurrent;

  }

  void turnOnAny() {
    int c = 0;
    for( int i=0; i< devices; i++) {
      unsigned long cm = millis();
      
      if (dstat[i] == off && cm - msAfterOff[i] > _reqOffDelay) {
        msAfterOn[i] = cm;
        dstat[i] = on;
        digitalWrite(dPins[i], on);
        ++c;
        if (c>=_concurrent) {
          break;
        }
      }

    }
  }

  void setupPins() {
    for(int i=0;i<devices;i++) {
      pinMode(dPins[i], OUTPUT);
      digitalWrite(dPins[i], off);
    }
  }

  void turnOffAll() {
    unsigned long cm = millis();
    for( int i=0; i< devices; i++) {
      digitalWrite(dPins[i], off);
      msAfterOff[i] = cm;
      dstat[i] = off;
    }
  }

  void _checkForturnOffOthers() {
    for( int i=0; i< devices; i++) {
      unsigned long cm = millis();
      if (dstat[i] == on && cm - msAfterOn[i] > _reqOnDelay) {
        msAfterOff[i] = cm;
        dstat[i] = off;
        digitalWrite(dPins[i], off);
        
      }

    }
  }

  // THIS MUST BE CALLED IN LOOP TO UPDATE OTHER SOLENOIDS
  void updateDevice() {
    _checkForturnOffOthers();
  }

  void testAllDevices() {
    for( int i=0; i< devices; i++) {
        digitalWrite(dPins[i], on);
        delay(50);
        digitalWrite(dPins[i], off);
        delay(1000);
    }
  }
};


void midiCallback(midi_event *pev)
  
// Called by the MIDIFile library when a file event needs to be processed
// thru the midi communications interface.
// This callback is set up in the setup() function.
{
  
  #if USE_MIDI
  int note = pev->data[1];
  int velocity = pev->data[2];
  bool shouldPlay = velocity != 64;

if (note == 53) {
     playBell(shouldPlay);
  }
  
  if (note == 39) {
    playTasha(shouldPlay);
  }

  if (note == 36) {
    playDhol(shouldPlay);
  }

  

//  
//  
//  switch(note) {
//    case 39:
//         
//    break;
//
//    case 36:
//         
//    break;
//
//    case 53:
//        
//    break;
//    
//    case 65:
//         playDhol(shouldPlay);
//    break;
//   case 63:
//          playTasha(shouldPlay);
//    break;
//    case 61:
//          playTasha2(shouldPlay);
//    break;
//    default:
//         return;
//    break;
//  }
  #else
  DEBUG("\n", millis());
  DEBUG("\tM T", pev->track);
  DEBUG(":  Ch ", pev->channel+1);
  DEBUGS(" Data");
  int note = pev->data[1];
  int velocity = pev->data[2];
  DEBUG(" ", note);
  DEBUG(" ", velocity);
  #endif
  
}

void sysexCallback(sysex_event *pev)
// Called by the MIDIFile library when a system Exclusive (sysex) file event needs
// to be processed through the midi communications interface. Most sysex events cannot
// really be processed, so we just ignore it here.
// This callback is set up in the setup() function.
{
  DEBUG("\nS T", pev->track);
  DEBUGS(": Data");
  for (uint8_t i=0; i<pev->size; i++)
  DEBUGX(" ", pev->data[i]);
}

void midiSilence(void)
// Turn everything off on every channel.
// Some midi files are badly behaved and leave notes hanging, so between songs turn
// off all the notes and sound
{
  midi_event ev;

  // All sound off
  // When All Sound Off is received all oscillators will turn off, and their volume
  // envelopes are set to zero as soon as possible.
  ev.size = 0;
  ev.data[ev.size++] = 0xb0;
  ev.data[ev.size++] = 120;
  ev.data[ev.size++] = 0;

  for (ev.channel = 0; ev.channel < 16; ev.channel++)
  midiCallback(&ev);
}

int ta[] = { 28,29, 30,31, 32,33,34,35, 6,10,7,8, 9,11,12,22, 23,24,25,18};//
TNJDevice tasha(ta, 20, 5, 50,20);

int ta2[] = {26,27,3, 5};
TNJDevice tasha2(ta2, 4, 2 , 30,30);
//
//int ta3[] = {11,12,30,31,32,33,34,35};
//TNJDevice tasha3(ta3, 8, 50,50);

int dh[] = {44,45};
TNJDevice dhol(dh, 2,2, 100,100);

int bl[] = {38, 39};
TNJDevice bell(bl,2,1,30,40);

void playBell(bool shouldPlay) {
  if(shouldPlay) {
    bell.turnOnAny();
  }
}

void playDhol(bool shouldPlay) {
  if(shouldPlay) {
    dhol.turnOnAny();  
  }
}

void playTasha(bool shouldPlay) {
  if(shouldPlay) {
    tasha.turnOnAny();  
    tasha2.turnOnAny();  
  }
}
void playTasha2(bool shouldPlay) {
  if(shouldPlay) {
    tasha2.turnOnAny();  
  }
}



void setup(void)
{
  pinMode(50,INPUT);

  tasha.setupPins();
  tasha2.setupPins();
  bell.setupPins();
  dhol.setupPins();

  Serial.begin(SERIAL_RATE);

  DEBUGS("\n[MidiFile Play List]");

  // Initialize SD
  if (!SD.begin(SD_SELECT, SPI_FULL_SPEED))
  {
    DEBUGS("\nSD init fail!");

    while (true) ;
  }

    dhol.turnOffAll();
    tasha.turnOffAll();
    tasha2.turnOffAll();
    bell.turnOffAll();

//    delay(1000);
//    tasha.testAllDevices();
    
  // Initialize MIDIFile
  SMF.begin(&SD);
  SMF.setMidiHandler(midiCallback);
  SMF.setSysexHandler(sysexCallback);

  
}

void tickMetronome(void)
// flash a LED to the beat
{
  static uint32_t lastBeatTime = 0;
  static boolean  inBeat = false;
  uint16_t  beatTime;

  beatTime = 60000/SMF.getTempo();    // msec/beat = ((60sec/min)*(1000 ms/sec))/(beats/min)
  if (!inBeat)
  {
    if ((millis() - lastBeatTime) >= beatTime)
    {
      lastBeatTime = millis();
      inBeat = true;
    }
  }
  else
  {
    if ((millis() - lastBeatTime) >= 100)	// keep the flash on for 100ms only
    {
      inBeat = false;
    }
  }
}

bool allow = false;

void loop(void)
{
//  allow = digitalRead(50);
//  if (!allow) {
//    return;
//  }
  
  static enum { S_IDLE, S_PLAYING, S_END, S_WAIT_BETWEEN } state = S_IDLE;
  static uint16_t currTune = ARRAY_SIZE(tuneList);
  static uint32_t timeStart;

  dhol.updateDevice();
  tasha.updateDevice();
  tasha2.updateDevice();
  bell.updateDevice();

  switch (state)
  {
    case S_IDLE:    // now idle, set up the next tune
    {
      int err;

      DEBUGS("\nS_IDLE");

      currTune++;
      if (currTune >= ARRAY_SIZE(tuneList))
      currTune = 0;

      // use the next file name and play it
      DEBUG("\nFile: ", tuneList[currTune]);
      err = SMF.load(tuneList[currTune]);
      if (err != MD_MIDIFile::E_OK)
      {
        DEBUG(" - SMF load Error ", err);

        timeStart = millis();
        state = S_WAIT_BETWEEN;
        DEBUGS("\nWAIT_BETWEEN");
      }
      else
      {
        DEBUGS("\nS_PLAYING");
        state = S_PLAYING;
      }
    }
    break;

    case S_PLAYING: // play the file
    //    DEBUGS("\nS_PLAYING");
    if (!SMF.isEOF())
    {
      if (SMF.getNextEvent())
      tickMetronome();
    }
    else
    state = S_END;
    break;

    case S_END:   // done with this one
    DEBUGS("\nS_END");
    SMF.close();
    midiSilence();
    timeStart = millis();
    state = S_WAIT_BETWEEN;
    dhol.turnOffAll();
    tasha.turnOffAll();
    tasha2.turnOffAll();
    bell.turnOffAll();

    DEBUGS("\nWAIT_BETWEEN");
    break;

    case S_WAIT_BETWEEN:    // signal finished with a dignified pause
    //    digitalWrite(READY_LED, HIGH);
    if (millis() - timeStart >= WAIT_DELAY)
    state = S_IDLE;
    break;

    default:
    state = S_IDLE;
    break;
  }
}
