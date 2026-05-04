# Documentație Tehnică: Implementare Software (Monitorizare RPM via CAN-Bus)
**Platformă:** ATmega328P Xplained Mini
**Limbaj de programare:** C/C++ (cu utilizarea bibliotecilor standard pentru microcontrolere)

---

## 1. Mediu de Dezvoltare și Biblioteci

Proiectul este implementat utilizând un mediu de dezvoltare compatibil cu arhitectura AVR (ex. Arduino IDE sau Microchip Studio cu extensii corespunzătoare), pentru a facilita utilizarea bibliotecilor testate în mediul auto.

### Biblioteci necesare:
1. **`SPI.h`**: Bibliotecă standard pentru gestionarea protocolului Serial Peripheral Interface (comunicarea cu MCP2515).
2. **`Wire.h`**: Bibliotecă standard pentru protocolul I2C (comunicarea cu ecranul LCD).
3. **`mcp_can.h`**: Bibliotecă specifică (ex. dezvoltată de *coryjfowler*) pentru controlul modulului MCP2515. Gestionează registrele complexe ale controllerului CAN.
4. **`LiquidCrystal_I2C.h`**: Bibliotecă pentru controlul afișajelor alfanumerice dotate cu expandor I2C (PCF8574).

---

## 2. Concepte Software Integrate (Syllabus PM)

Proiectul implementează software 4 concepte fundamentale din studiul microprocesoarelor:

* **GPIO (Laborator 0):** Configurarea registrelor de direcție (DDR) pentru pinii D3, D4, D5 ca ieșiri (`OUTPUT`) pentru a controla LED-urile indicatoare de turație.
* **Întreruperi Externe (Laborator 2):** Utilizarea pinului digital D2 (INT0 pe ATmega328P). Când modulul MCP2515 recepționează un mesaj valid de la mașină, trage pinul INT în `LOW`. Acest eveniment declanșează o rutină de tratare a întreruperii (ISR) care semnalează microcontrolerului că datele sunt gata de citire.
* **SPI (Laborator 5):** Inițializarea magistralei SPI (Master mode) pentru a trimite cereri și a citi bufferele din MCP2515.
* **I2C (Laborator 6):** Transmiterea asincronă a șirurilor de caractere către adresa specifică a LCD-ului (de regulă `0x27`).

---

## 3. Configurații Cheie (Parametri Tehnici)

Pentru ca sistemul să poată comunica cu rețeaua ECU a unui Ford Fiesta 2007, software-ul folosește următoarele setări stricte:

### 3.1. Setări MCP2515 (CAN-Bus)
* **Viteză Magistrală (Baudrate):** `CAN_500KBPS` (Standardul High-Speed CAN pentru majoritatea mașinilor OBD-II post-2008 / Ford 2007).
* **Frecvență Oscilator Modul:** `MCP_8MHZ` sau `MCP_16MHZ` (Setarea din cod trebuie să corespundă cu valoarea cristalului de cuarț lipit fizic pe modulul MCP2515).
* **Mod de operare:** `MCP_NORMAL` (permite trimiterea și recepționarea de mesaje pe rețea).

### 3.2. Protocol OBD-II (Cerere și Răspuns)
Comunicarea se face pe baza standardului SAE J1979.
* **ID Cerere (Request):** `0x7DF` (ID standard de broadcast către toate ECU-urile).
* **ID Răspuns (Reply):** `0x7E8` (ID standard de răspuns de la ECU-ul motorului).
* **PID pentru Turație (RPM):** `0x01` (Modul curent de diagnoză) + `0x0C` (Parametrul RPM).
* **Structura Cererii (Pachet 8 bytes):** `{0x02, 0x01, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00}` 
  *(0x02 indică faptul că urmează 2 bytes de date semnificative: modul 01 și PID-ul 0C).*

---

## 4. Arhitectura Codului și Workflow

Logica programului este împărțită în două secțiuni principale: Inițializare și Bucla infinită (State Machine).

### Pasul 1: `Setup` (Inițializarea sistemului)
1.  **Serial Monitor:** Inițializare UART (ex. `115200 baud`) pentru debugging pe laptop.
2.  **LCD:** Inițializare I2C, pornire backlight, afișare mesaj "Init CAN...".
3.  **CAN:** Inițializare modul MCP2515 la 500kbps. Verificarea succesului comunicării SPI. Dacă eșuează, sistemul se blochează (buclă infinită de eroare). Dacă reușește, modulul este trecut în Normal Mode.
4.  **GPIO & Întreruperi:** Setarea pinilor D3, D4, D5 ca OUTPUT. Setarea pinului D2 ca INPUT cu pull-up (`INPUT_PULLUP`). Atașarea întreruperii (`FALLING` edge) pe pinul D2.

### Pasul 2: `Loop` (Funcționarea în timp real)
1.  **Trimitere Cerere OBD-II:** La un interval fix (ex. 100ms, gestionat prin polling de timp, fără funcții blocante tip `delay`), microcontrolerul trimite pachetul cerere RPM (`0x7DF`) pe magistrala CAN.
2.  **Așteptare Întrerupere:** Microcontrolerul își continuă execuția până când funcția ISR (Interrupt Service Routine) setează un "steag" (flag logic `volatile bool rxFlag = true`) care confirmă recepția unui pachet de la mașină.
3.  **Citire Buffer CAN:** Dacă `rxFlag` este activ, se citește mesajul din MCP2515 (ID-ul expeditorului, lungimea și datele). Se resetează `rxFlag = false`.
4.  **Filtrare și Decodare:** Se verifică dacă ID-ul recepționat este `0x7E8` și dacă byte-ul care indică PID-ul este `0x0C`.
    * Dacă validarea este cu succes, se extrag Byte-ul A (Date[3]) și Byte-ul B (Date[4]).
    * Se calculează turația folosind formula OBD-II standard: **RPM = ((A * 256) + B) / 4**
5.  **Afișare LCD:** Se curăță rândul 2 al ecranului și se scrie noua valoare: "RPM: [valoare]".
6.  **Logica Shift-Light (GPIO):** * Dacă RPM < 2500: Pin D3 `HIGH`, restul `LOW` (Verde).
    * Dacă RPM >= 2500 și RPM < 4500: Pinii D3, D4 `HIGH`, D5 `LOW` (Verde + Galben).
    * Dacă RPM >= 4500: Pinii D3, D4, D5 `HIGH` (Verde + Galben + Roșu).

---

## 5. Implementarea Rutinei de Întrerupere (ISR)

Pentru a respecta bunele practici în programarea microcontrolerelor, ISR-ul va fi extrem de scurt, neconținând calcule sau funcții blocante (cum ar fi printarea pe Serial sau I2C).

**Pseudo-cod ISR:**
```c
volatile bool canMessageReceived = false;

void mcp2515_ISR() {
  canMessageReceived = true; // Setează flag-ul pentru bucla principală
}