# ESP32-Wireless-Robot-Arm

Un système de télécommande sans fil ultra-réactif et à faible latence, conçu pour un bras robotique à 4 degrés de liberté (4-DOF). Utilisant le **ESP-NOW** d'Espressif, cette configuration élimine le besoin de routeurs Wi-Fi locaux, établissant une connexion directe à haut débit entre une plateforme de contrôle analogique et les servomoteurs du bras robotique.

---

## Caractéristiques Principales

* **Latence Zéro-Routeur :** Utilise le protocole pair-à-pair ESP-NOW sur une fréquence radio fixe de 2,4 GHz pour une exécution instantanée des commandes.
* **Performances Sans Tremblement (Jitter-Free) :** Lissage du signal en temps réel utilisant un filtre passe-bas par moyenne mobile exponentielle (EMA) sur les entrées analogiques.
* **Minuterie Matérielle Indépendante :** Exploite des canaux matériels séparés via `ESP32Servo` pour fournir des signaux PWM ultra-stables à 4 moteurs indépendants simultanément.
* **Exécution Asynchrone :** Intercepte les paquets réseau entrants à l'aide d'interruptions matérielles, découplant ainsi le traitement du signal des limitations standard de la boucle du microcontrôleur.

---

## Câblage et Matériel (Hardware Mapping)

### 1. Unité Émettrice (Plateforme de Contrôle)

Lit 4 potentiomètres linéaires pour déterminer les positions souhaitées du bras.

| Composant | Broche ESP32 | Fonction |
| --- | --- | --- |
| **Potentiomètre 1** | GPIO 32 | Contrôle de l'angle de l'articulation 1 |
| **Potentiomètre 2** | GPIO 33 | Contrôle de l'angle de l'articulation 2 |
| **Potentiomètre 3** | GPIO 34 | Contrôle de l'angle de l'articulation 3 |
| **Potentiomètre 4** | GPIO 35 | Contrôle de l'angle de l'articulation 4 |

### 2. Unité Réceptrice (Bras Robotique)

Traduit les paquets entrants en coordonnées structurelles directes.

| Composant | Broche ESP32 | Canal du Timer | Limites Cibles du Bras |
| --- | --- | --- | --- |
| **Servo 1** | GPIO 13 | Timer 0 | 10° à 90° |
| **Servo 2** | GPIO 12 | Timer 1 | 0° à 180° |
| **Servo 3** | GPIO 14 | Timer 2 | 0° à 180° |
| **Servo 4** | GPIO 26 | Timer 3 | 0° à 180° |
| **LED d'état** | GPIO 2 | N/A | Bascule à chaque paquet de données |

---

## Installation et Configuration Étape par Étape

### Étape 1 : Extraire l'Adresse MAC du Récepteur

ESP-NOW exige un adressage MAC explicite pour cibler les modules.

1. Flashez `Esp32_MacAddress.ino` sur votre **ESP32 Récepteur**.
2. Ouvrez le Moniteur Série (115200 baud) et copiez l'Adresse MAC affichée.

### Étape 2 : Configurer et Déployer l'Émetteur

1. Ouvrez `Robot_emetteur.ino`.
2. Localisez le tableau `broadcastAddress` et remplacez-le par l'adresse MAC cible que vous avez copiée :
```cpp
uint8_t broadcastAddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

```


3. Téléversez le code sur votre **ESP32 Émetteur**.

### Étape 3 : Déployer le Récepteur

1. Téléversez `Robot_Recepteur.ino` sur votre **ESP32 Récepteur**.
2. Maintenez l'unité connectée à une alimentation externe stable de 5V/2A, capable de supporter les appels de courant des éléments mécaniques.

---

## 🛠 Fonctionnement du Code (En Détail)

### 1. Le Moule de Données Partagé (Struct)

L'émetteur et le récepteur partagent exactement la même structure (`struct`). C'est crucial. Lors de l'envoi des données, l'ESP32 envoie un flux d'octets bruts. En définissant une empreinte mémoire stricte des deux côtés, le récepteur sait exactement comment réassembler ces octets en variables entières.

```cpp
typedef struct struct_message {
  int angle1;
  int angle2;
  int angle3;
  int angle4;
} struct_message;

```

### 2. L'Émetteur : Filtre de Lissage Exponentiel

Les potentiomètres analogiques peuvent être bruités. Si nous envoyons directement les données brutes de l'ADC (Convertisseur Analogique-Numérique) aux servomoteurs, le bras robotique va trembler et s'agiter. Pour corriger cela, le code utilise un filtre passe-bas logiciel :

```cpp
valFiltree1 = (LISSAGE * brut1) + ((1.0 - LISSAGE) * valFiltree1);

```

* Si `LISSAGE` est à `0.1`, le bras bouge de manière très fluide mais semble un peu lent (forte inertie).
* Si `LISSAGE` est à `0.5`, il offre un équilibre parfait entre un temps de réaction rapide et le filtrage du bruit électrique.

### 3. L'Émetteur : Mappage de Sécurité

L'ADC de l'ESP32 lit des valeurs allant de 0 à 4095. Avant l'envoi, celles-ci sont mappées (converties) vers des angles physiques sécurisés pour le bras robotique. Cela empêche les servomoteurs d'essayer d'aller au-delà de leurs limites physiques et d'endommager leurs engrenages en plastique ou en métal.

```cpp
brasData.angle1 = map((int)valFiltree1, 0, 4095, 10, 90); // Mappage contraint

```

### 4. Le Récepteur : Timers Matériels PWM

La méthode standard pour générer du PWM (Modulation de Largeur d'Impulsion) de manière logicielle peut faire trembler les servomoteurs si le processeur est occupé à traiter des données Wi-Fi. Ce projet résout ce problème en allouant les timers matériels internes de l'ESP32.

```cpp
ESP32PWM::allocateTimer(0); // ... Alloue des canaux dédiés
servo1.attach(pinS1, 500, 2400);

```

Cela décharge la génération du signal sur du matériel dédié, garantissant qu'un signal carré de 50 Hz parfaitement stable soit envoyé aux servomoteurs, de manière totalement indépendante de la boucle principale du CPU.

### 5. Le Récepteur : Fonction de Rappel (Callback) Asynchrone

La fonction `loop()` du récepteur est complètement vide. La magie opère dans la fonction de rappel `OnDataRecv`.

```cpp
esp_now_register_recv_cb(OnDataRecv);

```

Lorsqu'un paquet valide atteint l'antenne Wi-Fi, il déclenche une interruption matérielle. L'ESP32 met en pause ce qu'il est en train de faire, copie instantanément la mémoire (`memcpy`), écrit les nouveaux angles aux servomoteurs, fait clignoter la LED intégrée pour confirmer la réception, et reprend son cycle. Cela garantit des performances sans aucun décalage (zéro lag).
