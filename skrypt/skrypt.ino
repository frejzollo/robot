const int speedsetter = A0;
const int button = 13;
int mode=0;

int Sensors[7] = {A1, A2, A3, A4, A5, A6, A7};
int blackLevels[7]; //stany na linii
int whiteLevels[7]; //stany na powierzchni
int caliValues[7]; //skalibrowane
int analogValues[7]; // wartosci z sensorow

int readErrorBlack = 7; // podloga
int readErrorWhite = 7; // linia

void setup() {
  Serial.begin(9600);  // Uruchomienie komunikacji szeregowej
  for (int i = 0; i < 7; i++) {
    pinMode(analogPins[i], INPUT);
  }
  pinMode(speedsetter, INPUT);
  pinMode(button, INPUT);
}

void loop() {
    for (int i = 0; i < 7; i++) {
    analogValues[i] = analogRead(Sensors[i]);
  }
  //basic_info();
}

//informacja na serial: stan guzika / wartość pokrętła / [wartości sensorów] / tryb pracy (mode)
void basicInfo(){

  Serial.print(digitalRead(button));
  Serial.print(" / ");
  Serial.print(analogRead(speedsetter));
  Serial.print(" / [");
  for (int i = 0; i < 7; i++) {
    Serial.print(analogValues[i]);
    Serial.print(i < 6 ? ", " : "] / ");
  }
  Serial.print(mode);
  Serial.println();
  delay(1000);
}


