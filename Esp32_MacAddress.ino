#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(1000); // Laisse un instant au port série pour se stabiliser

  // Réinitialise et active le Wi-Fi de manière forcée
  WiFi.disconnect(true);
  delay(100); 
  WiFi.mode(WIFI_STA); 
  delay(100); // Laisse le temps à l'antenne de s'activer

  Serial.print("Mac address: ");
  Serial.println(WiFi.macAddress());
}

void loop() {
  // Le programme tourne en boucle ici, 
  // même s'il n'y a rien à faire pour le moment.
}