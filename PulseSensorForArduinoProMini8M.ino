/*
 *  Apr. 21st 2016 by Yoshino Taro
 *  http://makers-with-myson.blog.so-net.ne.jp/
 *  This code is based on the sample code of
 *  https://github.com/WorldFamousElectronics/PulseSensor_Amped_Arduino/blob/master/README.md
 */

volatile int BPM;
volatile int Signal;
volatile int IBI = 600;
volatile boolean Pulse = false;
volatile boolean QS = false;

volatile int Rate[10];
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
boolean LongSampling = true;

void setup() {
  pinMode(BlinkPin, OUTPUT);
  pinMode(FadePin, OUTPUT);
  Serial.begin(115200);
  
  noInterrupts();
  TCCR2A = 0;
  TCCR2B = 0;
  TCNT2  = 0;
  
  TCCR2A = bit(WGM21);
  if (LongSampling) {
    TCCR2B = bit(CS22) | bit(CS21) | bit(CS20); // 8M/1024
    OCR2A = 78; // 10ms
  } else {
    TCCR2B = bit(CS22) | bit(CS20); //8M/128
    OCR2A = 124; // 2ms
  }
  TIMSK2 = bit(OCIE2A);
  interrupts();
}

ISR (TIMER2_COMPA_vect) {
  noInterrupts();
  Signal = analogRead(PulseSensorPin);
  
  if (LongSampling) {
    Counter += 10; // 10msec
  } else {
    Counter += 2; // 2msec
  }
  
  int interval = Counter - LastBeatTime;
  
  // hold bottom
  if (Signal < Threshold && interval > (IBI/5)*3) {
    if (Signal < T) T = Signal;
  }
  
  // hold peak
  if (Signal > Threshold && Signal > P) {
    P = Signal;
  }
  
  if (interval > 250 /* ms */) {
    
    // check if Signal is over Threshold
    if ((Signal > Threshold) && !Pulse && (interval > (IBI/5)*3)) {
      Pulse = true;
      digitalWrite(BlinkPin, HIGH);
      IBI = Counter - LastBeatTime;
      LastBeatTime = Counter;
      
      if (SecondBeat) {
        SecondBeat = false;
        for (int i = 0; i <= 9; ++i) {
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
  }
  
  // check if Signal is under Threshold
  if (Signal < Threshold && Pulse) {
    digitalWrite(BlinkPin, LOW);
    Pulse = false;
    Amplifier = P - T;
    Threshold = Amplifier / 2 + T; // revise Threshold
    P = Threshold;
    T = Threshold;
  }
  
  // check if no Signal is over 2.5 sec
  if (interval > 2500 /* ms */) {
    Threshold = 512;
    P = 512;
    T = 512;
    LastBeatTime = Counter;
    FirstBeat = true;
    SecondBeat = false;
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
  delay(20);  
}

