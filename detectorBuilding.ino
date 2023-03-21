#define RED_PIN 18
#define GREEN_PIN 15
#define BLUE_PIN 14
#define INPUT_PIN A7

#define ZERO_KG 0.70
#define ONE_KG 1.89

#define DELAY_TIME 20
#define ROLL_LEN 500

#define TARE_PIN 2
#define THRASH_PIN 3

bool red = false;
bool green = false;
bool blue = false;

long rollingVoltageSum = 0;
int rollingVoltageLocation = 0;
int rollingVoltages[ROLL_LEN];

double tareVoltageOffset = ZERO_KG;
double thrashVoltageOffset = ONE_KG;

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
  //gets the exact and rolling averaged voltage
  //a rolling average is used as a denoiser in lieu of a physical denoiser
  double exactVoltage, averageVoltage;
  getVoltage(&exactVoltage, &averageVoltage);

  //checks if it should tare or "thrash" the voltage
  tareThrash(averageVoltage);

  //calculates mass and acts upon it
  double mass = getMass(averageVoltage);
  setLights(mass);
  updateSerial(averageVoltage, mass, isStable(averageVoltage));
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

double getMass(double voltage) {
  //equation
  double mass = (voltage - tareVoltageOffset) / (thrashVoltageOffset - tareVoltageOffset) * 1000;
  
  //keeps in mind bounds of allowed masses in case something goes really wrong
  if (mass < 30) return 30;
  if (mass > 1000) return 1000;
  return mass;
}

void getVoltage(double* exact, double* average) {
  //calculates rolling voltage
  int inputIntVoltage = analogRead(INPUT_PIN);
  rollingVoltageSum -= rollingVoltages[rollingVoltageLocation];
  rollingVoltages[rollingVoltageLocation] = inputIntVoltage;
  rollingVoltageSum += inputIntVoltage;
  rollingVoltageLocation++;
  rollingVoltageLocation %= ROLL_LEN;

  //returns by reference the rolling average and exact voltage
  *exact = inputIntVoltage / 1023.0 * 2.56;
  
  *average = (((double) rollingVoltageSum) / ROLL_LEN) / 1023.0 * 2.56;
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
  //says that the voltage is stable if it has been at least 10 seconds since the voltage wavered
  //0.0119 V or about 10 g since it last did so
  if (voltage < lastBrokenVoltage - 0.0119 || voltage > lastBrokenVoltage + 0.0119) {
    lastBrokenVoltage = voltage;
    millisAtLastBreak = millis();
  }

  return (millis() - millisAtLastBreak > 10000);
}

void updateSerial(double voltage, double mass, boolean isStable) {
  //just sends a bunch of nice info
  /*char str[80];
  sprintf(
    str,
    " g\nRed: %s, Green: %s, Blue: %s\n",
    red ? "ON" : "OFF",
    green ? "ON" : "OFF",
    blue ? "ON" : "OFF"
  );*/
  Serial.print(F("\nVoltage: "));
  Serial.print(voltage, 6);
  Serial.print(F(" V"));
  Serial.print(F(", Mass: "));
  Serial.print(mass, 1);
  if (isStable) {
    Serial.print(F(" (stable)"));
  }
  //Serial.print(str);
}
