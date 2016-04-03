#define LEDMAX 255

int ledBluePin = 22;
int ledRedPin = 21;
int ledGreenPin = 20;

int currColor = 0;
int currIntensity = 0;
int prevIntensity = LEDMAX;

void setup() {
  // put your setup code here, to run once:
  pinMode(ledBluePin, OUTPUT);
  pinMode(ledRedPin, OUTPUT);
  pinMode(ledGreenPin, OUTPUT);
  analogWrite(ledBluePin, 0);
  analogWrite(ledRedPin, LEDMAX);
  analogWrite(ledGreenPin, LEDMAX);
}

void loop() {
  if (currIntensity == LEDMAX){
    prevIntensity --;
  } else{
    currIntensity ++;
  }

  if (currColor == 0){
    analogWrite(ledRedPin, LEDMAX - currIntensity);
    analogWrite(ledBluePin, LEDMAX - prevIntensity);
    if (prevIntensity == 0){
      currColor = 1;
      prevIntensity = LEDMAX;
      currIntensity = 0;
    }
  } else if (currColor == 1){
    analogWrite(ledGreenPin, LEDMAX - currIntensity);
    analogWrite(ledRedPin, LEDMAX - prevIntensity);
    if (prevIntensity == 0){
      currColor = 2;
      prevIntensity = LEDMAX;
      currIntensity = 0;
    }
  } else if (currColor == 2){
    analogWrite(ledBluePin, LEDMAX - currIntensity);
    analogWrite(ledGreenPin, LEDMAX - prevIntensity);
    if (prevIntensity == 0){
      currColor = 0;
      prevIntensity = LEDMAX;
      currIntensity = 0;
    }
  }
  delay(10);
}
