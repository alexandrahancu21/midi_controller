// pentru Arduino UNO
#define ATMEGA328 1
// definire librarii
#ifdef ATMEGA328
#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE();
#endif
unsigned long time;
const int numberLEDs = 3;
const int ledArduinoPin[numberLEDs] = { 13, 12, 11 };
// Butoane
const int numberButtons = 3;                              // numarul total de butoane de pe placuta
const int buttonArduinoPin[numberButtons] = { 2, 3, 4 };  // pinurile digitale conectate la butoane

int buttonCState[numberButtons] = {};  // retine valoarea curenta a butonului
int buttonPState[numberButtons] = {};  // retine valoarea anterioara a butonului

// debounce // uneori cand apasam butonul si se atinge de partea de jos a placutei se poate apasa de mai multe ori in cateva milisecunde iar in serial monitor chiar daca apasam butonul doar o singura data ne poate aparea ca l-am apasat de mult mai multe ori
unsigned long lastDebounceTime[numberButtons] = { 0 };  // ultima data cand pinul de iesire a fost comutat
unsigned long debounceDelay = 50;                       // timpul de debalansare; creste daca iesirea palpaie

// Potentiometre
const int numberPots = 2;                                     // numarul total de potentiometre de pe placuta
const int potentiometersArduinoPin[numberPots] = { A0, A1 };  // pinurile analogice conectate la potentiometre

int potCState[numberPots] = { 0 };  // retine valoarea curenta a potentiometrului
int potPState[numberPots] = { 0 };  // retine valoarea anterioara a potentiometrului
int potVar = 0;                     // diferenta dintre valoarea curenta si cea anterioara a potentiometrului

int midiCState[numberPots] = { 0 };  // valoarea curenta din midi
int midiPState[numberPots] = { 0 };  // valoarea anterioara din midi

const int timeout = 300;                  // perioada de timp in care potentiometrul va fi citit dupa ce depaseste varThreshold (variabila de prag)
const int varThreshold = 10;              // pragul pentru variatia semnalului potentiometrului
boolean potMoving = true;                 // daca potentiometrul se misca
unsigned long pTime[numberPots] = { 0 };  // valoarea timpului anterior
unsigned long timer[numberPots] = { 0 };  // stocheaza timpul care a trecut de la resetarea temporizatorului

// MIDI
byte midiCh = 1;  // canalul MIDI care va fi folosit in Ableton
byte note = 36;   // cea mai scazuta nota care va fi folosita
byte cc = 1;      // cel mai scazut MIDI CC (Control Change) care va fi folosit


// SETUP
void setup() {  // se executa doar o data in program

  // Baud Rate
  // 115200 pentru Hairless MIDI
  Serial.begin(115200);

#ifdef DEBUG
  Serial.println("Debug mode");
  Serial.println();  // salt la linie noua
#endif

  // Butoane
  // initializare butoane cu pull up resistor (rezistor folosit pentru a asigura o stare cunoscuta pentru un semnal)
  for (int i = 0; i < numberButtons; i++) {
    pinMode(buttonArduinoPin[i], INPUT_PULLUP);  // in loc sa conectezi un rezistor manual pui comanda PULLUP (butonul e de tip intrare)
    pinMode(ledArduinoPin[i], OUTPUT);
  }
}

// LOOP
void loop() {  // se executa de mai multe ori, de unde si numele

  buttons();
  potentiometers();
}


// Butoane
void buttons() {

  for (int i = 0; i < numberButtons; i++) {

    buttonCState[i] = digitalRead(buttonArduinoPin[i]);  // citeste pinul de pe Arduino la butoane, (vrem sa stim cum e butonul on/off sau 5V/0V)

    // de cate ori apasam pe buton trebuie sa-i retinem timpul
    if ((millis() - lastDebounceTime[i]) > debounceDelay) {

      if (buttonPState[i] != buttonCState[i]) {
        lastDebounceTime[i] = millis();

        if (buttonCState[i] == LOW) {  // daca butonul curent este apasat

          // trimite nota ON din MIDI la placuta
#ifdef ATMEGA328
          MIDI.sendNoteOn(note + i, 127, midiCh);  // nota, viteza, canal
          Serial.println("Note on");
          for (int i = 0; i < numberLEDs; i++) {
            digitalWrite(ledArduinoPin[i], HIGH);
            time = millis();
            delay(100);
          }


#elif DEBUG
          Serial.print(i);
          Serial.println(": button on");
#endif

        } else {

          // trimite nota OF din MIDI la placuta
#ifdef ATMEGA328
          MIDI.sendNoteOn(note + i, 0, midiCh);  // nota, viteza, canal
          Serial.println("Note off");
          for (int i = 0; i < numberLEDs; i++) {
            digitalWrite(ledArduinoPin[i], LOW);
          }

#elif DEBUG
          Serial.print(i);
          Serial.println(": button off");
#endif
        }
        buttonPState[i] = buttonCState[i];
      }
    }
  }
}

// Potentiometre
void potentiometers() {

  for (int i = 0; i < numberPots; i++) {

    potCState[i] = analogRead(potentiometersArduinoPin[i]);  // citeste pinul de pe arduino la potentiometre

    midiCState[i] = map(potCState[i], 0, 1023, 0, 127);  // mapeaza citirea lui potCState la o valoare utilizabila in midi

    potVar = abs(potCState[i] - potPState[i]);  // calculeaza valoarea absoluta a diferentei dintre starea curenta si cea anterioara a potentiometrului

    if (potVar > varThreshold) {  // deschide poarta daca variatia potentiometrului este mai mare decat pragul
      pTime[i] = millis();        // stocheaza timpul anterior
    }

    timer[i] = millis() - pTime[i];  // reseteaza timpul

    if (timer[i] < timeout) {  // daca temporizatorul este mai mic decat timpul maxim permis inseamna ca potentiometrul este inca in miscare
      potMoving = true;
    } else {
      potMoving = false;
    }

    if (potMoving == true) {  // daca potentiometrul este inca in miscare, trimiteti CC (Control Change)
      if (midiPState[i] != midiCState[i]) {

        // trimite MIDI CC la placuta
#ifdef ATMEGA328
        MIDI.sendControlChange(cc + i, midiCState[i], midiCh);  // numarul CC, valoarea CC, canalul MIDI

#elif DEBUG
        Serial.print("Pot: ");
        Serial.print(i);
        Serial.print(" ");
        Serial.println(midiCState[i]);
#endif
        potPState[i] = potCState[i];  // stocheaza citirea curenta a potentiometrului pentru a o compara cu urmatoarea
        midiPState[i] = midiCState[i];
      }
    }
  }
}

