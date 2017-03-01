//#define BLEport Serial1

HardwareSerial *BLEport = &Serial1;

void setup() {
  // put your setup code here, to run once:
  BLEport->begin(9600);

}

void loop() {
  // put your main code here, to run repeatedly:
  byte b = 5;
  BLEport->write(b);
}
