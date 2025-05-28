const float VREF = 3.3;  // Zmieniamy z 5.0 na 3.3V

void setup() {
  Serial.begin(9600);  // Uruchomienie komunikacji szeregowej
}

void loop() {
  basic_info();
}

void basic_info() {
  for (int i = 0; i <= 7; i++) {
    int value = analogRead(i);  // Odczyt z wejścia analogowego
    float voltage = (value / 1023.0) * VREF;  // Przeliczenie na napięcie

    Serial.print("A");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(value);
    Serial.print(" (");
    Serial.print(voltage, 3);  // 3 miejsca po przecinku
    Serial.print(" V)   ");
  }
  Serial.println();  // Nowa linia po wszystkich odczytach
  delay(500);        // Małe opóźnienie dla czytelności
}
