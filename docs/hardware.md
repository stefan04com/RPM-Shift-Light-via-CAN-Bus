# Documentație Tehnică: Sistem de Monitorizare RPM via CAN-Bus (OBD-II)
**Platformă:** ATmega328P Xplained Mini
**Vehicul vizat:** Ford Fiesta 2007 (Protocol ISO 15765-4 CAN)

---

## 1. Descriere generală a proiectului și workflow

### 1.1. Obiectiv
Proiectul vizează realizarea unui instrument de diagnoză activă care extrage în timp real turația motorului (RPM) dintr-un autovehicul folosind magistrala CAN-Bus. Datele sunt procesate și afișate dual: numeric (pe un ecran LCD) și vizual (printr-un sistem de 3 LED-uri / modul semafor tip "Shift Light").

### 1.2. Workflow-ul sistemului
1.  **Inițializare:** La alimentare, ATmega328P configurează perifericele SPI (pentru modulul CAN) și I2C (pentru LCD). Modulul MCP2515 este setat la viteza de 500kbps (specifică Ford Fiesta 2007).
2.  **Interogare (Polling/Interrupt):** Microcontrolerul trimite prin SPI o cerere CAN către ECU-ul mașinii pentru PID-ul `0x0C` (Engine RPM). De asemenea, "ascultă" în timp real mesajele broadcast trimise automat de ECU pe ID-ul `0x201`.
3.  **Recepție:** Când ECU răspunde, modulul MCP2515 primește pachetul și declanșează o întrerupere hardware pe pinul D2 (PD2) al ATmega.
4.  **Procesare:** Microcontrolerul citește datele și aplică formula: `RPM = ((A * 256) + B) / 4`.
5.  **Ieșire:** * Valoarea este trimisă către LCD via I2C.
    * Logica GPIO verifică pragurile de turație și aprinde LED-ul corespunzător (Verde, Galben sau Roșu).
6.  **Debug:** Valorile sunt transmise simultan prin UART către laptop pentru monitorizare (opțional).

---

## 2. Detaliere piese folosite

* **Microcontroler:** ATmega328P (Xplained Mini) – Procesează datele și gestionează protocoalele SPI, I2C și UART.
* **Interfață CAN:** Modulul CAN bus MCP2515 + TJA1050 este o soluție esențială pentru comunicațiile pe rețelele CAN (Controller Area Network). Modulul include atât controlerul CAN (MCP2515), cât și transceiverul CAN (TJA1050), permițând conectarea la microcontroler via SPI. Asigură o comunicare eficientă și fiabilă în timp real (5V DC, 1Mb/s, rezistență de terminare 120Ω).
* **Alimentare (Sursă Step-Down):** Modul DC-DC Step-Down MP1584EN. Preia tensiunea bateriei mașinii (aprox. 12V-14.4V) direct din portul OBD-II și o coboară (prin comutație eficientă, fără a se încinge) la 5.0V pentru a alimenta în siguranță întregul circuit.
* **Afișaj Alpha-numeric:** LCD 1602 cu rucsac I2C (PCF8574). Afișează 16 coloane pe 2 rânduri. Necesită doar 2 pini de date (SDA pe PC4, SCL pe PC5).
* **Sistem Vizual:** Modul 3 LED-uri (Semafor) sau LED-uri discrete:
    * 1x LED Verde (Turație optimă)
    * 1x LED Galben (Limită superioară)
    * 1x LED Roșu (Critical / Shift)
    * Rezistențe 330Ω (Limitare curent LED-uri pe traseul de semnal).
* **Conectivitate Auto:** Mufă OBD-II Tată (Pigtail) cu 16 pini.
* **Infrastructură:** Breadboard, Set fire Dupont.

---

## 3. Descrierea exactă a legăturilor hardware

### 3.1. Alimentare și Masă (Modul "Standalone" via OBD-II)
Sistemul este alimentat direct din mașină, independent de laptop. **Atenție:** Potențiometrul modulului Step-Down trebuie setat obligatoriu la 5.0V înainte de a-l conecta la restul circuitului!

* **Intrare alimentare (din mașină):** * Pinul 16 (OBD-II) se conectează la `IN+` (Modul MP1584EN).
    * Pinul 4 sau 5 (OBD-II GND) se conectează la `IN-` (Modul MP1584EN).
* **Ieșire alimentare (spre circuit):**
    * `OUT+` (Modul MP1584EN) se conectează la șina ROȘIE (+) a breadboard-ului.
    * `OUT-` (Modul MP1584EN) se conectează la șina ALBASTRĂ/NEAGRĂ (-) a breadboard-ului.
* **Alimentare Microcontroler:** Pinul `5V` și pinul `GND` de pe ATmega328P se conectează la șinele de alimentare de pe breadboard.

### 3.2. Magistrala SPI (ATmega $\leftrightarrow$ MCP2515)
| MCP2515 Pin | ATmega328P Pin (Port) | Descriere |
| :--- | :--- | :--- |
| VCC | 5V Rail | Alimentare modul |
| GND | GND Rail | Masă |
| CS | Pin 10 (PB2) | Chip Select |
| SI | Pin 11 (PB3) | Master Out Slave In (MOSI) |
| SO | Pin 12 (PB4) | Master In Slave Out (MISO) |
| SCK | Pin 13 (PB5) | Serial Clock |
| INT | Pin 2 (PD2) | Întrerupere Hardware (INT0) |

