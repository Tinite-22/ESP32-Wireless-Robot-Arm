#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <ESP32Servo.h> 

// Création des instances pour contrôler chaque moteur indépendamment
Servo servo1;
Servo servo2;
Servo servo3;
Servo servo4;

// Définition des broches de sortie (PWM) de l'ESP32. 
// Ces broches généreront le signal carré qui indique l'angle au servomoteur.
const int pinS1 = 13;
const int pinS2 = 12;
const int pinS3 = 14;
const int pinS4 = 26;

// La broche 2 correspond à la LED bleue intégrée sur la majorité des cartes ESP32
const int pinLED = 2;

// Définition du "moule" des données. Il est crucial que l'empreinte mémoire 
// (le nombre et le type de variables) soit rigoureusement identique entre 
// l'Émetteur et le Récepteur pour que le paquet soit décodé correctement.
typedef struct struct_message {
  int angle1;
  int angle2;
  int angle3;
  int angle4;
} struct_message;

// Instanciation de la structure pour stocker les valeurs reçues
struct_message brasData;

// --- FONCTION DE RAPPEL (CALLBACK) ---
// Cette fonction ne tourne pas dans le loop(). Elle est déclenchée de manière 
// asynchrone (interruption matérielle) par le module Wi-Fi dès qu'un paquet est validé.
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  
  // Transfert de la mémoire : on copie brutalement les octets bruts (incomingData) 
  // dans notre structure formatée (brasData) pour pouvoir lire les variables.
  memcpy(&brasData, incomingData, sizeof(brasData));
  
  // 1. Débogage visuel sur le Moniteur Série
  Serial.print("\nPaquet reçu (");
  Serial.print(len);
  Serial.print(" octets) -> ");
  Serial.print("S1: "); Serial.print(brasData.angle1);
  Serial.print("° | S2: "); Serial.print(brasData.angle2);
  Serial.print("° | S3: "); Serial.print(brasData.angle3);
  Serial.print("° | S4: "); Serial.println(brasData.angle4);

  // 2. Témoin visuel physique
  // On lit l'état actuel de la broche (0 ou 1) et on inverse cet état avec '!'. 
  // Cela fait clignoter la LED à la fréquence exacte de réception des paquets.
  digitalWrite(pinLED, !digitalRead(pinLED)); 

  // 3. Commande matérielle
  // L'ESP32 met à jour le rapport cyclique (duty cycle) des signaux PWM 
  // sur les 4 broches pour forcer les servomoteurs à rejoindre la nouvelle position.
  servo1.write(brasData.angle1);
  servo2.write(brasData.angle2);
  servo3.write(brasData.angle3);
  servo4.write(brasData.angle4);
}

void setup() {
  Serial.begin(115200);
  
  // Configuration de la broche de la LED interne comme une sortie de courant
  pinMode(pinLED, OUTPUT);
  digitalWrite(pinLED, LOW); // Éteindre par défaut

  // --- CONFIGURATION RADIO ---
  // Le mode WIFI_STA (Station) désactive le mode Point d'Accès (WIFI_AP).
  // Cela libère des ressources processeur et dédie l'antenne à la réception ESP-NOW.
  WiFi.mode(WIFI_STA);
  
  // L'ESP-NOW n'a pas de mécanisme de recherche de canal. On force donc 
  // le matériel radio à écouter exclusivement la fréquence spécifique du canal 1 (2412 MHz).
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  // --- CONFIGURATION DES TIMERS MATÉRIELS ---
  // L'ESP32 possède des générateurs PWM indépendants du processeur central.
  // On alloue ici 4 canaux matériels (timers) distincts pour garantir 
  // un signal très stable sans faire trembler les servomoteurs.
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  
  // La fréquence standard pour les servomoteurs analogiques classiques est de 50 Hz 
  // (un signal envoyé toutes les 20 millisecondes).
  servo1.setPeriodHertz(50);
  servo2.setPeriodHertz(50);
  servo3.setPeriodHertz(50);
  servo4.setPeriodHertz(50);

  // Attachement des broches aux timers. 
  // Les paramètres 500 et 2400 sont la durée d'impulsion minimum et maximum en microsecondes.
  // Ils définissent les limites physiques des servomoteurs (0 degrés = 500us, 180 degrés = 2400us).
  servo1.attach(pinS1, 500, 2400);
  servo2.attach(pinS2, 500, 2400);
  servo3.attach(pinS3, 500, 2400);
  servo4.attach(pinS4, 500, 2400);

  // Initialisation à la position de sécurité pour éviter les chocs mécaniques au démarrage.
  servo1.write(70);
  servo2.write(70);
  servo3.write(100);
  servo4.write(0);

  // --- DÉMARRAGE DU PROTOCOLE ---
  // Initialisation de la couche logicielle ESP-NOW.
  if (esp_now_init() != ESP_OK) {
    Serial.println("Erreur matérielle lors de l'initialisation ESP-NOW.");
    return; // Stoppe la fonction setup si le module radio ne répond pas
  }
  
  // On indique à la couche logicielle réseau quelle fonction exécuter 
  // lorsqu'un nouveau tampon de données (buffer) est plein.
  esp_now_register_recv_cb(OnDataRecv);
  
  Serial.println("Initialisation terminée. En attente de paquets réseau...");
}

void loop() {
  // Le processeur principal reste inactif ici.
  // L'ESP32 gère la réception Wi-Fi sur son propre cœur matériel ou via les interruptions.
}