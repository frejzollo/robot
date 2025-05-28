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


bool blackCali = false; //czy skalibrowano sensory na linie
bool whiteCali = false; //czy skalibrowano sensory na powierzchnie


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

  basicInfo();
  //levelsInfo();
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
  delay(1000);

}
