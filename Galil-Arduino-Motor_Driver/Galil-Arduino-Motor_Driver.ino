/*void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
*/
int value1 = 0;
int value2 = 0;
int inPin1 = 13;
int inPin2 = 12;
int pwmPin1 = 6;
int switchpin1_1 = 4;
int switchpin2_1 = 5;
int pwmPin2 = 10;
int switchpin1_2 = 8;
int switchpin2_2 = 9;

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(inPin1, INPUT);
  pinMode(inPin2, INPUT);
  pinMode(pwmPin1, OUTPUT);
  pinMode(switchpin1_1, OUTPUT);
  pinMode(switchpin2_1, OUTPUT);
  pinMode(pwmPin2, OUTPUT);
  pinMode(switchpin1_2, OUTPUT);
  pinMode(switchpin2_2, OUTPUT);
  digitalWrite(switchpin1_1, HIGH);
  digitalWrite(switchpin2_1, LOW);
  digitalWrite(switchpin1_2, HIGH);
  digitalWrite(switchpin2_2, LOW);
}

// the loop function runs over and over again forever
void loop() {
  
  value1 = digitalRead(inPin1);   // read the input pin
  value2 = digitalRead(inPin2);
  digitalWrite(LED_BUILTIN, value1);
  //digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  //delay(1000);                       // wait for a second
  //digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  //delay(1000);                       // wait for a second
 
 if(value1 == HIGH) {
    analogWrite(pwmPin1, 255);
    //digitalWrite(LED_BUILTIN, HIGH);
    //delay(1000); 
 }
 else {
    analogWrite(pwmPin1, 0);
    //digitalWrite(LED_BUILTIN, LOW);
    //delay(1000); 
 }

 if(value2 == HIGH) {
    analogWrite(pwmPin2, 255);
    //digitalWrite(LED_BUILTIN, HIGH);
    //delay(1000); 
 }
 else {
    analogWrite(pwmPin2, 0);
    //digitalWrite(LED_BUILTIN, LOW);
    //delay(1000); 
 }

 //analogWrite(pwmPin, 255);
 //digitalWrite(LED_BUILTIN, LOW); 
 //delay(1000);
 
  
}
