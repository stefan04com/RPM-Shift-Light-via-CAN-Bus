Iată template-ul completat cu absolut toate detaliile tehnice, hard și soft, pe care le-am dezvoltat împreună pentru proiectul tău. Fiind un template clasic de DokuWiki (foarte probabil pentru cursul de Proiectare cu Microprocesoare / PM), l-am formatat astfel încât să impresioneze asistentul la evaluare.

Privește acest draft și folosește-l direct:

***

====== Sistem de Monitorizare RPM via CAN-Bus (OBD-II) cu Shift Light ======

===== Introducere =====

Proiectul reprezintă un sistem hardware-software de diagnoză auto activă, conceput pentru a extrage și interpreta date în timp real de pe magistrala CAN (Controller Area Network) a unui autovehicul. 
* **Ce face:** Se conectează la portul OBD-II al mașinii (în cazul de față un Ford Fiesta 2007), interoghează unitatea de control a motorului (ECU) pentru valoarea turației (RPM), procesează pachetele de date și le afișează pe un ecran LCD, controlând simultan un sistem de avertizare vizuală tip "Shift Light" format din 3 LED-uri.
* **Scopul lui:** Realizarea unui instrument "standalone" pentru monitorizarea performanțelor motorului și asistarea șoferului pentru schimbarea optimă a treptelor de viteză.
* **Ideea de la care am pornit:** Dorința de a decoda și vizualiza traficul intern al mașinii (o rețea industrială complexă, extrem de zgomotoasă și aglomerată) fără a folosi testere comerciale (gen ELM327), ci comunicând nativ prin microcontroler.
* **Utilitate:** Demonstrează conceptele critice de rețele auto, gestiunea corectă a întreruperilor hardware pentru prevenirea pierderii de pachete și necesitatea izolării și stabilizării alimentării auto (12V -> 5V) prin convertoare în comutație.

===== Descriere generală =====

Arhitectura sistemului este modulară, fiecare protocol (CAN, SPI, I2C, UART) având un rol bine definit:

*   **Sursa de alimentare auto (OBD-II Pin 16):** Furnizează ~12-14.4V (bateria auto/alternator).
*   **Modul Step-Down DC-DC (MP1584EN):** Preia voltajul instabil al mașinii și îl coboară eficient, prin comutație (fără disipare termică excesivă), la un nivel logic stabil de 5.0V necesar microcontrolerului și ecranului.
*   **Transceiver & Controller CAN (MCP2515 + TJA1050):** Partea de transceiver (TJA1050) transformă semnalele diferențiale (CAN_H și CAN_L) în biți RX/TX. Partea de controller (MCP2515) stochează pachetele în buffere și anunță microcontrolerul printr-un pin de întrerupere hardware (INT).
*   **Unitatea de procesare (ATmega328P Xplained Mini):** Comunică prin SPI cu modulul CAN pentru a trimite cereri (PID-uri) și a citi buffere. Rulează un timer non-blocant, decodează octeții conform standardului ISO 15765-4 și calculează turația.
*   **Afișajul (LCD 1602 + PCF8574):** Primește comenzi prin protocolul I2C (SDA/SCL) de la microcontroler pentru a afișa textul aferent.
*   **Modul Shift Light:** Un grup de 3 LED-uri (Verde, Galben, Roșu) acționate pe pinii GPIO ai ATmega328P.

===== Hardware Design =====

**Listă de piese:**
*   Placă de dezvoltare ATmega328P Xplained Mini
*   Modul CAN-Bus (Controller MCP2515 + Transceiver TJA1050) (Cristal 8MHz)
*   Sursă Coborâtoare (Step-Down Buck Converter) MP1584EN
*   Ecran LCD 1602 cu modul I2C integrat
*   Modul semafor (sau 3x LED-uri cu rezistențe de 330Ω)
*   Cablu / Mufă tată OBD-II pigtail
*   Breadboard și fire conexiune Dupont

**Mapare Pini (Conexiuni logice):**
*   **SPI (MCP2515):** CS -> PB2 (Pin 10), MOSI -> PB3 (Pin 11), MISO -> PB4 (Pin 12), SCK -> PB5 (Pin 13)
*   **Interrupt:** INT -> PD2 (Pin 2) - Configurat pe declanșare FALLING.
*   **I2C (LCD):** SDA -> PC4, SCL -> PC5
*   **GPIO (LED-uri):** Verde -> PD3 (Pin 3), Galben -> PD4 (Pin 4), Roșu -> PD5 (Pin 5)
*   **Alimentare:** OBD-II Pin 16 & Pin 4 -> IN (+/-) Step-Down -> OUT (+/-) ajustat pe multimetru la 5.0V -> Breadboard Power Rails. 

