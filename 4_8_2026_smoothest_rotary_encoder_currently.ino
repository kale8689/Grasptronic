

// MightyZap
#include <ESP32Servo.h>
#include <ESP32RotaryEncoder.h>


Servo myServo;
int CLK = 18;
int DT = 19;
int motor_pin = 14;
RotaryEncoder encoder(CLK, DT);


// 900 - 2100 full extention
const int MIN_PULSE = 1200; // extend longer (1325)
const int MAX_PULSE = 1270; // extend shorter (1175)
int pulse = 1235;


// Function that cause 3 us change in PWM per click on encoder
void onTurned(long value) {

  static long lastValue = 0;

  if (value > lastValue) {
    pulse += 5;   // one step only
  } 
  else if (value < lastValue) {
    pulse -= 5;
  }

  lastValue = value;

  pulse = constrain(pulse, 1200, 1270);

  myServo.writeMicroseconds(pulse);

  Serial.print("Pulse: ");
  Serial.println(pulse);
}


void setup() {

  Serial.begin(115200);
  delay(1000);

  // knows servo is on
  myServo.attach(motor_pin);

  // turns the encoder on
  encoder.onTurned(onTurned);
  encoder.setEncoderType(EncoderType::HAS_PULLUP);
  encoder.setBoundaries(-7, 7, false);
  encoder.begin();
}



void loop() {

  // encoder tells MightyZap what to do
  encoder.loop();

  Serial.print("Pulse: ");
  Serial.println(pulse);
  }
 


