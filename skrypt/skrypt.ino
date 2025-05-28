const int speedsetter = A0;
const int button = 13;
int mode=0;

int analogPins[7] = {A1, A2, A3, A4, A5, A6, A7};
int blackLevels[7]; //stany na linii
int whiteLevels[7]; //stany na powierzchni
int caliValues[7]; //skalibrowane
int analogValues[7]; // wartosci z sensorow
float sensor_weights[7] = {-4.0, -3.0, -1.0, 0.0, 1.0, 3.0, 4.0};

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
const int L2 = 7;
const int L1 = 6; //z tym pinem serial nie działa, można zmienić jak zależy na debugu
// Motor prawy
const int ENR = 10; // ^ -||-
const int R2 = 9;
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
    speedRatio = constrain(1 - float(analogRead(speedsetter))/150, 0, 1);
    if(sumSensorsAnalog() < 70){ //stop gdy podniesiemy robota lub gdy najedzie prostopadle na linie!!
      leftMotor(0);
      rightMotor(0);
    } else {
      // Nowe wagi sensorów (bez skrajnych sensorów)
      float sensor_weights[7] = {-4.0, -3.0, -1.0, 0.0, 1.0, 3.0, 4.0};

      float line_error = 0.0;
      int count = 0;

      for (int i = 0; i < 7; i++) {
        if (caliValues[i] == -1) {
          line_error += sensor_weights[i];
          count++;
        }
      }

      if (count > 0) {
        line_error = line_error / count;
      } else {
        line_error = 0;
      }

      // Regulacja PD
      static float last_error = 0;
      float Kp = 35.0;
      float Kd = 25.0;
      float derivative = line_error - last_error;
      float correction = Kp * line_error + Kd * derivative;
      last_error = line_error;

      // Dynamiczna prędkość w zależności od zakrętu
      float base_speed = constrain(130.0 - abs(line_error) * 10.0, 80.0, 130.0);

      float left_speed = base_speed + correction;
      float right_speed = base_speed - correction;

      leftMotor(left_speed);
      rightMotor(right_speed);
}
}

  if(iteration % 100 == 0){ //wykonuje się z okresem = 100*(czas potrzebny na wykonanie wszystkiego w loop)
    //basicInfo();
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


//------------------------------------------------------------------------------------------------------
//SILNIKI

//Silnik lewy
void leftMotor(float speed) {
  speed = constrain(speed, -255.0, 255.0);
  speed = speed * speedRatio;
  if (speed > 0) {
    digitalWrite(L1, HIGH);
    digitalWrite(L2, LOW);
    analogWrite(ENL, int(speed));
  } else if (speed < 0) {
    digitalWrite(L1, LOW);
    digitalWrite(L2, HIGH);
    analogWrite(ENL, int(-speed));
  } else {
    digitalWrite(L1, LOW);
    digitalWrite(L2, LOW);
    analogWrite(ENL, 0);
  }
  delay(50);
}

//Silnik prawy
void rightMotor(float speed) {
  speed = constrain(speed, -255.0, 255.0);
  speed = speed * speedRatio;
  if (speed > 0) {
    digitalWrite(R1, HIGH);
    digitalWrite(R2, LOW);
    analogWrite(ENR, int(speed));
  } else if (speed < 0) {
    digitalWrite(R1, LOW);
    digitalWrite(R2, HIGH);
    analogWrite(ENR, int(-speed));
  } else {
    digitalWrite(R1, LOW);
    digitalWrite(R2, LOW);
    analogWrite(ENR, 0);
  }
  delay(50);
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