===== Software Design =====

*   **Mediu de dezvoltare:** Proiectul a fost dezvoltat în Visual Studio Code folosind extensia PlatformIO, framework-ul Arduino și toolchain-ul `avr-gcc`. Au fost necesare flag-uri custom (`upload_protocol = custom`) direct către utilitarul `avrdude` pe mediul macOS pentru programarea prin interfața USB nativă mEDBG a plăcii Xplained Mini.
*   **Librării third-party:**
    *   `mcp_can` (by Cory J. Fowler) - Manipularea registrelor interne ale MCP2515 via SPI.
    *   `LiquidCrystal_I2C` - Controlul afișajului.
    *   `Wire.h` și `SPI.h` (Native).
*   **Algoritmi și decizii de implementare:**
    *   **Cod non-blocant:** În loc de folosirea funcției `delay()`, s-a utilizat Timer0 (`millis()`) pentru a interoga CAN-ul la intervale precise de 100ms, lăsând procesorul liber să asculte asincron răspunsurile.
    *   **Interrupt Overrun Handling:** Traficul auto este imens. Dacă modulul nu citește bufferul MCP2515 destul de repede, pinul de întrerupere rămâne blocat pe `LOW`. Rutina de tratare folosește un sistem în cascadă `while (CAN_MSGAVAIL == CAN.checkReceive())` care citește simultan toate mesajele adunate pentru a debloca microcontrolerul.
    *   **Decodare duală:** Software-ul identifică răspunsurile standard la interogări OBD-II (ID `0x7E8`), folosind formula `((A * 256) + B) / 4`. Adițional, funcționează ca un *sniffer* și prinde cadrele transmise broadcast de mașină (ex: ID `0x201` la Ford) care conțin turația în timp real direct în primii 2 octeți, reducând latența pe ecran.

===== Rezultate Obţinute =====

Sistemul a fost testat fizic pe un autoturism Ford Fiesta. A reușit negocierea cu ECU la viteza de 500kbps în modul `MCP_NORMAL`. Datele au fost citite și afișate fără latențe vizibile. S-a observat sensibilitatea ecranelor LCD la fluctuații de tensiune, problema afișării neclare fiind corectată prin calibrarea precisă (la zecimală) a sursei Step-Down cu potențiometru la pragul de 5.0V sub sarcină. Shift-light-ul răspunde instantaneu la turațiile motorului.

===== Concluzii =====

Proiectul a dovedit cu succes că accesarea rețelelor CAN din vehicule este posibilă folosind echipamente hardware low-cost, atât timp cât protocolul și zgomotul de pe magistrală sunt corect filtrate software prin buffere și întreruperi. Am învățat importanța stabilizării tensiunii în domeniul automotive (unde pot apărea spike-uri de la alternator) și necesitatea citirii codului non-blocant pentru sisteme de operare de tip "real-time".

===== Download =====

*   Cod Sursă (main.cpp)
*   platformio.ini
*   (Aici uploadezi arhiva ta zip pe Wiki).

===== Jurnal =====
*   *Etapa 1:* Configurare mediu PlatformIO și depanare port USB Mac (`upload_protocol = custom` necesar pentru Xplained Mini).
*   *Etapa 2:* Interconectare I2C (Ecran LCD) și depanare scanner de adrese (identificare adresă 0x27).
*   *Etapa 3:* Interconectare SPI, setare module și testare cod de bază OBD-II cu LED-uri pe GPIO.
*   *Etapa 4:* Testarea pe vehicul real. Identificare eroare de blocare a programului datorită întreruperilor suprapuse (rezolvată prin citire buffer continuă).
*   *Etapa 5:* Integrare sursă alimentare standalone (MP1584EN) de la 12V la 5V, rezolvare probleme de contrast pe ecran.

===== Bibliografie/Resurse =====

**Resurse Software:**
*   PlatformIO Documentation: [https://docs.platformio.org/](https://docs.platformio.org/)
*   Librăria `mcp_can` (Cory J Fowler): [https://github.com/coryjfowler/MCP_CAN_lib](https://github.com/coryjfowler/MCP_CAN_lib)
*   Wikipedia: OBD-II PIDs (Modul 01 PID 0C)

**Resurse Hardware:**
*   Datasheet ATmega328P (Microchip).
*   Datasheet MCP2515 Stand-Alone CAN Controller cu interfață SPI.
*   Datasheet MP1584EN Step-Down Converter.
