#define RED_PIN 18
#define GREEN_PIN 15
#define BLUE_PIN 14
#define INPUT_PIN A7

#define DELAY_TIME 20
#define ROLL_LEN 50

#define TARE_PIN 2
#define THRASH_PIN 3

#define DEBUG false

bool red = false;
bool green = false;
bool blue = false;

double averageVoltage;
double voltagesRecorded[ROLL_LEN];
int rollingIndex = 0;

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
  analogReference(INTERNAL);
}

void loop() {
  //gets the exact voltage
  double exactVoltage;
  getVoltage(&exactVoltage);

  //calculates mass and acts upon it
  double mass, angle;
  getMass(averageVoltage, &mass, &angle);
  setLights(mass);
  updateSerial(exactVoltage, angle, mass, isStable(exactVoltage), averageVoltage);
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
  double tempMass = 1239.64 * sin(0.988683 * voltage + 0.71261) - 150.32;

  //good enough for nows
  *angle = (voltage - 2.454) / (0.875 - 2.454) * PI/2;
  
  //keeps in mind bounds of allowed masses in case something goes really wrong
  #if !DEBUG
  if (tempMass < 30) tempMass = 30;
  if (tempMass > 1000) tempMass = 1000;
  #endif
  *mass = tempMass;
}

void getVoltage(double* exact) {
  //calculates rolling voltage
  int inputIntVoltage = analogRead(INPUT_PIN);

  //returns by reference the rolling average and exact voltage
  *exact = inputIntVoltage / 1023.0 * 2.56;

  averageVoltage += (*exact - voltagesRecorded[rollingIndex]) / ROLL_LEN;
  voltagesRecorded[rollingIndex] = *exact;
  rollingIndex = (rollingIndex + 1) % ROLL_LEN;
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

void updateSerial(double voltage, double angle, double mass, boolean isStable, double asymptotal) {
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
  Serial.print(asymptotal, 6);
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
