/*void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
*/
int value = 0;
int inPin = 13;

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(A5, INPUT);
}

// the loop function runs over and over again forever
void loop() {
  value = digitalRead(A5);   // read the input pin
  digitalWrite(LED_BUILTIN, value);
  //digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  //digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  //delay(1000);                       // wait for a second
  
  //digitalWrite(LED_BUILTIN, LOW); 
  //delay(500);
  
}
