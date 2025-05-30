//HARDWARE_____________________________________________________________

int sensorsNumber = 7;
int sensorsInAirValue = 70;
int speedsetterMax = 150;
bool isSpeedSetter = true;

//BACK_EMF_____________________________________________________________
bool doIGiveAFuck = true; //<------------------------------------------Jak false to wszystkie dodatkowe zabezpieczenia idą się jebać
int safetySpeedChange = 50;
int backEMFDelay = 30;

//SOFTWARE_____________________________________________________________

float Kp = 60.0;
float Kd = 40.0;
float baseSpeedMax = 130.0;
float baseSpeedMin = 80.0;
float sensor_weights[sensorsNumber] = {-12.0, -9.0, -4.0, 0.0, 4.0, 9.0, 12.0};
int loopDelay = 10;
int inRideDelay = 15;

//PINY_________________________________________________________________

const int speedsetter = A0; //do wyjebania
const int button = 13;
int analogPins[sensorsNumber] = {A1, A2, A3, A4, A5, A6, A7};

//TABLICE WARTOŚCI SENSORÓW____________________________________________

int analogValues[sensorsNumber]; // wartosci z sensorow
int blackLevels[sensorsNumber]; //stany na linii
int whiteLevels[sensorsNumber]; //stany na powierzchni
int caliValues[sensorsNumber]; //skalibrowane

//BŁĘDY________________________________________________________________

int readErrorBlack = 7; // podloga
int readErrorWhite = 7; // linia

//TRYB_________________________________________________________________

int mode=0;

//WARTOŚCI_POMOCNICZE__________________________________________________

int iteration = 0; //ile razy wykonano pętle loop()
float speedRatio = 0;
bool blackCali = false; //czy skalibrowano sensory na linie
bool whiteCali = false; //czy skalibrowano sensory na powierzchnie

//FUNKCJE_DODATKOWE____________________________________________________

//Suma analogowych wartości czujników 
int sumSensorsAnalog(){
  int x = 0;
  for(int i = 0; i < sensorsNumber; i++){
    x += analogValues[i];
  }
  return x;
}

//Zrzut: tablica[A] => tablica[B]
void drop(int* A, int* B){
  for(int i = 0; i < sensorsNumber; i++) {
    B[i] = analogRead(A[i]);
  }
}
 

//FUNKCJE_GŁÓWNE_______________________________________________________

//JAZDA________________________________________________________________

void ride(){

  float line_error = 0.0;
  int count = 0;

  for(int i = 0; i < sensorsNumber; i++){
    if(caliValues[i] == -1){
      line_error += sensor_weights[i];
      count++;
    }
  }

  if(count > 0){
    line_error = line_error / count;
  }
  else{
    line_error = 0;
  }

  // Regulacja PD
  static float last_error = 0;
  float derivative = line_error - last_error;
  float correction = Kp * line_error + Kd * derivative;
  last_error = line_error;

  // Dynamiczna prędkość w zależności od zakrętu
  float base_speed = constrain(baseSpeedMax - abs(line_error) * 10.0, baseSpeedMin, baseSpeedMax);

  float left_speed = base_speed + correction;
  float right_speed = base_speed - correction;

  leftMotor(left_speed);
  rightMotor(right_speed);

  delay(inRideDelay);
}

//lewy silnik
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


//prawy silnik
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

}

//SETUP________________________________________________________________

void setup(){

  for (int i = 0; i < sensorsNumber; i++) {
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

//LOOP_________________________________________________________________

void loop(){

  drop(analogPins, analogValues);

  if(digitalRead(button)){
    mode += 1;
    delay(400); //debounce guzika
  }

  //zczytywanie wartości czujników po kalibracji: powierzchnia = 1, linia = -1, niepewny odczyt = 0
  if(whiteCali && blackCali){
    for (int i = 0; i < sensorsNumber; i++) {
      if(abs(blackLevels[i] - analogValues[i]) < readErrorBlack){
        caliValues[i] = -1;
      }
      else if(abs(whiteLevels[i] - analogValues[i]) < readErrorWhite || analogValues[i] > whiteLevels[i]){ //or dlatego, że kalibracja mogła być w cieniu czy coś tam...
        caliValues[i] = 1;
      }
      else{
        caliValues[i] = 0;
      }
    }
  }

//MODY__________________________________________________________________

  //mod 1: jednorazowe zczytanie aktualnych odczytów czujników i przypisanie ich jako wartości odpowiadających linii
  if(mode == 1 && !blackCali){
    drop(analogPins, blackLevels);
    blackCali = true;
  }
  
  //mod 2: jednorazowe zczytanie aktualnych odczytów czujników i przypisanie ich jako wartości odpowiadających powierzchni
  if(mode == 2 && !whiteCali){
    drop(analogPins, whiteLevels);
    whiteCali = true;
  }

  //mod 3: jazda
  if(mode == 3){

    if(isSpeedSetter){
      speedRatio = constrain(1 - float(analogRead(speedsetter))/speedsetterMax, 0, 1);
    }
    else{
      speedRatio = 1;
    }

    if(sumSensorsAnalog() < sensorsInAirValue){ //stop gdy podniesiemy robota lub gdy najedzie prostopadle na linie!!
      leftMotor(0);
      rightMotor(0);
    }
    else{
      ride();
    }
  }

  //mod 4: zmiana ustawień softu
  if(mode == 4){

  }

  //mod 5: powrót do mod 3
  if(mode == 5){
    mode = 3;
  }



  if(iteration % 100 == 0){ //wykonuje się z okresem = 100*(czas potrzebny na wykonanie wszystkiego w loop)
    //basicInfo();
    //levelsInfo();
  }
  iteration += 1;
  delay(loopDelay);
}

//DEBUG_________________________________________________________________

void basicInfo(){

  Serial.print(digitalRead(button));
  Serial.print(" / ");
  Serial.print(analogRead(speedsetter));
  Serial.print(" / [");
  for (int i = 0; i < sensorsNumber; i++) {
    Serial.print(analogValues[i]);
    Serial.print(i < sensorsNumber - 1 ? ", " : "] / ");
  }
  Serial.print(mode);
  Serial.println();
}

void levelsInfo(){

  Serial.print("[");
  for (int i = 0; i < sensorsNumber; i++) {
    Serial.print(analogValues[i]);
    Serial.print(i < sensorsNumber - 1 ? ", " : "] / ");
  }
  Serial.print("[");
  for (int i = 0; i < sensorsNumber; i++) {
    Serial.print(blackLevels[i]);
    Serial.print(i < sensorsNumber - 1 ? ", " : "] / ");
  }
  Serial.print("[");
  for (int i = 0; i < sensorsNumber; i++) {
    Serial.print(whiteLevels[i]);
    Serial.print(i < sensorsNumber - 1 ? ", " : "] / ");
  }
  Serial.print("[");
  for (int i = 0; i < sensorsNumber; i++) {
    Serial.print(caliValues[i]);
    Serial.print(i < sensorsNumber - 1 ? ", " : "] \n");
  }

}