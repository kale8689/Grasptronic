 
  ////// SOFTWARE PROTOTYPE START /////////

// Potentiometer-controlled Stepper Motor with Switch and LEDs
// Pin definitions for Software Prototype
#define red_LED 5                // Red LED illuminate for Unlimited Mode
#define green_LED 18             // Green LED illuminate for AGFM
#define switch_pin 22            // Switch input for proof of concept mode toggling
#define dir_pin 16               // Dir pin for EasyDriver
#define step_pin 17              // Step pin for EasyDriver
#define potPin 13                // Potentiometer input

// Configuration for Software Prototype
const int stepsPerRevolution = 200;  // Corresponds to 1.8 degrees per step
const int maxSteps = 400;        // Maximum steps from center position
const int deadZone = 25;         // Dead zone around center (reduce if motor doesn't respond at edges)
const int stepDelay = 800;       // Microseconds between steps (speed control)
const int threshold_Impedance =  0.1  // Impedance value that differentiates needle from tissue/suture/mesh
const int material = "";         // Variable that will change based on impedance reading

int currentPos = 0;              // Current motor position
int targetPos = 0;               // Target position from pot

  ////// SOFTWARE PROTOTYPE END /////////

  ////// ELECTRICAL PROTOTYPE START /////////
/*
  AD5933 Auto-calibrate + Measure
  1) Calibrates gain using a known resistor.
  2) Measures unknown material using the computed gain.
*/
#include <Wire.h>
#include "AD5933.h"


#define START_FREQ  (10000)   // Start frequency in Hz
#define FREQ_INCR   (2000)    // Frequency increment in Hz
#define NUM_INCR    (50)      // Number of increments

// Put your known resistor value here (ohms)
const double KNOWN_RESISTOR_OHMS = 10.0;


double gain[NUM_INCR+1];  // What is this?
int phaseArr[NUM_INCR+1]; // What is this?

  ////// ELECTRICAL PROTOTYPE END /////////

void setup() {

  ////// SOFTWARE PROTOTYPE START /////////
  Serial.begin(115200);
  pinMode(red_LED, OUTPUT);
  pinMode(green_LED, OUTPUT);
  pinMode(switch_pin, INPUT);
  pinMode(step_pin, OUTPUT);
  pinMode(dir_pin, OUTPUT);

  ////// SOFTWARE PROTOTYPE END /////////

  ////// ELECTRICAL PROTOTYPE START /////////
 Wire.begin();
  Serial.begin(115200);
  while (!Serial) ; // wait for Serial on some boards
  Serial.println();
  Serial.println("AD5933 Auto-calibrate + Measure");
  Serial.println("Make sure known resistor is connected between VOUT and VIN.");
  Serial.print("Known resistor (ohms): ");
  Serial.println(KNOWN_RESISTOR_OHMS, 6);


  // AD5933 configuration
  if (!(AD5933::reset() &&
        AD5933::setInternalClock(true) &&
        AD5933::setStartFrequency(START_FREQ) &&
        AD5933::setIncrementFrequency(FREQ_INCR) &&
        AD5933::setNumberIncrements(NUM_INCR) &&
        AD5933::setPGAGain(PGA_GAIN_X1)))
  {
    Serial.println("Initialization failed!");
    while (true);
  }

  // Wait for user to confirm they have the known resistor connected
  Serial.println("\nPress ENTER to start calibration with the known resistor...");
  while (Serial.available() == 0) { delay(10); }
  while (Serial.available() > 0) Serial.read(); // clear


  calibrateWithKnownResistor();
  Serial.println("\nCalibration complete. Now measure unknowns. Sweeps start in 2s...");
  delay(2000);
}

  ////// ELECTRICAL PROTOTYPE END /////////

}

