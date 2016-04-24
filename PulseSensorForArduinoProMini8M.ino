/*
 *  Apr. 23rd 2016 by Yoshino Taro
 *  http://makers-with-myson.blog.so-net.ne.jp/
 *  This code is based on the sample code of
 *  https://github.com/WorldFamousElectronics/PulseSensor_Amped_Arduino/blob/master/README.md
 */

#define N 10

volatile int BPM;
volatile int Signal;
volatile int IBI = 600;
volatile boolean Pulse = false;
volatile boolean QS = false;

volatile int Rate[N];
volatile unsigned long Counter = 0;
volatile unsigned long LastBeatTime = 0;
volatile int P = 512;
volatile int T = 512;
volatile int Threshold = 525;
volatile int Amplifier = 100;

int PulseSensorPin = 0;
int FadePin = 5;
int FadeRate = 0;

void setup() {
  pinMode(FadePin, OUTPUT);
  Serial.begin(115200); 
  noInterrupts();
  TCCR2A = 0;
  TCCR2B = 0;
  TCNT2  = 0;
  TCCR2A = bit(WGM21);
  TCCR2B = bit(CS22) | bit(CS21) | bit(CS20); // 8M/1024
  OCR2A = 78; // 10ms
  // TCCR2B = bit(CS22) | bit(CS20); //8M/128
  // OCR2A = 124; // 2ms
  TIMSK2 = bit(OCIE2A);
  interrupts();
}

ISR (TIMER2_COMPA_vect) {
  noInterrupts();
  Signal = analogRead(PulseSensorPin);
  
  Counter += 10; // 10msec
  // Counter += 2; // 2msec
  
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
      IBI = Counter - LastBeatTime;
      LastBeatTime = Counter;
      
      if (Rate[0] < 0) {
        Rate[0] = 0;
        interrupts();
        return;
      } else if (Rate[0] == 0) {
        for (int i = 0; i < N; ++i) {
          Rate[i] = IBI;
        }
      }
            
      word running_total = 0;     
      for (int i = 0; i < N-1; ++i) {
        Rate[i] = Rate[i+1];
        running_total += Rate[i];
      }

      Rate[N-1] = IBI;
      running_total += IBI;
      running_total /= N;
      BPM = 60000 / running_total;
      QS = true;
    }
  }
  
  // check if Signal is under Threshold
  if (Signal < Threshold && Pulse) {
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
    for (int i = 0; i < N; ++i) {
      Rate[i] = -1;
    }
  }
 
  interrupts();
}

void loop() {
  
  if (QS) {
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
