void setup() {
  Serial.begin(9600);  // Uruchomienie komunikacji szeregowej
}

void loop() {
  // Pętla przez wszystkie wejścia analogowe A0 do A7
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
}
