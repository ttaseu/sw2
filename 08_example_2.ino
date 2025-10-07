// --- pins ---
#define PIN_LED  9
#define PIN_TRIG 12
#define PIN_ECHO 13

// --- params ---
#define SND_VEL 346.0
#define INTERVAL 25          // ms
#define PULSE_DURATION 10    // usec
#define _DIST_MIN 100.0      // mm
#define _DIST_MAX 300.0      // mm

#define TIMEOUT ((INTERVAL / 2) * 1000.0)     // usec
#define SCALE (0.001 * 0.5 * SND_VEL)         // -> mm

// brightness profile (triangle: brightest at 200 mm)
#define CENTER_MM 200.0
#define HALF_SPAN 100.0                       // 200±100

unsigned long last_sampling_time = 0;
unsigned long last_header_time   = 0;

float USS_measure(int TRIG, int ECHO) {
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(PULSE_DURATION);
  digitalWrite(TRIG, LOW);
  return pulseIn(ECHO, HIGH, TIMEOUT) * SCALE; // mm
}

// map distance(mm) -> PWM duty(0~255), active-low LED
int distanceToDuty(float d) {
  if (d <= 0.0) return 255;                              // bad read -> off
  if (d <= _DIST_MIN || d >= _DIST_MAX) return 255;      // out of range -> off
  float duty_f = 255.0f * (fabs(d - CENTER_MM) / HALF_SPAN);
  if (duty_f < 0) duty_f = 0;
  if (duty_f > 255) duty_f = 255;
  return (int)(duty_f + 0.5f);
}

void setup() {
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  Serial.begin(57600);
  delay(300);
  Serial.println("Min,distance,Max");  // Serial Plotter legend (ASCII only)
}

void loop() {
  if (millis() - last_sampling_time < INTERVAL) return;
  last_sampling_time = millis();

  float distance = USS_measure(PIN_TRIG, PIN_ECHO);  // mm
  int duty = distanceToDuty(distance);
  analogWrite(PIN_LED, duty);                        // brightness control

  // --- Serial Plotter: show ONLY Min, distance, Max ---
  Serial.print(_DIST_MIN);      // Min
  Serial.print(",");
  Serial.print(distance, 1);    // distance (mm)
  Serial.print(",");
  Serial.println(_DIST_MAX);    // Max

  // resend legend every 2 seconds (in case Plotter opens later)
  if (millis() - last_header_time > 2000) {
    Serial.println("Min,distance,Max");
    last_header_time = millis();
  }
}
