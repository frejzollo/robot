#include <WiFiNINA.h>

char ssid[] = "Nano33IOT_Config";
char pass[] = "arduino123";
int status = WL_IDLE_STATUS;

WiFiServer server(80);

float Kp = 60.0;
float Kd = 40.0;

const int speedsetter = A0; // Pokrętło analogowe
const int button = 13;      // Guzik – D13, ma LED (działa)

// Sensory analogowe
int analogPins[7] = {A1, A2, A3, A4, A5, A6, A7}; // OK

// Silnik lewy
const int ENL = 5;   // PWM – OK
const int L2  = 6;   // cyfrowy, zamienione z 7
const int L1  = 3;   // cyfrowy, zamienione z 4

// Silnik prawy
const int ENR = 10;  // PWM – OK
const int R2  = 9;   // OK – cyfrowy
const int R1  = 11;  // cyfrowy, zamienione z 8

int blackLevels[7]; //stany na linii
int whiteLevels[7]; //stany na powierzchni
int caliValues[7];  //skalibrowane
int analogValues[7]; // wartosci z sensorow

int readErrorBlack = 7; // podloga
int readErrorWhite = 7; // linia

int iteration = 0; //ile razy wykonano pętle loop()
float speedRatio = 0;
bool blackCali = false; //czy skalibrowano sensory na linie
bool whiteCali = false; //czy skalibrowano sensory na powierzchnię

int mode = 0;

String htmlPage() {
  String html = 
    "<!DOCTYPE html><html><head><meta charset='utf-8'><title>PD Config</title></head><body>"
    "<h2>Regulator PD</h2>"
    "<form action='/' method='GET'>"
    "Kp: <input type='number' step='0.1' name='Kp' value='" + String(Kp) + "'><br><br>"
    "Kd: <input type='number' step='0.1' name='Kd' value='" + String(Kd) + "'><br><br>"
    "<input type='submit' value='Zapisz'>"
    "</form>"
    "<p>Aktualne Kp: " + String(Kp) + "</p>"
    "<p>Aktualne Kd: " + String(Kd) + "</p>"
    "</body></html>";
  return html;
}

void updatePDValuesFromRequest(String request) {
  int kpIndex = request.indexOf("Kp=");
  if (kpIndex != -1) {
    int amp = request.indexOf('&', kpIndex);
    if (amp == -1) amp = request.indexOf(' ', kpIndex);
    String kpStr = request.substring(kpIndex + 3, amp);
    Kp = kpStr.toFloat();
  }

  int kdIndex = request.indexOf("Kd=");
  if (kdIndex != -1) {
    int space = request.indexOf(' ', kdIndex);
    String kdStr = request.substring(kdIndex + 3, space);
    Kd = kdStr.toFloat();
  }
}

void setup() {
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
  
  WiFi.setPins(8, 7, 4, 2);
  status = WiFi.beginAP(ssid, pass);
  while (status != WL_AP_LISTENING) {
    delay(500);
    status = WiFi.beginAP(ssid, pass);
  }
  delay(1000);
  server.begin();
}

void loop() {
  // Obsługa przycisku (zmiana trybu)
  if (digitalRead(button)) {
    mode++;
    delay(400);
  }

  if (mode > 3 && mode % 2 == 0) {
    handleWiFi();
  } else if (mode > 3 && mode % 2 == 1) {
    mode = 3; // wracamy do trybu jazdy
  }

  if (mode == 3) {
    // Twoja logika jazdy z PID
    readSensors();
    runPID();
  }

  // Kalibracje i inne tryby, bez zmian
  if (mode == 1 && !blackCali) {
    calibrateBlack();
  }
  if (mode == 2 && !whiteCali) {
    calibrateWhite();
  }
  
  iteration++;
  delay(10);
}

void handleWiFi() {
  WiFiClient client = server.available();
  if (client) {
    String req = client.readStringUntil('\r');
    client.readStringUntil('\n'); // usuń \n

    if (req.startsWith("GET / ")) {
      sendHTML(client);
    } else if (req.startsWith("GET /?")) {
      updatePDValuesFromRequest(req);
      sendPlain(client, "Zaktualizowano Kp i Kd");
      mode = 3; // wracamy do jazdy po aktualizacji
    } else {
      sendPlain(client, "Nieznane żądanie");
    }
    delay(1);
    client.stop();
  }
}

void sendHTML(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println(htmlPage());
}

void sendPlain(WiFiClient &client, const char* msg) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();
  client.println(msg);
}

void readSensors() {
  for (int i = 0; i < 7; i++) {
    analogValues[i] = analogRead(analogPins[i]);
  }
}

void calibrateBlack() {
  for (int i = 0; i < 7; i++) {
    blackLevels[i] = analogRead(analogPins[i]);
  }
  blackCali = true;
}

void calibrateWhite() {
  for (int i = 0; i < 7; i++) {
    whiteLevels[i] = analogRead(analogPins[i]);
  }
  whiteCali = true;
}

void runPID() {
  speedRatio = constrain(1 - float(analogRead(speedsetter))/150, 0, 1);
  
  if(sumSensorsAnalog() < 70){ // stop gdy podniesiemy robota lub gdy najedzie prostopadle na linie!!
    leftMotor(0);
    rightMotor(0);
    return;
  }

  float sensor_weights[7] = {-12.0, -9.0, -4.0, 0.0, 4.0, 9.0, 12.0};
  float line_error = 0.0;
  int count = 0;

  for (int i = 0; i < 7; i++) {
    if (caliValues[i] == -1) {
      line_error += sensor_weights[i];
      count++;
    }
  }

  if (count > 0) {
    line_error /= count;
  } else {
    line_error = 0;
  }

  static float last_error = 0;
  float derivative = line_error - last_error;
  float correction = Kp * line_error + Kd * derivative;
  last_error = line_error;

  float base_speed = constrain(130.0 - abs(line_error) * 10.0, 80.0, 130.0);

  float left_speed = base_speed + correction;
  float right_speed = base_speed - correction;

  leftMotor(left_speed);
  rightMotor(right_speed);

  delay(15);
}

int sumSensorsAnalog(){
  int x = 0;
  for(int i = 0; i < 7; i++){
    x += analogValues[i];
  }
  return x;
}

// Silniki
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
}

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
}
