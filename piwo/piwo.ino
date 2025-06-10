//HARDWARE-PINY________________________________________________________________________________________________________
const int sensorsNumber = 9;
int analogPins[sensorsNumber] = {27, 26, 25, 33, 32, 35, 34, 39, 36}; //patrząc z góry od lewej do prawej
const int button = 18;
//Motor-prawy
const int ENR = 19;
const int R2 =  17;
const int R1 = 16;
//Motor-lewy
const int ENL = 23; 
const int L1 = 21;
const int L2 = 22;
//BACK-EMF_____________________________________________________________________________________________________________
bool doIGiveAFuck = true; //jak false to wszystkie dodatkowe zabezpieczenia idą się jebać
int safetySpeedChange = 50;
int backEMFDelay = 30;
//PWM__________________________________________________________________________________________________________________
const int pwmFreq = 1000;
const int pwmResolution = 8;
const int pwmChannelL = 0;
const int pwmChannelR = 1;
//SOFTWARE-ZMIENNE_____________________________________________________________________________________________________
int loopDelay = 10;
//SOFTWARE-TABLICE_____________________________________________________________________________________________________
int analogValues[sensorsNumber];
int blackLevels[sensorsNumber]; //stany na linii
int whiteLevels[sensorsNumber]; //stany na powierzchni
int caliValues[sensorsNumber]; //skalibrowane
//ZAKRESY-BŁĘDÓW_______________________________________________________________________________________________________
int readErrorBlack = 300;
int readErrorWhite = 300;
//ZMIENNE-POMOCNICZE___________________________________________________________________________________________________
bool blackCali = false; //czy skalibrowano sensory na linie
bool whiteCali = false; //czy skalibrowano sensory na powierzchnie
int iteration = 0;
int mode = 0; //tryb guzika
//STROJENIE>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
float speedNerf = 0.3;
float turnSpeed = 255.0;
float arcMov = 0.8;
float Kp = 65.0;
float Kd = 45.0;
float s08 = 10.0;
float s17 = 4.0;
float s26 = 3.0;
float s35 = 2.0;
float s4 = 0.0;
float sensorsWeights[sensorsNumber] = {-s08, -s17, -s26, -s35, s4, s35, s26, s17, s08};
float baseSpeedMax = 240.0;
float baseSpeedMin = 120.0;
//POMOC-STROJENIA>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int lastKnowDirection = 0; //-1 lewo, 1 prawo
int hardTurn = 0; //-1 lewo, 1 prawo
int hardTimeStart = 0;
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//FUNKCJE-GŁÓWNE<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
void ride(){
    while(digitalRead(button)){
        //Update-sensorów______________________________________________________________________________________________
        drop(analogPins, analogValues);
        for(int i = 0; i < sensorsNumber; i++){
                if(abs(blackLevels[i] - analogValues[i]) < readErrorBlack){
                    caliValues[i] = -1;
                }
                else if(abs(whiteLevels[i] - analogValues[i]) < readErrorWhite || analogValues[i] > whiteLevels[i]){
                    caliValues[i] = 1;
                }
                else{
                    caliValues[i] = 0;
                }
            }
        //_____________________________________________________________________________________________________________
        float lineError = 0.0;
        int count = 0;
        for(int i = 0; i < sensorsNumber; i++){
            if(caliValues[i] == -1){
            lineError += sensorsWeights[i];
            count++;
            }
        }
        if(count > 0){
            lineError = lineError / count;
        }
        else{
            emergencyTurn();
            return;
        }
        // Regulacja PD
        static float lastError = 0;
        float derivative = lineError - lastError;
        float correction = Kp * lineError + Kd * derivative;
        lastError = lineError;

        // Dynamiczna prędkość w zależności od zakrętu
        float baseSpeed = constrain(baseSpeedMax - abs(lineError) * 10.0, baseSpeedMin, baseSpeedMax);

        float leftSpeed = baseSpeed + correction;
        float rightSpeed = baseSpeed - correction;

        if(leftSpeed > rightSpeed){
            lastKnowDirection = -1;
        }
        else{
            lastKnowDirection = 1;
        }
        if(count >= 3 && caliValues[0] == -1){
            hardTurn = -1;
            hardTimeStart = millis();
        }
        else if(count >= 3 && caliValues[8] == -1){
            hardTurn = 1;
            hardTimeStart = millis();
        }

        if(millis() - hardTimeStart > 800){
            hardTurn = 0;
        }

        if(caliValues[4] == -1){
            resetWeights();
        }
        leftMotor(leftSpeed);
        rightMotor(rightSpeed);        
    }
}

