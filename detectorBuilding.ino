#define RED_PIN 18
#define GREEN_PIN 15
#define BLUE_PIN 14
#define INPUT_PIN A7

#define ZERO_KG 2.390
#define ONE_KG 4.017

#define PROP_CONSTANT 1000.0

#define DELAY_TIME 20
#define ROLL_LEN 500

#define TARE_PIN 2
#define THRASH_PIN 3

#define DEBUG false

bool red = false;
bool green = false;
bool blue = false;

double tareVoltageOffset = ZERO_KG;
double thrashVoltageOffset = ONE_KG;

double propConstant = PROP_CONSTANT;

double lastBrokenVoltage;
unsigned long millisAtLastBreak;

void setup() {
  Serial.begin(9600);
  // put your setup code here, to run once:
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(INPUT_PIN, INPUT);
  pinMode(TARE_PIN, INPUT_PULLUP);
  pinMode(THRASH_PIN, INPUT_PULLUP);
  analogReference(DEFAULT);
}

void loop() {
  //gets the exact voltage
  double exactVoltage;
  getVoltage(&exactVoltage);

  //checks if it should tare or "thrash" the voltage
  tareThrash(exactVoltage);

  //calculates mass and acts upon it
  double mass, angle;
  getMass(exactVoltage, &mass, &angle);
  setLights(mass);
  updateSerial(exactVoltage, angle, mass, isStable(exactVoltage));
  delay(DELAY_TIME);
}

void setLights(double mass) {
  red = LOW;
  green = LOW;
  blue = LOW;
  if (mass < 100) {
    red = HIGH;
  }
  if (mass >= 100 && mass < 500) {
    green = HIGH;
  } 
  if (mass >= 500) {
    blue = HIGH;
  }
  digitalWrite(RED_PIN, red);
  digitalWrite(GREEN_PIN, green);
  digitalWrite(BLUE_PIN, blue);
}

void getMass(double voltage, double* mass, double* angle) {
  //equation
  *angle = (voltage - tareVoltageOffset) / (thrashVoltageOffset - tareVoltageOffset) * PI / 2;

  double tempMass = sin(*angle) * propConstant;
  
  //keeps in mind bounds of allowed masses in case something goes really wrong
  if (tempMass < 30) tempMass = 30;
  if (tempMass > 1000) tempMass = 1000;
  *mass = tempMass;
}

void getVoltage(double* exact) {
  //calculates rolling voltage
  int inputIntVoltage = analogRead(INPUT_PIN);

  //returns by reference the rolling average and exact voltage
  *exact = inputIntVoltage / 1023.0 * 5.00;
}

void tareThrash(double averageVoltage) {
  if (digitalRead(TARE_PIN) == LOW) {
    tareVoltageOffset = averageVoltage;
  }

  if (digitalRead(THRASH_PIN) == LOW) {
    thrashVoltageOffset = averageVoltage;
  }
}

boolean isStable(double voltage) {
  //says that the voltage is stable if it has been at least 3 seconds since the voltage wavered
  //0.0119 V or about 10 g since it last did so
  if (voltage < lastBrokenVoltage - 0.0119 || voltage > lastBrokenVoltage + 0.0119) {
    lastBrokenVoltage = voltage;
    millisAtLastBreak = millis();
  }

  return (millis() - millisAtLastBreak > 3000);
}

void updateSerial(double voltage, double angle, double mass, boolean isStable) {
  //just sends a bunch of nice info
  #if DEBUG
  char str[80];
  sprintf(
    str,
    " Red: %s, Green: %s, Blue: %s",
    red ? " ON" : "OFF",
    green ? " ON" : "OFF",
    blue ? " ON" : "OFF"
  );
  #endif
  Serial.print(F("\nVoltage: "));
  Serial.print(voltage, 6);
  Serial.print(F(" V"));
  #if DEBUG
  Serial.print(F(", Angle: "));
  Serial.print(angle, 6);
  Serial.print(F(" rad"));
  #endif
  Serial.print(F(", Mass: "));
  Serial.print(mass, 
  #if DEBUG
  6
  #else
  1
  #endif
  );
  Serial.print(F(" g"));
  #if DEBUG
  Serial.print(str);
  #endif
  if (isStable) {
    Serial.print(F(" (stable)"));
  }
}
