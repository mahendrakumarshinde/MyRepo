void setup() {
  analogReference(EXTERNAL);
  analogReadResolution(12);
  analogReadAveraging(32);
  Serial2.begin(9600);
}

void loop() {
  uint32_t bat;
  bat = getInputVoltage();
  Serial2.println(bat);
  delay(1000);
}

uint32_t getInputVoltage(){
  uint32_t x = analogRead(39);
  return (178*x*x + 2688743864 - 1182047 * x) / 371794;
}
