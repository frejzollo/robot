const int speedsetter = A0;
const int button = 13;

int Sensors[7] = {A1, A2, A3, A4, A5, A6, A7};
int AnalogValues[7];
int CaliValues[7];

void setup() {
  Serial.begin(9600);  // Uruchomienie komunikacji szeregowej
}

void loop() {
basic_info();
}

void basic_info()
  {
  for (int i = 0; i <= 7; i++) {
    int value = analogRead(i);  // Odczyt z wejścia analogowego (A0 to 0, A1 to 1, ..., A7 to 7)
    Serial.print("A");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(value);
    Serial.print("   ");
  }
  Serial.println();  // Nowa linia po wszystkich odczytach
  delay(500);        // Małe opóźnienie dla czytelności
};


