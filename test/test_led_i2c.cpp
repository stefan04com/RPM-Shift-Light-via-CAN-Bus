#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Arduino.h>

// Inițializare LCD la adresa I2C standard 0x27, cu 16 coloane și 2 rânduri
LiquidCrystal_I2C lcd(0x27, 16, 2); 

// Definirea pinilor GPIO pentru LED-uri
const int LED_VERDE = A2; // PC2
const int LED_GALBEN = 4;
const int LED_ROSU = 5;

// Nota: A4/A5 sunt folosite de I2C și corespund PC4/PC5 pe ATmega328P

// Variabile pentru simularea turației
int simulatedRPM = 800;      // RPM de pornire (relanti)
int rpmStep = 150;           // Cu cât crește/scade RPM-ul la fiecare ciclu

// Variabile pentru Timer (Non-blocking)
unsigned long previousMillis = 0; 
const long interval = 50;    // Intervalul de actualizare: 50 milisecunde

void setup() {
  // Configurare pini ca Ieșiri (Laboratorul 0: GPIO)
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_GALBEN, OUTPUT);
  pinMode(LED_ROSU, OUTPUT);

  // Inițializare protocol I2C și LCD (Laboratorul 6: I2C)
  lcd.init();
  lcd.backlight();
  
  // Mesaj de întâmpinare
  lcd.setCursor(0, 0);
  lcd.print("Status: TEST HIL");
  
  // Asigurare că LED-urile sunt stinse la pornire
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_GALBEN, LOW);
  digitalWrite(LED_ROSU, LOW);
}

void loop() {
  // Preluăm timpul curent bazat pe Timer0 hardware
  unsigned long currentMillis = millis();

  // Verificăm dacă au trecut 50ms de la ultima actualizare
  if (currentMillis - previousMillis >= interval) {
    // Salvăm noul timp
    previousMillis = currentMillis;

    // 1. GENERARE SEMNAL SIMULAT
    simulatedRPM += rpmStep;
    
    // Inversăm direcția turației dacă atinge limitele (simulăm accelerație/decelerație)
    if (simulatedRPM >= 5500 || simulatedRPM <= 800) {
      rpmStep = -rpmStep; 
    }

    // 2. AFIȘARE PE LCD (I2C)
    lcd.setCursor(0, 1); // Rândul 2, caracterul 1
    lcd.print("RPM: ");
    lcd.print(simulatedRPM);
    lcd.print("   "); // Spații goale pentru a șterge "urmele" caracterelor vechi

    // 3. LOGICA SHIFT LIGHT (GPIO)
    // Turație optimă (< 2500)
    if (simulatedRPM < 2500) {
      digitalWrite(LED_VERDE, HIGH);
      digitalWrite(LED_GALBEN, LOW);
      digitalWrite(LED_ROSU, LOW);
    } 
    // Turație medie-înaltă (2500 - 4500)
    else if (simulatedRPM >= 2500 && simulatedRPM < 4500) {
      digitalWrite(LED_VERDE, HIGH);
      digitalWrite(LED_GALBEN, HIGH);
      digitalWrite(LED_ROSU, LOW);
    } 
    // Limită superioară - Zona Roșie (>= 4500)
    else {
      digitalWrite(LED_VERDE, HIGH);
      digitalWrite(LED_GALBEN, HIGH);
      digitalWrite(LED_ROSU, HIGH); // Semnal de schimbare viteză
    }
  }
}