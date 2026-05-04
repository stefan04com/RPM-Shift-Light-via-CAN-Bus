#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <mcp_can.h>

// --- CONFIGURARE PINI (Porturile AVR) ---
// PORT D: GPIO și Întreruperi
const int PIN_INT = 2;       // PD2: Întrerupere CAN (INT0)
const int LED_VERDE = 3;     // PD3: Shift Light (Optim)
const int LED_GALBEN = 4;    // PD4: Shift Light (Avertizare)
const int LED_ROSU = 5;      // PD5: Shift Light (Schimbă viteza)

// PORT B: SPI Chip Select
const int PIN_CS = 10;       // PB2: Chip Select MCP2515

// NOTĂ HARDWARE I2C: 
// PC4 (A4) = SDA, PC5 (A5) = SCL (Fixați hardware pe ATmega328P)

// --- INIȚIALIZARE MODULE ---
LiquidCrystal_I2C lcd(0x27, 16, 2); 
MCP_CAN CAN(PIN_CS);                

// --- VARIABILE GLOBALE ---
volatile bool canMessageReceived = false; 
unsigned long previousMillis = 0;         
const long interval = 100;                // Interval polling OBD-II
int currentRPM = 0;                       
bool lcdConnected = false;                

// --- RUTINA DE TRATARE A ÎNTRERUPERII (ISR) ---
void canISR() {
  canMessageReceived = true;
}

// Funcție separată pentru actualizarea LED-urilor
void updateShiftLight(int rpm) {
  if (rpm < 2500) {
    digitalWrite(LED_VERDE, HIGH);
    digitalWrite(LED_GALBEN, LOW);
    digitalWrite(LED_ROSU, LOW);
  }
  else if (rpm >= 2500 && rpm < 3500) {
    digitalWrite(LED_VERDE, HIGH);
    digitalWrite(LED_GALBEN, HIGH);
    digitalWrite(LED_ROSU, LOW);
  } 
  else {
    digitalWrite(LED_VERDE, HIGH);
    digitalWrite(LED_GALBEN, HIGH);
    digitalWrite(LED_ROSU, HIGH);
  }
}

void setup() {
  Serial.begin(57600);
  delay(500); 
  
  Serial.println("\n--- INIT START ---");
  
  // 1. Configurare GPIO (Port D)
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_GALBEN, OUTPUT);
  pinMode(LED_ROSU, OUTPUT);
  
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_GALBEN, LOW);
  digitalWrite(LED_ROSU, LOW);

  // 2. Inițializare I2C și LCD
  Wire.begin(); 
  delay(100);
  
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Init CAN-Bus...");
  lcdConnected = true; 
  Serial.println("[LCD] Init complete (PC4=SDA, PC5=SCL)");

  // 3. Inițializare CAN-Bus
  if (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("[CAN] MCP2515 initialized successfully!");
    if (lcdConnected) {
      lcd.setCursor(0, 1);
      lcd.print("CAN OK! 500kbps ");
    }
  } else {
    Serial.println("[CAN] ERROR: MCP2515 init failed!");
    if (lcdConnected) {
      lcd.setCursor(0, 1);
      lcd.print("Eroare CAN!     ");
    }
    while (1); 
  }

  CAN.setMode(MCP_NORMAL);

  // 4. Configurare Întrerupere
  pinMode(PIN_INT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_INT), canISR, FALLING);
  
  delay(1000);
  if (lcdConnected) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PM Fair 2026");
  }
  Serial.println("--- INIT COMPLETE ---\n");
}

void loop() {
  unsigned long currentMillis = millis();

  // --- 1. TRIMITERE CERERE OBD-II ---
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    byte requestData[8] = {0x02, 0x01, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00};
    CAN.sendMsgBuf(0x7DF, 0, 8, requestData);
  }

  // --- 2. RECEPȚIE ROBUSTĂ CAN-BUS ---
  // Condiția OR (||) asigură că nu ne blocăm dacă pinul rămâne forțat LOW
  if (canMessageReceived || digitalRead(PIN_INT) == LOW) {
    canMessageReceived = false; 

    // Citim TOATE mesajele din bufferul modulului într-o cascadă
    while (CAN_MSGAVAIL == CAN.checkReceive()) {
      long unsigned int rxId;
      unsigned char len = 0;
      unsigned char rxBuf[8];

      CAN.readMsgBuf(&rxId, &len, rxBuf);

      // --- DECODARE A: Răspuns OBD-II Standard (0x7E8) ---
      if (rxId == 0x7E8 && len >= 5 && rxBuf[1] == 0x41 && rxBuf[2] == 0x0C) {
        currentRPM = ((rxBuf[3] * 256) + rxBuf[4]) / 4;
        
        Serial.print("[OBD-II] RPM Calculat: ");
        Serial.println(currentRPM);

        if (lcdConnected) {
          lcd.setCursor(0, 1);
          lcd.print("RPM: ");
          lcd.print(currentRPM);
          lcd.print("    "); 
        }
        updateShiftLight(currentRPM);
      }
      
      // --- DECODARE B: Broadcast rapid specific Ford (0x201) ---
      // Mașina ta trimite singură ID-ul 0x201! Primii 2 bytes conțin turația.
      else if (rxId == 0x201 && len >= 2) {
        // Formula pentru broadcast direct Ford (Bytes 0 și 1)
        int directRPM = ((rxBuf[0] * 256) + rxBuf[1]) / 4;
        
        // Evităm fluctuațiile absurde sau datele corupte
        if (directRPM >= 0 && directRPM <= 8000) {
            currentRPM = directRPM;
            
            Serial.print("[Ford Fast-Data 0x201] RPM: ");
            Serial.println(currentRPM);

            if (lcdConnected) {
              lcd.setCursor(0, 1);
              lcd.print("RPM: ");
              lcd.print(currentRPM);
              lcd.print("    "); 
            }
            updateShiftLight(currentRPM);
        }
      }
      
      // Opțional: Printează restul ID-urilor necunoscute pentru viitoare analize
      else if (rxId != 0x7E8 && rxId != 0x201) {
         Serial.print("[CAN-RAW] ID: 0x");
         Serial.println(rxId, HEX);
      }
    }
  }
}