//HARDWARE-PINY_____________________________________________________________
const int sensorsNumber = 9;
int analogPins[sensorsNumber] = {27, 26, 25, 33, 32, 35, 34, 39, 36}; //patrząc z góry od lewej do prawej
const int button = 18;

//SOFTWARE-ZIMENNE__________________________________________________________
int loopDelay = 10;
int iteration = 0;
int mode=0; // tryb guzika

//SOFTWARE-TABLICE__________________________________________________________
int analogValues[sensorsNumber];

//FUNKCJE-DODATKOWE________________________________________________________

//Zrzut: tablica[A] => tablica[B]
void drop(int* A, int* B){
  for(int i = 0; i < sensorsNumber; i++) {
    B[i] = analogRead(A[i]);
  }
}



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
  drop(analogPins, analogValues);
  if (digitalRead(button) == LOW) {
    mode += 1;
    delay(400);
  }

  //DEBUG
  if(iteration % 100 == 0){
  basicInfo();
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
