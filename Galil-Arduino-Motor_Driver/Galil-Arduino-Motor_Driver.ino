/*void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
*/
int value = 0;
int inPin = 13;
int pwmPin = 6;
int switchpin1 = 4;
int switchpin2 = 5;

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(inPin, INPUT);
  pinMode(pwmPin, OUTPUT);
  pinMode(switchpin1, OUTPUT);
  pinMode(switchpin2, OUTPUT);
  digitalWrite(switchpin1, HIGH);
  digitalWrite(switchpin2, LOW);
}

// the loop function runs over and over again forever
void loop() {
  
  value = digitalRead(inPin);   // read the input pin
  digitalWrite(LED_BUILTIN, value);
  //digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  //delay(1000);                       // wait for a second
  //digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  //delay(1000);                       // wait for a second
 
 if(value == HIGH) {
    analogWrite(pwmPin, 255);
    //digitalWrite(LED_BUILTIN, HIGH);
    //delay(1000); 
 }
 else {
    analogWrite(pwmPin, 0);
    //digitalWrite(LED_BUILTIN, LOW);
    //delay(1000); 
 }

 //analogWrite(pwmPin, 255);
 //digitalWrite(LED_BUILTIN, LOW); 
 //delay(1000);
 
  
}
