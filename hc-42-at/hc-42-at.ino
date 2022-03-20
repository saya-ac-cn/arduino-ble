#include <SoftwareSerial.h> 

/***
 * HC-42模块AT指令调试
 */

// Pin10为RX，接HC05的TXD
// Pin11为TX，接HC05的RXD
SoftwareSerial HC(10, 11); 
char val;

void setup() {
  Serial.begin(9600); 
  Serial.println("HC is ready!");
  // HC-42默认，9600
  HC.begin(9600);
}

void loop() {
  if (Serial.available()) {
    val = Serial.read();
    HC.print(val);
  }

  if (HC.available()) {
    val = HC.read();
    Serial.print(val);
  }
}
