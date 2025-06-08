//HARDWARE-PINY_____________________________________________________________
const int sensorsNumber = 9;
int analogPins[sensorsNumber] = {27, 26, 25, 33, 32, 35, 34, 39, 36}; //patrząc z góry od lewej do prawej
const int button = 18;

//SOFTWARE-ZIMENNE__________________________________________________________
int loopDelay = 200;
int mode=0; // tryb guzika

//SOFTWARE-TABLICE__________________________________________________________
int analogValues[sensorsNumber];

//FuUNKCJE-DODATKOWE________________________________________________________
void readSensors() {
  for (int i = 0; i < sensorsNumber; i++) {
    analogValues[i] = analogRead(analogPins[i]);
  }
};



//SETUP________________________________________________________________

void setup(){
  Serial.begin(9600);
  for (int i = 0; i < sensorsNumber; i++) {
    pinMode(analogPins[i], INPUT);
  }
  pinMode(button, INPUT_PULLUP);
}

//LOOP_________________________________________________________________

void loop(){
  if (digitalRead(button) == LOW) {
    mode += 1;
    delay(400);
  }
  readSensors();
  basicInfo();
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
