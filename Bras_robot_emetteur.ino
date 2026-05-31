#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

// 🔴 REMPLACEZ PAR L'ADRESSE MAC DE L'ESP32 ESCLAVE 🔴
uint8_t broadcastAddress[] = {0x30, 0x76, 0xF5, 0xA6, 0x17, 0x90};

const int pot1 = 32;
const int pot2 = 33;
const int pot3 = 34;
const int pot4 = 35;

typedef struct struct_message {
  int angle1;
  int angle2;
  int angle3;
  int angle4;
} struct_message;

struct_message brasData;
esp_now_peer_info_t peerInfo;

// --- VARIABLES POUR LE FILTRE DE LISSAGE ---
// Le facteur de lissage (entre 0.0 et 1.0)
// 0.1 = Très fluide mais un peu lent (beaucoup d'inertie)
// 0.5 = Plus réactif mais moins filtré
// 1.0 = Aucun lissage (tremblements)
const float LISSAGE = 0.5; 

// Variables pour stocker les valeurs filtrées (en float pour la précision)
float valFiltree1 = 0;
float valFiltree2 = 0;
float valFiltree3 = 0;
float valFiltree4 = 0;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Erreur d'initialisation ESP-NOW");
    return;
  }

  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 1;  
  peerInfo.encrypt = false;
  
  esp_now_add_peer(&peerInfo);
  
  // Initialiser les filtres avec la première lecture pour éviter un démarrage brusque
  valFiltree1 = analogRead(pot1);
  valFiltree2 = analogRead(pot2);
  valFiltree3 = analogRead(pot3);
  valFiltree4 = analogRead(pot4);
}

void loop() {
  // 1. Lecture brute des potentiomètres
  int brut1 = analogRead(pot1);
  int brut2 = analogRead(pot2);
  int brut3 = analogRead(pot3);
  int brut4 = analogRead(pot4);

  // 2. Application du filtre passe-bas (Lissage exponentiel)
  valFiltree1 = (LISSAGE * brut1) + ((1.0 - LISSAGE) * valFiltree1);
  valFiltree2 = (LISSAGE * brut2) + ((1.0 - LISSAGE) * valFiltree2);
  valFiltree3 = (LISSAGE * brut3) + ((1.0 - LISSAGE) * valFiltree3);
  valFiltree4 = (LISSAGE * brut4) + ((1.0 - LISSAGE) * valFiltree4);

  // 3. Mappage des valeurs filtrées vers les angles du bras
  // On convertit le float (valFiltree) en int au moment du map()
  brasData.angle1 = map((int)valFiltree1, 0, 4095, 10, 90);
  brasData.angle2 = map((int)valFiltree2, 0, 4095, 0, 180);
  brasData.angle3 = map((int)valFiltree3, 0, 4095, 0, 170);
  brasData.angle4 = map((int)valFiltree4, 0, 4095, 0, 180);

  // 4. Envoi des données
  esp_now_send(broadcastAddress, (uint8_t *) &brasData, sizeof(brasData));
  
  // Pause nécessaire pour laisser l'ADC et le Wi-Fi respirer
  delay(50); 
}