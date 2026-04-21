
// INCLUDES
#include "pitches.h"
#include <CapacitiveSensor.h>

// LEDS
#define LED_PIN 8
#define LED2_PIN 9

// STUFF FOR TOUCHPAD
#define COMMON_PIN 3  // The common pullup for all keys
#define BUZZER_PIN A4 // The output pin for the piezo buzzer
#define NUM_OF_SAMPLES                                                         \
  5 // Higher number = more delay but more consistent readings
#define CAP_THRESHOLD 50             // loop iteration count for RC charge
#define NUM_OF_KEYS 4                // Number of keys that are on the keyboard
#define CS(Y) CapacitiveSensor(3, Y) // create a capacitive sensor object

// C Major Scale
int notes[] = {NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4,
               NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5};
// define the pins that the keys are connected to:
CapacitiveSensor keys[] = {CS(4), CS(5), CS(6), CS(7)};
// define the passcode
int code[] = {0, 1, 3, 2, 1};
// store entered keys
int entry[] = {0, 0, 0, 0, 0};
// flag for keypress debouncing
int pressed;
// count of how many keys have been entered in the current attempt
int entry_count;

// STUFF FOR METAL DETECTOR
float freq_threshold = 8250.0f;

// globals for the metal detector
const uint8_t DETECT_PIN = 2;
const uint8_t MAGNET_PIN = 10;
const unsigned long SAMPLE_MS = 100;
volatile unsigned long pulseCount = 0;
unsigned long lastSampleMs = 0;
unsigned int detectCount = 0;
unsigned long loopCount = 0;
float avgFreq = 0;
const int metalSamples = 10;
float sampleArr[metalSamples] = {0};
int sampleIndex = 0;

float freqAvg = 0;

void serialInit() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  // clear screen + cursor home
  //Serial.print("\x1B[2J\x1B[H");
}

void touchPadInit() {
  // Turn off autocalibrate on all channels:
  for (int keyNumber = 0; keyNumber < NUM_OF_KEYS; ++keyNumber) {
    keys[keyNumber].set_CS_AutocaL_Millis(0xFFFFFFFF);
  }
  // Set the buzzer as an output:
  pinMode(BUZZER_PIN, OUTPUT);
  // Set the LED as an output and turn it off:
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  // initialize flag for keypress debouncing
  pressed = 0;
  // initialize entry count
  entry_count = 0;
}

void detectorISR() { pulseCount++; }

void metalDetectorInit() {
  // Set up the input from the 555 timer
  pinMode(DETECT_PIN, INPUT);
  // Set up the LED indicator
  pinMode(LED2_PIN, OUTPUT);
  digitalWrite(LED2_PIN, LOW);
  // Set up the magnet control pin (keep high to hold the ball initially)
  pinMode(MAGNET_PIN, OUTPUT);
  digitalWrite(MAGNET_PIN, HIGH);
  // Attach interrupt handler to count edges
  attachInterrupt(digitalPinToInterrupt(DETECT_PIN), detectorISR, RISING);
  // Disable interrupts to reset the edge count and sample timer
  noInterrupts();
  pulseCount = 0;
  lastSampleMs = millis();
  interrupts();
}

// do the rickroll
void neverGiveUp() {
    // send the secret message
    Serial.print("Never Gonna Give You Up\n");
  //indicate with led
  digitalWrite(LED2_PIN, HIGH);
  delay(5000);
  // drop the ball (DEBUGGING STILL)
  digitalWrite(MAGNET_PIN, LOW);
  //reenable magnet
  //digitalWrite(MAGNET_PIN, HIGH);
  //disable led
  //digitalWrite(LED2_PIN, LOW);
}

void touchPadLoop() {
  // flag for incorrect password entry
  int fail = 0;

  // check for incorrect password entry
  for (int i = 0; i < 5; ++i) {
    if (code[i] != entry[i]) {
      fail = 1;
    }
  }

  // handle the case where the password is entered correctly
  if (!fail) {

    // clear screen + cursor home
    //Serial.print("\x1B[2J\x1B[H");
    // print a success message
    //Serial.print("Password Entered Successfully!");

    // play a sound
    for (int i = 0; i < 8; ++i) {
      delay(100);
      tone(BUZZER_PIN, notes[i]);
    }
    delay(100);

    // turn the sound off
    noTone(BUZZER_PIN);

    // enable the LED
    digitalWrite(LED_PIN, HIGH);
    // wait
    //delay(10000);
    // disable the LED
    //digitalWrite(LED_PIN, LOW);

    // clear the entry buffer
    for (int i = 0; i < 5; ++i)
      entry[i] = 0;

    // reset entry count
    entry_count = 0;

    // clear screen + cursor home
    //Serial.print("\x1B[2J\x1B[H");
    // print a prompt  message
    //Serial.print("Enter the Password:\n");
  } else if (entry_count == 5) {
    // clear screen + cursor home
    //Serial.print("\x1B[2J\x1B[H");
    // print a failure message
    //Serial.print("Wrong Password...");
    // play a noise
    for (int i = 0; i < 10; ++i) {
      tone(BUZZER_PIN, notes[i % 2]);
      delay(100);
    }
    // turn the sound off
    noTone(BUZZER_PIN);
    delay(1000);
    entry_count = 0;
    // clear screen + cursor home
    //Serial.print("\x1B[2J\x1B[H");
    // print a success message
    //Serial.print("Enter the password:\n");
  }

  // piano key sensing loop
  int pressed_in_loop = 0;
  for (int keyNumber = 0; keyNumber < NUM_OF_KEYS; ++keyNumber) {

    // If the capacitance reading is greater than the threshold, play a note:
    if (keys[keyNumber].capacitiveSensor(NUM_OF_SAMPLES) > CAP_THRESHOLD) {
      pressed_in_loop = 1;

      // play the note associated with the key
      if (!pressed) {

        // register a keypress
        pressed = 1;
        ++entry_count;
        // play the sound according to the key pressed
        tone(BUZZER_PIN, notes[keyNumber]); // Plays the note corresponding to
                                            // the key pressed

        // push back the FIFO queue
        for (int q = 0; q < 4; ++q) {
          entry[q] = entry[q + 1];
        }

        // push the entered key onto the queue
        entry[4] = keyNumber;

        // debug which key was pressed
        //Serial.print(keyNumber);
        delay(50);
      }
    }
  }

  if (!pressed_in_loop) {
    pressed = 0;
    noTone(BUZZER_PIN);
  }
}

void metalDetectorLoop() {
  unsigned long now = millis();
  if (now - lastSampleMs >= SAMPLE_MS) {
    noInterrupts();
    loopCount = pulseCount;
    pulseCount = 0;
    interrupts();


    float measuredFreq = (float)loopCount / ((float)(now - lastSampleMs) / 1000.0f);
    if (measuredFreq < avgFreq - 1500.0f) {
        neverGiveUp();
      }

    sampleArr[sampleIndex] = measuredFreq;
    sampleIndex = (sampleIndex + 1) % metalSamples;
    for (int i = 0; i < metalSamples; ++i) {
      avgFreq += sampleArr[i];
    }
    avgFreq /= (float)metalSamples;


    //Serial.print("Freq (Hz): ");
    //Serial.println(measuredFreq, 2);
    //Serial.println(avgFreq, 2);
    //Serial.println(loopCount);
    lastSampleMs = now;


  }
}

void setup() {
  serialInit();
  touchPadInit();
  metalDetectorInit();
  delay(1000);
  // clear screen + cursor home
}

void loop() {
  touchPadLoop();
  metalDetectorLoop();
}
