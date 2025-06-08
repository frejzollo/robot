//HARDWARE-PINY_____________________________________________________________
const int sensorsNumber = 9;
int analogPins[sensorsNumber] = {27, 26, 25, 33, 32, 35, 34, 39, 36}; //patrząc z góry od lewej do prawej
const int button = 18;

// Motor prawy
const int ENR = 19;
const int R1 =  17;
const int R2 = 16;

//Motor lewy
const int ENL = 23; 
const int L2 = 21;
const int L1 = 22;

//BACK_EMF_____________________________________________________________
bool doIGiveAFuck = true; //<------------------------------------------Jak false to wszystkie dodatkowe zabezpieczenia idą się jebać
int safetySpeedChange = 50;
int backEMFDelay = 30;


//PWM__________________________________________________________________
const int pwmFreq = 1000;
const int pwmResolution = 8;
const int pwmChannelL = 0;
const int pwmChannelR = 1;


//SOFTWARE-ZIMENNE__________________________________________________________
int loopDelay = 10;
int iteration = 0;
int mode=0; // tryb guzika
int sensorsInAirValue = 100;
float Kp = 60.0;
float Kd = 45.0;
float baseSpeedMax = 130.0;
float baseSpeedMin = 80.0;
float sensor_weights[sensorsNumber] = {-10.0, -6.0, -2.0, -1.0, 0.0, 1.0, 2.0, 6.0, 10.0};
int inRideDelay = 15;


//ZAKRESY BLEDOW
int readErrorBlack = 300; // podloga
int readErrorWhite = 300; // linia


bool blackCali = false; //czy skalibrowano sensory na linie
bool whiteCali = false; //czy skalibrowano sensory na powierzchnie

//SOFTWARE-TABLICE__________________________________________________________
int analogValues[sensorsNumber];
int blackLevels[sensorsNumber]; //stany na linii
int whiteLevels[sensorsNumber]; //stany na powierzchni
int caliValues[sensorsNumber]; //skalibrowane


//FUNKCJE-SILNIKI__________________________________________________________

//lewy silnik
void leftMotor(float speed) {

  static float lastSpeed = 0;
  speed = constrain(speed, -255.0, 255.0);

  if(abs(speed - lastSpeed) > safetySpeedChange && doIGiveAFuck){
    digitalWrite(L1, HIGH);
    digitalWrite(L2, HIGH);
    delay(backEMFDelay);
  }

if(speed > 0){
  digitalWrite(L1, HIGH);
  digitalWrite(L2, LOW);
  ledcWrite(pwmChannelL, int(speed));
}
else if(speed < 0){
  digitalWrite(L1, LOW);
  digitalWrite(L2, HIGH);
  ledcWrite(pwmChannelL, int(-speed));
}
else{
  digitalWrite(L1, LOW);
  digitalWrite(L2, LOW);
  ledcWrite(pwmChannelL, 0);
}

  lastSpeed = speed;
}

//prawy silnik
void rightMotor(float speed) {

  static float lastSpeed = 0;
  speed = constrain(speed, -255.0, 255.0);

  if(abs(speed - lastSpeed) > safetySpeedChange && doIGiveAFuck){
    digitalWrite(R1, HIGH);
    digitalWrite(R2, HIGH);
    delay(backEMFDelay);
  }
  if(speed > 0){
    digitalWrite(R1, HIGH);
    digitalWrite(R2, LOW);
    ledcWrite(pwmChannelR, int(speed));
  }
  else if(speed < 0){
    digitalWrite(R1, LOW);
    digitalWrite(R2, HIGH);
    ledcWrite(pwmChannelR, int(-speed));
  }
  else{
    digitalWrite(R1, LOW);
    digitalWrite(R2, LOW);
    ledcWrite(pwmChannelR, 0);
  }

  lastSpeed = speed;
}

//FUNKCJE-GLOWNE
void ride(){

  float line_error = 0.0;
  int count = 0;

  for(int i = 0; i < sensorsNumber; i++){
    if(caliValues[i] == 1){
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

//FUNKCJE-DODATKOWE________________________________________________________

//Zrzut: tablica[A] => tablica[B]
void drop(int* A, int* B){
  for(int i = 0; i < sensorsNumber; i++) {
    B[i] = analogRead(A[i]);
  }
}

//Suma analogowych wartości czujników 
int sumSensorsAnalog(){
  int x = 0;
  for(int i = 0; i < sensorsNumber; i++){
    x += analogValues[i];
  }
  return x;
}

//SETUP________________________________________________________________

void setup(){
  Serial.begin(9600);
  for (int i = 0; i < sensorsNumber; i++) {
    pinMode(analogPins[i], INPUT);
  }
  pinMode(button, INPUT_PULLUP);
  pinMode(L1, OUTPUT);
  pinMode(L2, OUTPUT);
  pinMode(ENL, OUTPUT);
  pinMode(R1, OUTPUT);
  pinMode(R2, OUTPUT);
  pinMode(ENR, OUTPUT);

  //PWM setup
  ledcSetup(pwmChannelL, pwmFreq, pwmResolution);
  ledcAttachPin(ENL, pwmChannelL);

  ledcSetup(pwmChannelR, pwmFreq, pwmResolution);
  ledcAttachPin(ENR, pwmChannelR);
}

//LOOP_________________________________________________________________

void loop(){
  drop(analogPins, analogValues);
  if (digitalRead(button) == LOW) {
    mode += 1;
    delay(400);
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
  if(mode == 3)
  {
   
    if(sumSensorsAnalog() < sensorsInAirValue){ //stop gdy podniesiemy robota lub gdy najedzie prostopadle na linie!!
      leftMotor(0);
      rightMotor(0);
    }
    else{
      ride();
    }
  }
    if(mode >= 4)
  {
    leftMotor(0);
    rightMotor(0);
  }
  
  
  //DEBUG
  if(iteration % 100 == 0){
  //basicInfo();
  levelsInfo();
  }
  iteration += 1;


  delay(loopDelay);
}



//DEBUG_________________________________________________________________

void basicInfo(){

  Serial.print(digitalRead(button));
  Serial.print(" / ");
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
  Serial.println();
}