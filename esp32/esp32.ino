//przypisanie pinów ESP do hardwaru
const int speedsetter = 39; //pokrętło
const int button = 36; //guzik
const int analogPins[9] = {13, 14, 27, 26, 25, 33, 32, 35, 34}; //sensory patrząc z góry od lewej do prawej
// Motor lewy
const int ENL = 23;
const int L2 = 22;

// Motor prawy
const int ENR = 21; // ^ -||-
const int R2 = 19;

//wartości sensorów
int analogValues[9]; //analogowe
int blackLevels[9]; //stany na linii
int whiteLevels[9]; //stany na powierzchni
int caliValues[9]; //skalibrowane

//wartości pomocnicze
int mode = 0; //tryb pracy
float speedRatio = 0; //mnożnik prędkości zależny od pokrętła
int iteration = 0; //ile razy wykonano pętle loop()
bool blackCali = false; //czy skalibrowano sensory na linie
bool whiteCali = false; //czy skalibrowano sensory na powierzchnie

//dopuszczalne błędy odczytu sensorów
int readErrorBlack = 10; //linia
int readErrorWhite = 50; //powierzchnia

//PWM
const int pwmFreq = 1000;
const int pwmResolution = 8;
const int pwmChannelL = 0;
const int pwmChannelR = 1;


void setup(){ //wszystko co przed startem robota

  Serial.begin(115200); //uruchomienie serialu w celu dostarczaniu informacji przez USB
  delay(1000);
//przypisanie ról I/O do pinów
  for (int i = 0; i < 9; i++) {
    pinMode(analogPins[i], INPUT);
  }
  pinMode(speedsetter, INPUT);
  pinMode(button, INPUT);
  pinMode(L2, OUTPUT);
  pinMode(R2, OUTPUT);

//PWM setup
  ledcSetup(pwmChannelL, pwmFreq, pwmResolution);
  ledcAttachPin(ENL, pwmChannelL);

  ledcSetup(pwmChannelR, pwmFreq, pwmResolution);
  ledcAttachPin(ENR, pwmChannelR);

}

