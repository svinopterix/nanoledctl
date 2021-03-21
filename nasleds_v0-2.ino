// LED control for NAS
// LED modes: 0 - OFF; 1 - blink 1 time for 0,5 sec; 2 - blink every 1 sec; 3 - ON.
// Serial commands:
// "ABC\n" - A - led 1 mode, B - led 2 mode, C - led3 mode
// "AB\n" - A - led number 0..2, B - led mode 0..3
// Return codes - OK, ER

// Pin numbers for leds
int ledpins[3] = {5, 6, 7};

// Modes of blinking for leds
volatile int ledmodes[3] = {0, 0, 0};

volatile int parseError = 0;

int maxMode = 3; // see LED modes comment

// Blinking patterns (20 phases, 2 phases per second; Pattern bin X shows LED status on phase X)
volatile long ledpatterns[3] = {0, 0, 0};
volatile long patterntemplates[4] = {0, 1048575, 838860, 1048575}; //3 - bin 11001100110011001100; 4 - bin 11111111111111111111

// Phase of current interrupt
volatile int phase = 0;
int maxPhase = 20;


// Variables for Serial exchange
String inputString = "";         // a String to hold incoming data
bool stringComplete = false;  // whether the string is complete

void setup() {
  // initialize Leds and light all LEDs
  for (byte i = 0; i < 3; i = i + 1) {
    pinMode(ledpins[i], OUTPUT);
    digitalWrite(ledpins[i], HIGH);
  }

  // switch off all leds after 0,5 sec
  delay(500);
  for (byte i = 0; i < 3; i = i + 1) {
    digitalWrite(ledpins[i], LOW);
  }

  // initialize serial:
  Serial.begin(9600);
  // reserve 200 bytes for the inputString:
  inputString.reserve(200);

  // инициализация Timer1
  cli(); // отключить глобальные прерывания
  TCCR1A = 0; // установить регистры в 0
  TCCR1B = 0; 

  OCR1A = 7812; // установка регистра совпадения на 0,5 с.
  TCCR1B |= (1 << WGM12); // включение в CTC режим

  // Установка битов CS10 и CS12 на коэффициент деления 1024
  TCCR1B |= (1 << CS10);
  TCCR1B |= (1 << CS12);

  TIMSK1 |= (1 << OCIE1A);  // включение прерываний по совпадению
  sei(); // включить глобальные прерывания
}

void parseinput(String s) {
  byte lednum = 0;
  byte ledmode = 0;
  
  if (inputString.length() == 4) {
    for (byte i = 0; i < 3; i = i + 1) {
      ledmodes[i] = s[i] - '0';
    
      if (ledmodes[i] > maxMode) {
        parseError = 1;
      }
    }
  } else {
    lednum = s[0]-'0';
    ledmode = s[1]-'0';
    ledmodes[lednum] = ledmode;
  }
}

void setLedPattern(int pin, int mode) {
  ledpatterns[pin] = patterntemplates[mode];
}

void loop() {
  // print the string when a newline arrives:
  if (stringComplete) {
    if (inputString.length() == 4 || inputString.length() == 3) {
      parseinput(inputString);
    } 
    else {
      parseError = 1;
    }

    if (parseError == 1) {
      Serial.println("ER\n");
    }
    else {
      for (byte i = 0; i < 3; i = i + 1) {
        setLedPattern(i, ledmodes[i]);
      }
      Serial.println("OK\n");
    }
    
    inputString = "";
    stringComplete = false;
    parseError = 0;
  }

}

/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.
*/
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    //Serial.print(inChar);
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == 13) {
      stringComplete = true;
    }
  }
}

int getLedStatus(int pin, int phase) {
  long pattern = ledpatterns[pin];
  int b = bitRead(pattern, phase);

  // !!!!! Написать однократное мигание при mode = 1

  switch (b) {
    case 0: 
      return LOW;
      break;
    case 1:
      return HIGH;
      break;
  }
}

// Прерывание от таймера. Вызывается один раз в 0,5 секунды.
ISR(TIMER1_COMPA_vect)
{   
  for (byte i = 0; i < 3; i = i + 1) {
    digitalWrite(ledpins[i], getLedStatus(i, phase));
  }

  for (byte i = 0; i < 3; i = i + 1) {
    if (ledmodes[i] == 1) {
      ledmodes[i] = 0;
      ledpatterns[i] = 0;
    }
  }

  phase++;

  if (phase >= maxPhase) {
    phase = 0;
  }
}
