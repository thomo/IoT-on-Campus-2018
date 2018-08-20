/*
 * Used libraries
 * 
 * to install: Sketch -> Include Library -> Manage Libraries -> Search 
 */

#define SW1_PIN D6
#define SW2_PIN D7

void setup() {
  Serial.begin(9600);

  while (!Serial) {}

  pinMode(LED_BUILTIN, OUTPUT); // onboard LED
  
  pinMode(SW1_PIN, INPUT);
  pinMode(SW2_PIN, INPUT);
}

void loop() {
  Serial.print("S1: ");
  Serial.println(digitalRead(SW1_PIN));
  Serial.print(" S2: ");
  Serial.println(digitalRead(SW2_PIN));
  Serial.println(" ");

  if (digitalRead(SW1_PIN) || digitalRead(SW2_PIN)) {
    // enable LED
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    // disable LED
    digitalWrite(LED_BUILTIN, LOW);
  }
}

