const int speedsetter = A0;
const int button = 13;
int mode=0;

int analogPins[7] = {A1, A2, A3, A4, A5, A6, A7};
int blackLevels[7]; //stany na linii
int whiteLevels[7]; //stany na powierzchni
int caliValues[7]; //skalibrowane
int analogValues[7]; // wartosci z sensorow

int readErrorBlack = 7; // podloga
int readErrorWhite = 7; // linia

//-----------------------------------------------------------------------------------
// Wartości pomocnicze
int iteration = 0; //ile razy wykonano pętle loop()
float speedRatio = 0;
bool blackCali = false; //czy skalibrowano sensory na linie
bool whiteCali = false; //czy skalibrowano sensory na powierzchnie


// Motor lewy
const int ENL = 5;
const int L2 = 3;
const int L1 = 4; //z tym pinem serial nie działa, można zmienić jak zależy na debugu
// Motor prawy
const int ENR = 6; // ^ -||-
const int R2 = 7;
const int R1 = 8;


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

void loop() {
    for (int i = 0; i < 7; i++) {
    analogValues[i] = analogRead(analogPins[i]);
  }

  if(digitalRead(button)){
    mode += 1;
    delay(400); //debounce guzika
  }

  //mod 1: jednorazowe zczytanie aktualnych odczytów czujników i przypisanie ich jako wartości odpowiadających linii
  if(mode == 1 && !blackCali){
    for (int i = 0; i < 7; i++) {
      blackLevels[i] = analogRead(analogPins[i]);
    }
    blackCali = true;
  }
  
  //mod 2: jednorazowe zczytanie aktualnych odczytów czujników i przypisanie ich jako wartości odpowiadających powierzchni
  if(mode == 2 && !whiteCali){
    for (int i = 0; i < 7; i++) {
      whiteLevels[i] = analogRead(analogPins[i]);
    }
    whiteCali = true;
  }

  //zczytywanie wartości czujników po kalibracji: powierzchnia = 1, linia = -1, niepewny odczyt = 0
  if(whiteCali && blackCali){
    for (int i = 0; i < 7; i++) {
      if(abs(blackLevels[i] - analogValues[i]) < readErrorBlack){
        caliValues[i] = -1;
      } else if(abs(whiteLevels[i] - analogValues[i]) < readErrorWhite || analogValues[i] > whiteLevels[i]){ // or dlatego, że kalibracja mogła być w cieniu czy coś tam...}
        caliValues[i] = 1;
      } else{
        caliValues[i] = 0;
      }
    }
  }

  //mod 3: jazda
  if(mode == 3){
    speedRatio = constrain(1 - float(analogRead(speedsetter))/690, 0, 1);
    if(sumSensorsAnalog() < 10){ //stop gdy podniesiemy robota lub gdy najedzie prostopadle na linie!!
      // leftMotor(0);
      // rightMotor(0);
    }else{
      //TUTAJ JAZDA
    }
  }

  if(iteration % 100 == 0){ //wykonuje się z okresem = 100*(czas potrzebny na wykonanie wszystkiego w loop)
    basicInfo();
    //levelsInfo();
  }

  iteration += 1;
  delay(10);
}

//suma analogowych wartości czujników 
int sumSensorsAnalog(){
  int x = 0;
  for(int i = 0; i < 7; i++){
    x += analogValues[i];
  }
  return x;
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