### 3.3. Magistrala I2C (ATmega $\leftrightarrow$ LCD 1602 I2C)
| LCD I2C Pin | ATmega328P Pin (Port) | Descriere |
| :--- | :--- | :--- |
| VCC | 5V Rail | Alimentare ecran |
| GND | GND Rail | Masă |
| SDA | A4 (PC4) | Serial Data |
| SCL | A5 (PC5) | Serial Clock |

### 3.4. Conexiunea CAN (MCP2515 $\leftrightarrow$ OBD-II Mașină)
| MCP2515 Terminal | OBD-II Pin | Descriere |
| :--- | :--- | :--- |
| H (CAN-H) | Pin 6 | CAN High (Port Diagnoză) |
| L (CAN-L) | Pin 14 | CAN Low (Port Diagnoză) |

### 3.5. Configurație LED-uri (GPIO Shift Light)
Modulul tip semafor este conectat folosind rezistențe de 330Ω inseriate între pinul plăcii și pinul modulului, pentru protecție. Pinul GND al modulului LED merge la șina de Masă.

| Modul LED | ATmega328P Pin (Port) | Prag de Turație |
| :--- | :--- | :--- |
| G (Verde) | Pin 3 (PD3) | RPM < 2500 |
| Y (Galben)| Pin 4 (PD4) | 2500 <= RPM < 4500 |
| R (Roșu)  | Pin 5 (PD5) | RPM >= 4500 |

---

## 4. Note tehnice de configurare

1.  **Viteza CAN:** Pentru Ford Fiesta 2007, viteza magistralei este de **500 KBPS**.
2.  **Frecvență Modul:** Configurată în cod la `MCP_8MHZ`.
3.  **Adresă I2C LCD:** Adresa standard identificată și folosită este `0x27`.
4.  **Recepție asincronă:** S-a implementat logică de citire în cascadă (`while (CAN_MSGAVAIL == CAN.checkReceive())`) pentru a preveni blocarea microcontrolerului pe erori de tip *Interrupt Overrun* în timpul fluxului intens de date auto.

---

## 5. Ghid Rapid de Cablare (Pinout Complet)
Această listă poate fi folosită ca un checklist pas-cu-pas pentru a reface întregul circuit fizic de la zero:

### A. Alimentarea (OBD-II -> Modul Step-Down -> Breadboard)
* [ ] OBD-II **Pin 16** $\rightarrow$ Modul Step-Down **IN+**
* [ ] OBD-II **Pin 4 (sau 5)** $\rightarrow$ Modul Step-Down **IN-**
* [ ] Modul Step-Down **OUT+** $\rightarrow$ Șina **ROȘIE (5V)** pe breadboard
* [ ] Modul Step-Down **OUT-** $\rightarrow$ Șina **ALBASTRĂ (GND)** pe breadboard

### B. Mesele și Alimentările Componentelor
* [ ] ATmega328P **5V** $\rightarrow$ Șina **ROȘIE**
* [ ] ATmega328P **GND** $\rightarrow$ Șina **ALBASTRĂ**
* [ ] LCD **VCC** $\rightarrow$ Șina **ROȘIE**
* [ ] LCD **GND** $\rightarrow$ Șina **ALBASTRĂ**
* [ ] MCP2515 **VCC** $\rightarrow$ Șina **ROȘIE**
* [ ] MCP2515 **GND** $\rightarrow$ Șina **ALBASTRĂ**
* [ ] Modul Semafor **GND** $\rightarrow$ Șina **ALBASTRĂ**

### C. Firele de Semnal și Comunicație
* **Modulul MCP2515 (SPI & CAN)**
    * [ ] MCP2515 **H** $\rightarrow$ OBD-II **Pin 6**
    * [ ] MCP2515 **L** $\rightarrow$ OBD-II **Pin 14**
    * [ ] MCP2515 **CS** $\rightarrow$ ATmega **Pin 10 (PB2)**
    * [ ] MCP2515 **SI** $\rightarrow$ ATmega **Pin 11 (PB3)**
    * [ ] MCP2515 **SO** $\rightarrow$ ATmega **Pin 12 (PB4)**
    * [ ] MCP2515 **SCK** $\rightarrow$ ATmega **Pin 13 (PB5)**
    * [ ] MCP2515 **INT** $\rightarrow$ ATmega **Pin 2 (PD2)**

* **Ecranul LCD (I2C)**
    * [ ] LCD **SDA** $\rightarrow$ ATmega **A4 / PC4** (sau pinul SDA dedicat)
    * [ ] LCD **SCL** $\rightarrow$ ATmega **A5 / PC5** (sau pinul SCL dedicat)

* **Modul Semafor (LED-uri prin rezistențe 330Ω)**
    * [ ] Modul Semafor **G (Verde)** $\rightarrow$ Rezistență $\rightarrow$ ATmega **Pin 3 (PD3)**
    * [ ] Modul Semafor **Y (Galben)** $\rightarrow$ Rezistență $\rightarrow$ ATmega **Pin 4 (PD4)**
    * [ ] Modul Semafor **R (Roșu)** $\rightarrow$ Rezistență $\rightarrow$ ATmega **Pin 5 (PD5)**