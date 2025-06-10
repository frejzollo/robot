//HARDWARE-PINY________________________________________________________________________________________________________
int led = 2;
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
float speedNerf = 0.4;
float turnNerf = 10;
float turnSpeed = 255.0;
float arcMov = 0.7;
int turnBreakDistance = 2;
float Kp = 50.0;
float Kd = 30.0;
float s08 = 10.0;
float s17 = 6.0;
float s26 = 2.0;
float s35 = 1.0;
float s4 = 0.0;
float sensorsWeights[sensorsNumber] = {-s08, -s17, -s26, -s35, s4, s35, s26, s17, s08};
float baseSpeedMax = 150.0;
float baseSpeedMin = 100.0;
//POMOC-STROJENIA>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int lastKnowDirection = 0; //-1 lewo, 1 prawo
int hardTimeStart = 0;

// ustawianie mode>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int lastHandledMode = -1;

bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long lastPressHandledTime = 0;

const int DEBOUNCE_TIME = 50;
const int HOLD_OFF_TIME = 300;

// emergencyTurn state

int emergencyTurnState = 0;

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//FUNKCJE-GŁÓWNE<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
void ride() {
    //Update-sensorów______________________________________________________________________________________________
    updateCaliValues();
    digitalWrite(led, 0);
    //_____________________________________________________________________________________________________________
    float lineError = 0.0;
    int count = 0;
    for(int i = 0; i < sensorsNumber; i++){
        if(caliValues[i] == -1) {
            lineError += sensorsWeights[i];
            count++;
        }
    }
    if(count > 0){
        lineError = lineError / count;
    }
    // Regulacja PD
    static float lastError = 0;
    float derivative = lineError - lastError;
    float correction = Kp * lineError + Kd * derivative;
    lastError = lineError;
    // Dynamiczna prędkość w zależności od zakrętu
    float baseSpeed = constrain(baseSpeedMax - abs(lineError) * turnNerf, baseSpeedMin, baseSpeedMax);
    float leftSpeed = baseSpeed + correction;
    float rightSpeed = baseSpeed - correction;


    /*
    emergencyTurnState:
    0 - jedziemy normalnie
    1 - skrajny lewy sensor wykrył linie (wcześniej wykrywał tło)
    2 - skrajny prawy sensor wykrył linie (wcześniej wykrywał tło)

    
    */
    if (caliValues[0] == -1 && emergencyTurnState == 0)
        emergencyTurnState = 1;
    if (caliValues[0] == 1 && emergencyTurnState == 1){ // wykrywa tło (wcześniej wykrył linie więc trzeba się obrócić w lewo)
        rotate(turnSpeed);
        emergencyTurnState = 0;
        return;
    }
        
    if (caliValues[sensorsNumber] == -1 && emergencyTurnState == 0)
        emergencyTurnState = 2;
    if (caliValues[sensorsNumber] == 1 && emergencyTurnState == 2){ // wykrywa tło (wcześniej wykrył linie więc trzeba się obrócić w prawo)
        rotate(-turnSpeed);
        emergencyTurnState = 0;
        return;
    }


    leftMotor(leftSpeed);
    rightMotor(rightSpeed);        
}

void rotate(int direction){
    while(true){
        // kręcimy się do czasu aż środkowy sensor wykryje linie
        updateCaliValues();
        if (caliValues[sensorsNumber/2] == -1)
            break;

        leftMotor(direction);
        rightMotor(-direction); 
    }
}

void emergencyTurn(){
    digitalWrite(led, 1);
    if(lastKnowDirection == 1){
        leftMotor(turnSpeed);
        rightMotor(-turnSpeed * arcMov);
    }
    else{
        leftMotor(-turnSpeed * arcMov);
        rightMotor(turnSpeed);
    }
    while(caliValues[4 - lastKnowDirection * turnBreakDistance] == 1){
        updateCaliValues();
        continue;
    }
    return;
}


void checkButton() {
    bool currentButtonState = digitalRead(button);
    unsigned long currentTime = millis();
  
    if (currentButtonState != lastButtonState) {
      lastDebounceTime = currentTime;
    }
  
    if ((currentTime - lastDebounceTime) > DEBOUNCE_TIME) {
        if (currentButtonState == LOW && (currentTime - lastPressHandledTime) > HOLD_OFF_TIME) {
            mode++;
            if (mode > 5) {
                mode = 0;
            }
    
            Serial.print("Wciśnięto przycisk");
    
            lastPressHandledTime = currentTime;
        }
    }
  
    lastButtonState = currentButtonState;
}

void handleModeChange() {
    if (mode != lastHandledMode) {
      switch (mode) {
        case 1:
            drop(analogPins, blackLevels);
            Serial.println("Zmiana na stan 1. (set blackLevels)");
            blackCali = true;
            break;
        case 2:
            Serial.println("Zmiana na stan 2. (set whiteLevels)");
            drop(analogPins, whiteLevels);
            whiteCali = true;
            break;
        case 3:
            Serial.println("Zmiana na stan 3. (ride)");
        case 4:
            Serial.println("Zmiana na stan 4. (stop )");
            leftMotor(0);
            rightMotor(0);
            break;
        default:
            Serial.println("Nieznany tryb");
          break;
        }
        lastHandledMode = mode;  // zapamiętaj, że już obsłużyliśmy ten tryb
    }

    if(mode == 3)
      ride();
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
  pinMode(led,OUTPUT);
  //PWM setup
  ledcSetup(pwmChannelL, pwmFreq, pwmResolution);
  ledcAttachPin(ENL, pwmChannelL);
  ledcSetup(pwmChannelR, pwmFreq, pwmResolution);
  ledcAttachPin(ENR, pwmChannelR);
}
//LOOP_________________________________________________________________________________________________________________
void loop(){
    checkButton();
    handleModeChange();

    //DEBUG
    if(iteration % 100 == 0){
        iteration = 0;
        updateCaliValues();
        //basicInfo();
        levelsInfo();
    }  
    iteration += 1;
}
//DEBUG________________________________________________________________________________________________________________
void basicInfo() {

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
//Suma-skalibrowanych-wartości-czujników 
int sumCaliValues(){
  int x = 0;
  for(int i = 0; i < sensorsNumber; i++){
    x += caliValues[i];
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

void updateCaliValues() {
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
}