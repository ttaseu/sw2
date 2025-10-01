int pwmPin = 7;
int period = 10000; //10ms = 10000, 1ms = 1000, 0.1ms = 100
int duty = 0;

void setup() {
  pinMode(pwmPin, OUTPUT);
}

void set_period(int p) {
  if (p >= 100 && p <= 10000) {
    period = p;
  }
}

void set_duty(int d) {
  if (d >= 0 && d <= 100) {
    duty = d;
  }
}

void pwm_cycle() {
  int highTime = period * duty / 100;
  int lowTime  = period - highTime;

  if (highTime > 0) {
    digitalWrite(pwmPin, HIGH);
    delayMicroseconds(highTime);
  }
  if (lowTime > 0) {
    digitalWrite(pwmPin, LOW);
    delayMicroseconds(lowTime);
  }
}

void loop() {
  for (int d = 0; d <= 100; d++) {
    set_duty(d);
    for (int i = 0; i < 5; i++)
      pwm_cycle();
  }

  for (int d = 100; d >= 0; d--) {
    set_duty(d);
    for (int i = 0; i < 5; i++)
      pwm_cycle();
  }
}
