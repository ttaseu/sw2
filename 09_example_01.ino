#define PIN_LED   9
#define PIN_TRIG 12
#define PIN_ECHO 13

// -----------------------------
// Configurable parameters
// -----------------------------
#define SND_VEL 346.0       // sound velocity at 24 celsius degree (m/sec)
#define INTERVAL 25         // sampling interval (msec)
#define PULSE_DURATION 10   // trigger pulse duration (usec)
#define _DIST_MIN 100.0     // for plotting scale only (mm)  
#define _DIST_MAX 300.0

#define TIMEOUT ((INTERVAL / 2) * 1000.0)  // maximum echo waiting time (usec)
#define SCALE   (0.001 * 0.5 * SND_VEL)    // coefficent to convert duration to distance (mm)

// -----------------------------
// Median filter window size
// -----------------------------
#ifndef MEDIAN_N
#define MEDIAN_N 30      // 기본 N 
#endif

// -----------------------------
// Globals
// -----------------------------
float dist_raw = _DIST_MAX;          
float dist_median = _DIST_MAX;       

// 순환 버퍼
float buf[MEDIAN_N];
int   buf_count = 0;   
int   buf_head  = 0;   

unsigned long last_sampling_time = 0;

// -----------------------------
// Helper: ultrasonic distance read (mm)
// -----------------------------
float readDistanceMM() {
  // Create a 10us TRIG pulse
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(PULSE_DURATION);
  digitalWrite(PIN_TRIG, LOW);

  // Measure ECHO high-time with timeout (usec)
  unsigned long dur = pulseIn(PIN_ECHO, HIGH, (unsigned long)TIMEOUT);

  // Convert duration to distance (mm)
  float d = (float)dur * SCALE; 
  return d;  
}


float computeMedian() {
  int n = buf_count;
  if (n <= 0) return dist_raw;
  // 복사
  float tmp[MEDIAN_N];
  for (int i = 0; i < n; ++i) {
    
    int idx = (buf_head - i - 1);
    if (idx < 0) idx += MEDIAN_N;
    tmp[i] = buf[idx];
  }

  for (int i = 1; i < n; ++i) {
    float key = tmp[i];
    int j = i - 1;
    while (j >= 0 && tmp[j] > key) {
      tmp[j + 1] = tmp[j];
      --j;
    }
    tmp[j + 1] = key;
  }

  if (n % 2 == 1) {
    return tmp[n / 2];
  } else {
    return 0.5f * (tmp[n/2 - 1] + tmp[n/2]);
  }
}

void setup() {
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  Serial.begin(115200);


  dist_raw = readDistanceMM();
  buf[0] = dist_raw;
  buf_count = 1;
  buf_head = 1 % MEDIAN_N;
  dist_median = computeMedian();
}

void loop() {

  unsigned long now = millis();
  if (now - last_sampling_time < INTERVAL) return;
  last_sampling_time = now;

  dist_raw = readDistanceMM();


  buf[buf_head] = dist_raw;
  buf_head = (buf_head + 1) % MEDIAN_N;
  if (buf_count < MEDIAN_N) buf_count++;

  // 3) 중위수 계산
  dist_median = computeMedian();

  // 4) 시리얼 플로터 출력 (RAW + MEDIAN만)
  Serial.print("Min:");      Serial.print(_DIST_MIN);
  Serial.print(",raw:");     Serial.print(dist_raw);
  Serial.print(",median:");  Serial.print(dist_median);
  Serial.print(",Max:");     Serial.print(_DIST_MAX);
  Serial.println("");

  // 5) LED 시각화: 중위수 기준 (가까울수록 밝게)
  float d = dist_median;
  float t = (d - _DIST_MIN) / (_DIST_MAX - _DIST_MIN); // 0..1 예상
  if (t < 0) t = 0; if (t > 1) t = 1;
  int pwm = (int)((1.0f - t) * 255.0f);
  if (pwm < 0) pwm = 0; if (pwm > 255) pwm = 255;
  analogWrite(PIN_LED, pwm);
}
