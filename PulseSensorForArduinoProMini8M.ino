volatile int Rate[10];
volatile int BPM;
volatile int Signal;
volatile int IBI = 600;
volatile boolean Pulse = false;
volatile boolean QS = false;
volatile unsigned long Counter = 0;
volatile unsigned long LastBeatTime = 0;
volatile boolean FirstBeat = true;
volatile boolean SecondBeat = false;
volatile int P = 512;
volatile int T = 512;
volatile int Threshold = 525;
volatile int Amplifier = 100;

int PulseSensorPin = 0;
int BlinkPin = 13;
int FadePin = 5;
int FadeRate = 0;

void setup() {
  pinMode(BlinkPin, OUTPUT);
  pinMode(FadePin, OUTPUT);
  Serial.begin(115200);
  
  noInterrupts();
  TCCR2A = 0;
  TCCR2B = 0;
  TCNT2  = 0;
  
  TCCR2A = bit(WGM21);
  TCCR2B = bit(CS22) | bit(CS20);
  OCR2A = 124;
  TIMSK2 = bit(OCIE2A);
  interrupts();
}

ISR (TIMER2_COMPA_vect) {
  noInterrupts();
  Signal = analogRead(PulseSensorPin);
  Counter += 2;
  int interval = Counter - LastBeatTime;
  if (Signal < Threshold && interval > (IBI/5)*3) {
    if (Signal < T) T = Signal;
  }
  
  if (Signal > Threshold && Signal > P) {
    P = Signal;
  }
  
  if (interval > 250 /* ms */) {
    if ((Signal > Threshold) && !Pulse && (interval > (IBI*3)/5)) {
      Pulse = true;
      digitalWrite(BlinkPin, HIGH);
      IBI = Counter - LastBeatTime;
      LastBeatTime = Counter;
      
      if (SecondBeat) {
        SecondBeat = false;
        for (int i = 0; i < 9; ++i) {
          Rate[i] = IBI;
        }
      }
      
      if (FirstBeat) {
        FirstBeat = false;
        SecondBeat = true;
        interrupts();
        return;
      }
      
      word running_total = 0;
      
      for (int i = 0; i <= 8; ++i) {
        Rate[i] = Rate[i+1];
        running_total += Rate[i];
      }
      
      Rate[9] = IBI;
      running_total += IBI;
      running_total /= 10;
      BPM = 60000 / running_total;
      QS = true;
    }
    
    if (Signal < Threshold && Pulse) {
      digitalWrite(BlinkPin, LOW);
      Pulse = false;
      Amplifier = P - T;
      Threshold = Amplifier / 2 + T;
      P = Threshold;
      T = Threshold;
    }
    
    if (interval > 2500 /* ms */) {
      Threshold = 512;
      P = 512;
      T = 512;
      LastBeatTime = Counter;
      FirstBeat = true;
      SecondBeat = false;
    }
  }
  
  interrupts();
}

void loop() {
  
  if (QS) {
    digitalWrite(BlinkPin, HIGH);
    FadeRate = 255;
  
    Serial.print("BPM: ");
    Serial.println(BPM);
    
    QS = false;
  }
  
  FadeRate -= 15;
  FadeRate = constrain(FadeRate, 0, 255);
  analogWrite(FadePin, FadeRate);
  
}