void emergencyTurn(){
    if(hardTurn != 0){
        if(hardTurn == 1){
            int idx[] = {8,7,6};
            disableWeights(idx, sizeof(idx) / sizeof(idx[0]));
            leftMotor(turnSpeed);
            rightMotor(-turnSpeed * arcMov);
        }
        else{
            int idx[] = {0,1,2};
            disableWeights(idx, sizeof(idx) / sizeof(idx[0]));
            leftMotor(-turnSpeed * arcMov);
            rightMotor(turnSpeed);
        }
    }
    else{
        if(lastKnowDirection == 1){
            leftMotor(turnSpeed);
            rightMotor(-turnSpeed * arcMov);
        }
        else if(lastKnowDirection == -1){
            leftMotor(-turnSpeed * arcMov);
            rightMotor(turnSpeed);
        }
    }
}
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
//SETUP________________________________________________________________________________________________________________
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
//LOOP_________________________________________________________________________________________________________________
void loop(){
    if (digitalRead(button) == LOW) {
        mode += 1;
        delay(400);
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
    if(mode == 3){
        ride();
    }

    if(mode >= 4){
        leftMotor(0);
        rightMotor(0);
    }

    //DEBUG
    if(iteration % 100 == 0){
        drop(analogPins, analogValues);
        if(whiteCali && blackCali){
            for(int i = 0; i < sensorsNumber; i++){
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
        //basicInfo();
        levelsInfo();
    }
  
  
    iteration += 1;
    delay(loopDelay);
}
//DEBUG________________________________________________________________________________________________________________
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
    Serial.print(caliValues[i] == 1 ? "■" : caliValues[i] == -1 ? "□" : "?");
    Serial.print(i < sensorsNumber - 1 ? ", " : "] /");
  }
  Serial.print("[");
  for (int i = 0; i < sensorsNumber; i++) {
    Serial.print(blackLevels[i]);
    Serial.print(i < sensorsNumber - 1 ? ", " : "] / ");
  }
  Serial.print("[");
  for (int i = 0; i < sensorsNumber; i++) {
    Serial.print(whiteLevels[i]);
    Serial.print(i < sensorsNumber - 1 ? ", " : "]");
  }
  Serial.println();
}
//FUNKCJE-DODATKOWE____________________________________________________________________________________________________
//Lewy-silnik
void leftMotor(float speed) {

  static float lastSpeed = 0;
  speed = constrain(speed, -255.0, 255.0) * speedNerf;

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

//Prawy-silnik
void rightMotor(float speed) {

  static float lastSpeed = 0;
  speed = constrain(speed, -255.0, 255.0) * speedNerf;

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
//Zrzut:tablica[A]=>tablica[B]
void drop(int* A, int* B){
  for(int i = 0; i < sensorsNumber; i++) {
    B[i] = analogRead(A[i]);
  }
}
//Suma-analogowych-wartości-czujników 
int sumSensorsAnalog(){
  int x = 0;
  for(int i = 0; i < sensorsNumber; i++){
    x += analogValues[i];
  }
  return x;
}
//Reset-wag
void resetWeights(){
    sensorsWeights[0] = -s08;
    sensorsWeights[1] = -s17;
    sensorsWeights[2] = -s26;
    sensorsWeights[3] = -s35;
    sensorsWeights[4] = s4;
    sensorsWeights[5] = s35;
    sensorsWeights[6] = s26;
    sensorsWeights[7] = s17;
    sensorsWeights[6] = s08;
}
//Wyłączanie-wag
void disableWeights(int idxs[], int size){
    for (int i = 0; i < size; i++){
        int idx = idxs[i];
        sensorsWeights[idx] = 0;
    }
}