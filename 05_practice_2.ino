#define LED_PIN 7          
#define LED_ON  HIGH       
#define LED_OFF LOW        

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LED_OFF); 
}

void loop() {
  digitalWrite(LED_PIN, LED_ON);
  delay(1000);

  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_PIN, LED_ON);
    delay(100);  // 0.1초 ON
    digitalWrite(LED_PIN, LED_OFF);
    delay(100);  // 0.1초 OFF
  }

  digitalWrite(LED_PIN, LED_OFF);
  while (1) {
  }
}