void loop() {

//Zczytywanie wartości czujników
  for (int i = 0; i < 9; i++) {
    analogValues[i] = analogRead(analogPins[i]);
  }


//Naciśnięcie guzika zmienia tryb pracy robota (mode)
  if(digitalRead(button)){
    mode += 1;
    delay(400); //debounce guzika
  }

//mod 1: jednorazowe zczytanie aktualnych odczytów czujników i przypisanie ich jako wartości odpowiadających linii
  if(mode == 1 && !blackCali){
    for (int i = 0; i < 9; i++) {
      blackLevels[i] = analogRead(analogPins[i]);
    }
    blackCali = true;
  }

//mod 2: jednorazowe zczytanie aktualnych odczytów czujników i przypisanie ich jako wartości odpowiadających powierzchni
  if(mode == 2 && !whiteCali){
    for (int i = 0; i < 9; i++) {
      whiteLevels[i] = analogRead(analogPins[i]);
    }
    whiteCali = true;
  }

//zczytywanie wartości czujników po kalibracji: powierzchnia = 1, linia = -1, niepewny odczyt = 0
  if(whiteCali && blackCali){
    for (int i = 0; i < 9; i++) {
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
  if (mode == 3) {
    if (sumSensorsAnalog() < 10) { // stop gdy robot nie widzi linii
      leftMotor(0);
      rightMotor(0);
    } else {
      speedRatio = constrain(1 - float(analogRead(speedsetter)) / 690.0, 0, 1);

      // Nowe wagi sensorów (bez skrajnych sensorów)
      float sensor_weights[9] = {0.0, -4.0, -3.0, -1.0, 0.0, 1.0, 3.0, 4.0, 0.0};

      float line_error = 0.0;
      int count = 0;

      for (int i = 0; i < 9; i++) {
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



//ustawienie prędkości silników. Prędkość € <-255;255> mnożona jest przez speedRatio € <0;1>, zależne od pokrętła. Gdy prędkość < 0 tzn. jedziemy do tyłu
void leftMotor(float speed) {
  static int lastDirectionL = 0; // 1 = przód, -1 = tył, 0 = stop

  speed = constrain(speed, -255.0, 255.0);
  speed = speed * speedRatio;

  int newDirection = (speed > 0) ? 1 : (speed < 0) ? -1 : 0;

  // Jeśli zmiana kierunku — zatrzymaj silnik najpierw
  if (newDirection != lastDirectionL && lastDirectionL != 0 && newDirection != 0) {
    digitalWrite(L2, LOW);
    ledcWrite(pwmChannelL, 0);
    delay(50); // opóźnienie chroniące przed Back EMF
  }

  if (speed > 0) {
    digitalWrite(L2, LOW);
    ledcWrite(pwmChannelL, int(speed));
  } else if (speed < 0) {
    digitalWrite(L2, HIGH);
    ledcWrite(pwmChannelL, int(-speed));
  } else {
    digitalWrite(L2, LOW);
    ledcWrite(pwmChannelL, 0);
  }

  lastDirectionL = newDirection;
}

void rightMotor(float speed) {
  static int lastDirectionR = 0;

  speed = constrain(speed, -255.0, 255.0);
  speed = speed * speedRatio;

  int newDirection = (speed > 0) ? 1 : (speed < 0) ? -1 : 0;

  if (newDirection != lastDirectionR && lastDirectionR != 0 && newDirection != 0) {
    digitalWrite(R2, LOW);
    ledcWrite(pwmChannelR, 0);
    delay(50); // 🔒 opóźnienie chroniące przed Back EMF
  }

  if (speed > 0) {
    digitalWrite(R2, LOW);
    ledcWrite(pwmChannelR, int(speed));
  } else if (speed < 0) {
    digitalWrite(R2, HIGH);
    ledcWrite(pwmChannelR, int(-speed));
  } else {
    digitalWrite(R2, LOW);
    ledcWrite(pwmChannelR, 0);
  }

  lastDirectionR = newDirection;
}


//suma analogowych wartości czujników 
int sumSensorsAnalog(){
  int x = 0;
  for(int i = 0; i < 9; i++){
    x += analogValues[i];
  }
  return x;
}

//suma zkalibrowanych wartości czujników 
int sumSensors(){
  int x = 0;
  for(int i = 0; i < 9; i++){
    x += caliValues[i];
  }
  return x;
}

//informacja na serial: [wartości sensorów] / [wartości linii] / [wartości powierzchni] / [wartości sensorów po kalibracji]
void levelsInfo(){

  Serial.print("[");
  for (int i = 0; i < 9; i++) {
    Serial.print(analogValues[i]);
    Serial.print(i < 8 ? ", " : "] / ");
  }
  Serial.print("[");
  for (int i = 0; i < 9; i++) {
    Serial.print(blackLevels[i]);
    Serial.print(i < 8 ? ", " : "] / ");
  }
  Serial.print("[");
  for (int i = 0; i < 9; i++) {
    Serial.print(whiteLevels[i]);
    Serial.print(i < 8 ? ", " : "] / ");
  }
  Serial.print("[");
  for (int i = 0; i < 9; i++) {
    Serial.print(caliValues[i]);
    Serial.print(i < 8 ? ", " : "] \n");
  }

}





//informacja na serial: stan guzika / wartość pokrętła / [wartości sensorów] / tryb pracy (mode)
void basicInfo(){

  Serial.print(digitalRead(button));
  Serial.print(" / ");
  Serial.print(analogRead(speedsetter));
  Serial.print(" / [");
  for (int i = 0; i < 9; i++) {
    Serial.print(analogValues[i]);
    Serial.print(i < 8 ? ", " : "] / ");
  }
  Serial.print(mode);
  Serial.println();
}