//Hardware - PINY
//--------------------------------------------------------------------------------------------------
const int speedsetter = A0;
const int button = 13;
int mode=0;
int analogPins[7] = {A1, A2, A3, A4, A5, A6, A7};
// Motor lewy
const int ENL = 5;
const int L2 = 7;
const int L1 = 6; //z tym pinem serial nie działa, można zmienić jak zależy na debugu
// Motor prawy
const int ENR = 10; // ^ -||-
const int R2 = 9;
const int R1 = 8;


//HARDWARE - ZMIENNE
//---------------------------------------------------------------------------------------------------
const int sensorsNumber = 7;
int sensorsInAirValue = 70;
int speedsetterMax = 150;
bool isSpeedSetter = true;

//BACK_EMF - zabezpieczenie do sterownika silników
bool doIGiveAFuck = true; //<------------------------------------------Jak false to wszystkie dodatkowe zabezpieczenia idą się jebać
int safetySpeedChange = 50;
int backEMFDelay = 30;

//SOFT
//----------------------------------------------------------------------------------------------------
// Wartości pomocnicze
int iteration = 0; //ile razy wykonano pętle loop()


// tabilce z czujnikami
int blackLevels[7]; //stany na linii
int whiteLevels[7]; //stany na powierzchni
int caliValues[7]; //skalibrowane
int analogValues[7]; // wartosci z sensorow
float sensor_weights[sensorsNumber] = {-12.0, -4.0, -1.0, 0.0, 1.0, 4.0, 12.0};

// do odczytu czujnikow
bool blackCali = false; //czy skalibrowano sensory na linie
bool whiteCali = false; //czy skalibrowano sensory na powierzchnie
bool InvertLogic = true; //linia biala-true , linia czarna-false  

//zakresy Bledow
int readErrorBlack = 7; // podloga
int readErrorWhite = 7; // linia

// SOFT - FUNKCJE
//------------------------------------------------------------------------------------------------------
//SILNIKI

//Silnik lewy
void leftMotor(float speed) {

  static float lastSpeed = 0;
  speed = constrain(speed, -255.0, 255.0);
  speed = speed * speedRatio;

  if(abs(speed - lastSpeed) > safetySpeedChange && doIGiveAFuck){
    digitalWrite(L1, HIGH);
    digitalWrite(L2, HIGH);
    delay(backEMFDelay);
  }

  if(speed > 0){
    digitalWrite(L1, HIGH);
    digitalWrite(L2, LOW);
    analogWrite(ENL, int(speed));
  }
  else if(speed < 0){
    digitalWrite(L1, LOW);
    digitalWrite(L2, HIGH);
    analogWrite(ENL, int(-speed));
  }
  else{
    digitalWrite(L1, LOW);
    digitalWrite(L2, LOW);
    analogWrite(ENL, 0);
  }

  lastSpeed = speed;
}

//Silnik prawy
void rightMotor(float speed) {

  static float lastSpeed = 0;
  speed = constrain(speed, -255.0, 255.0);
  speed = speed * speedRatio;

  if(abs(speed - lastSpeed) > safetySpeedChange && doIGiveAFuck){
    digitalWrite(R1, HIGH);
    digitalWrite(R2, HIGH);
    delay(backEMFDelay);
  }
  if(speed > 0){
    digitalWrite(R1, HIGH);
    digitalWrite(R2, LOW);
    analogWrite(ENR, int(speed));
  }
  else if(speed < 0){
    digitalWrite(R1, LOW);
    digitalWrite(R2, HIGH);
    analogWrite(ENR, int(-speed));
  }
  else{
    digitalWrite(R1, LOW);
    digitalWrite(R2, LOW);
    analogWrite(ENR, 0);
  }

  lastSpeed = speed;
}

//FUNKCJE_GŁÓWNE
//-----------------------------------------------------------------------------------------------------

// funkcja jazdy robota


//Funkcje Pomocnicze
//----------------------------------------------------------------------------------------------------
i


//SETUP
//----------------------------------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);  // Uruchomienie komunikacji szeregowej
  for (int i = 0; i < 7; i++) {
    pinMode(analogPins[i], INPUT);
  }
  pinMode(speedsetter, INPUT);
  pinMode(button, INPUT);
  pinMode(L1, OUTPUT);
  pinMode(L2, OUTPUT);
  pinMode(R1, OUTPUT);
  pinMode(R2, OUTPUT);
  pinMode(ENL, OUTPUT);
  pinMode(ENR, OUTPUT);
}

void loop(){

  if(iteration % 100 == 0){ //wykonuje się z okresem = 100*(czas potrzebny na wykonanie wszystkiego w loop)
    basicInfo();
    //levelsInfo();
  }
  iteration += 1;
  delay(loopDelay);
}


//_______________________________________________________________________________________________
// DEBUG

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
}

void levelsInfo(){

  Serial.print("[");
  for (int i = 0; i < 7; i++) {
    Serial.print(analogValues[i]);
    Serial.print(i < 6 ? ", " : "] / ");
  }
  Serial.print("[");
  for (int i = 0; i < 7; i++) {
    Serial.print(blackLevels[i]);
    Serial.print(i < 6 ? ", " : "] / ");
  }
  Serial.print("[");
  for (int i = 0; i < 7; i++) {
    Serial.print(whiteLevels[i]);
    Serial.print(i < 6 ? ", " : "] / ");
  }
  Serial.print("[");
  for (int i = 0; i < 7; i++) {
    Serial.print(caliValues[i]);
    Serial.print(i < 6 ? ", " : "] \n");
  }

}
