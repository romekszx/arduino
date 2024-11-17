void setup() {
  // Initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
}

void loop() {
  // Print "Hello, World!" to the serial monitor:
  Serial.println("Hello, World!");
  // Wait for a second:
  delay(1000);
}
