#include <Servo.h>

/* ---------------- Pin assignment ---------------- */
#define PIN_LED    9    // LED active-low (LOW:on, HIGH:off)
#define PIN_TRIG   12   // sonar TRIGGER
#define PIN_ECHO   13   // sonar ECHO
#define PIN_SERVO  10   // servo PWM

/* ---------------- Sonar config ------------------- */
#define SND_VEL         346.0    // m/s @ 24°C
#define INTERVAL        25       // sampling interval [ms]
#define PULSE_DURATION  10       // trigger pulse [us]

// 과제 범위: 18~36 cm  → mm로 환산
#define _DIST_MIN       180.0    // [mm] 측정 하한 (≤ → 0°)
#define _DIST_MAX       360.0    // [mm] 측정 상한 (≥ → 180°)

#define TIMEOUT  ((INTERVAL / 2) * 1000.0)  // [us]
#define SCALE    (0.001 * 0.5 * SND_VEL)    // duration->distance(mm)

/* ---------------- Filtering ---------------------- */
#define _EMA_ALPHA  0.3    // 0~1 (1.0이면 EMA 미적용)

/* ---------------- Servo pulses ------------------- */
/* 각 서보마다 조정 필요 */
#define _DUTY_MIN   500    // ≈ 0°
#define _DUTY_NEU   1500   // ≈ 90°
#define _DUTY_MAX   2500   // ≈ 180°

/* ---------------- Globals ------------------------ */
float dist_ema, dist_prev = _DIST_MAX;   // [mm]
unsigned long last_sampling_time;        // [ms]
Servo myservo;

/* -------------- Utility: clamp ------------------- */
static inline float clampf(float x, float lo, float hi) {
  if (x < lo) return lo;
  if (x > hi) return hi;
  return x;
}

/* -------------- Utility: map float --------------- */
// 선형 매핑: x∈[in_min,in_max] → [out_min,out_max]
static inline float fmap(float x, float in_min, float in_max,
                         float out_min, float out_max) {
  float t = (x - in_min) / (in_max - in_min);
  return out_min + t * (out_max - out_min);
}

/* ------------------------------------------------- */
void setup() {
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  myservo.attach(PIN_SERVO);
  myservo.writeMicroseconds(_DUTY_NEU);  // 시작은 90°

  dist_prev = _DIST_MIN;
  dist_ema  = dist_prev;                 // EMA 초기화
  last_sampling_time = millis();

  Serial.begin(57600);
}

/* ------------------------------------------------- */
void loop() {
  // 주기 동기화
  if (millis() < last_sampling_time + INTERVAL) return;

  /* 1) 초음파 원시값 */
  float dist_raw = USS_measure(PIN_TRIG, PIN_ECHO);

  /* 2) 범위 필터 (윈도 밖이면 이전값 유지) */
  float dist_filtered;
  if ((dist_raw == 0.0) || (dist_raw > _DIST_MAX) || (dist_raw < _DIST_MIN)) {
    dist_filtered = dist_prev;
  } else {
    dist_filtered = dist_raw;
    dist_prev = dist_raw;
  }

  /* 3) EMA 필터 */
  dist_ema = _EMA_ALPHA * dist_filtered + (1.0 - _EMA_ALPHA) * dist_ema;

  /* 4) 서보 각도 계산 (18~36cm → 0~180° 선형) */
  float d_clamped = clampf(dist_ema, _DIST_MIN, _DIST_MAX);  // 안전 클램프
  float servo_deg;

  if (d_clamped <= _DIST_MIN) {
    servo_deg = 0.0;
  } else if (d_clamped >= _DIST_MAX) {
    servo_deg = 180.0;
  } else {
    // 예시 확인: 190mm≈19cm→≈10°, 270mm→≈90°, 350mm→≈170°
    servo_deg = fmap(d_clamped, _DIST_MIN, _DIST_MAX, 0.0, 180.0);
  }

  // 각도→펄스폭(미세 보정 가능)
  int duty = (int)fmap(servo_deg, 0.0, 180.0, _DUTY_MIN, _DUTY_MAX);
  myservo.writeMicroseconds(duty);

  /* 5) LED: 범위안(18~36cm)일 때 점등 */
  if ((dist_ema >= _DIST_MIN) && (dist_ema <= _DIST_MAX)) {
    digitalWrite(PIN_LED, LOW);   // active-low → ON
  } else {
    digitalWrite(PIN_LED, HIGH);  // OFF
  }

  /* 6) 시리얼 출력(플로터용 포맷) */
  Serial.print("Min:");   Serial.print(_DIST_MIN);
  Serial.print(",dist:"); Serial.print(dist_raw);
  Serial.print(",ema:");  Serial.print(dist_ema);
  Serial.print(",Servo:");Serial.print(servo_deg);   // 각도 표시
  Serial.print(",Max:");  Serial.print(_DIST_MAX);
  Serial.println("");

  // 다음 주기
  last_sampling_time += INTERVAL;
}

/* -------- USS: 거리 측정(mm) -------- */
float USS_measure(int TRIG, int ECHO) {
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(PULSE_DURATION);
  digitalWrite(TRIG, LOW);
  unsigned long dur = pulseIn(ECHO, HIGH, (unsigned long)TIMEOUT); // [us]
  return dur * SCALE;  // → [mm]
}