void loop() {

  ////// SOFTWARE PROTOTYPE START /////////

  // Read switch state and update LEDs
  int switch_state = digitalRead(switch_pin);
  
  // Read potentiometer (0-4095 on ESP32)
  int position = analogRead(potPin);
  
  // Map pot to position range: 0 to maxSteps*2
  // Fully counter-clockwise (0) = 0 steps
  // Fully clockwise (4095) = 800 steps (maxSteps*2)
  targetPos = map(position, 0, 4095, 0, maxSteps * 2);
  

// Mechanical switch utilized to change between modes
  if (switch_state == HIGH) {
    digitalWrite(green_LED, HIGH); // Illuminate green LED to indicate Automatic Grasping Force Mode
    digitalWrite(red_LED, LOW);    // Turn off red LED

     // Calculate steps needed
  int stepsToMove = targetPos - currentPos;
  
  // Only move if there's a difference
  if (stepsToMove != 0) {
    // Set direction
    if (stepsToMove > 0) {
      digitalWrite(dir_pin, HIGH);  // Clockwise
    } else {
      digitalWrite(dir_pin, LOW);   // Counter-clockwise
      stepsToMove = -stepsToMove;   // Make positive for loop
    }
    
    // Move one step toward target
    digitalWrite(step_pin, HIGH);
    delayMicroseconds(stepDelay);
    digitalWrite(step_pin, LOW);
    delayMicroseconds(stepDelay);
    
    // Update current position
    if (targetPos > currentPos) {
      currentPos++;
    } else {
      currentPos--;
    }

  } else {
    digitalWrite(red_LED, HIGH); // Illuminate red LED to indicate Unlimited mode
    digitalWrite(green_LED, LOW);
  }
  
  
  }
  
  delay(10);  // Small delay for stability

  ////// SOFTWARE PROTOTYPE END /////////

  ////// ELECTRICAL PROTOTYPE START /////////

measureUnknown();
  delay(5000); // pause between measurements
}


// Calibrate gain[] using KNOWN_RESISTOR_OHMS
void calibrateWithKnownResistor() {
  Serial.println("Starting calibration sweep...");


  int real[NUM_INCR+1], imag[NUM_INCR+1];


  // Use the library sweep to get raw data with known resistor
  if (!AD5933::frequencySweep(real, imag, NUM_INCR+1)) {
    Serial.println("Calibration sweep failed!");
    return;
  }


  for (int i = 0; i <= NUM_INCR; i++) {
    double magnitude = sqrt((double)real[i] * real[i] + (double)imag[i] * imag[i]);
    if (magnitude == 0.0) {
      gain[i] = 0.0;
      Serial.print("Freq ");
      Serial.print(START_FREQ + i * FREQ_INCR);
      Serial.println(" Hz: magnitude=0 -> gain=0 (bad)");
    } else {
      gain[i] = 1.0 / (magnitude * KNOWN_RESISTOR_OHMS);
      Serial.print("Freq ");
      Serial.print(START_FREQ + i * FREQ_INCR);
      Serial.print(" Hz: mag=");
      Serial.print(magnitude, 6);
      Serial.print(" -> gain=");
      Serial.println(gain[i], 12);
    }
  }
}


// Measure unknown using computed gain[]
void measureUnknown() {
  Serial.println("\nStarting measurement sweep on unknown...");


  int real[NUM_INCR+1], imag[NUM_INCR+1];


  if (!AD5933::frequencySweep(real, imag, NUM_INCR+1)) {
    Serial.println("Frequency sweep failed...");
    return;
  }


  int cfreq_kHz = START_FREQ / 1000;
  for (int i = 0; i <= NUM_INCR; i++, cfreq_kHz += FREQ_INCR / 1000) {
    Serial.print(cfreq_kHz);
    Serial.print(" kHz: R=");
    Serial.print(real[i]);
    Serial.print("/I=");
    Serial.print(imag[i]);


    double magnitude = sqrt((double)real[i] * real[i] + (double)imag[i] * imag[i]);
    double Z = 0.0;
    if (gain[i] > 0.0) {
      Z = 1.0 / (magnitude * gain[i]);
    } else {
      Z = NAN;
    }


    double phase_deg = atan2((double)imag[i], (double)real[i]) * 180.0 / PI;


    Serial.print("  |Z|=");
    if (isnan(Z)) Serial.print("NaN");
    else Serial.print(Z, 6);
    Serial.print(" ohms, Phase=");
    Serial.println(phase_deg, 2);
  }


  Serial.println("Measurement sweep complete!");


////// ELECTRICAL PROTOTYPE END /////////



}
