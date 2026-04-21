
#include "pitches.h"
#include <CapacitiveSensor.h>

#define LED_PIN 8
#define COMMON_PIN 2  // The common 'send' pin for all keys
#define BUZZER_PIN A4 // The output pin for the piezo buzzer
#define NUM_OF_SAMPLES                                                         \
  10 // Higher number whens more delay but more consistent readings
#define CAP_THRESHOLD                                                          \
  150 // Capactive reading that triggers a note (adjust to fit your needs)
#define NUM_OF_KEYS 4 // Number of keys that are on the keyboard

// This macro creates a capacitance "key" sensor object for each key on the
// piano keyboard:
#define CS(Y) CapacitiveSensor(2, Y)

// Each key corresponds to a note, which are defined here. Uncomment the scale
// that you want to use:
int notes[] = {NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4,
               NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5}; // C-Major scale
// int
// notes[]={NOTE_A4,NOTE_B4,NOTE_C5,NOTE_D5,NOTE_E5,NOTE_F5,NOTE_G5,NOTE_A5}; //
// A-Minor scale int
// notes[]={NOTE_C4,NOTE_DS4,NOTE_F4,NOTE_FS4,NOTE_G4,NOTE_AS4,NOTE_C5,NOTE_DS5};
// // C Blues scale

// Defines the pins that the keys are connected to:
CapacitiveSensor keys[] = {CS(3), CS(4), CS(5), CS(6)};

// define the passcode
int code[] = {0, 1, 3, 2, 1};

// store entered keys
int entry[] = {0, 0, 0, 0, 0};

// flag for keypress debouncing
int pressed = 0;
int entry_count = 0;

void setup() {

  // initialize serial debugging interface
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  } // helps on some boards/cores

  // clear screen + cursor home
  Serial.print("\x1B[2J\x1B[H");
  // print a success message
  Serial.print("Password Entered Successfully!");

  // Turn off autocalibrate on all channels:
  for (int keyNumber = 0; keyNumber < NUM_OF_KEYS; ++keyNumber) {
    keys[keyNumber].set_CS_AutocaL_Millis(0xFFFFFFFF);
  }

  // Set the buzzer as an output:
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  int pressed = 0;
}

void loop() {

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
    Serial.print("\x1B[2J\x1B[H");
    // print a success message
    Serial.print("Password Entered Successfully!");

    // play a sound
    for (int i = 0; i < 8; ++i) {
      delay(100);
      tone(BUZZER_PIN, notes[i]);
    }
    delay(100);

    // turn the sound off
    noTone(BUZZER_PIN);

    // enable the eLED
    digitalWrite(LED_PIN, HIGH);
    // wait
    delay(10000);
    // disable the LED
    digitalWrite(LED_PIN, LOW);

    // clear the entry buffer
    for (int i = 0; i < 5; ++i)
      entry[i] = 0;

    // reset entry count
    entry_count = 0;

    // clear screen + cursor home
    Serial.print("\x1B[2J\x1B[H");
    // print a success message
    Serial.print("Enter the Password:\n");
  } else if (entry_count == 5) {
    // clear screen + cursor home
    Serial.print("\x1B[2J\x1B[H");
    // print a success message
    Serial.print("Wrong Password...");
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
    Serial.print("\x1B[2J\x1B[H");
    // print a success message
    Serial.print("Enter the password:\n");
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
        Serial.print(keyNumber);
        delay(50);
      }
    }
  }

  if (!pressed_in_loop) {
    pressed = 0;
    noTone(BUZZER_PIN);
  }
}
