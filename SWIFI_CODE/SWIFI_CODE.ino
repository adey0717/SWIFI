#include <Wire.h>
#include <SoftwareSerial.h>
#include <Adafruit_PWMServoDriver.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
hd44780_I2Cexp lcd;
#include "HX711.h"
#define DOUT  3
#define SCK  4

HX711 scale(DOUT, SCK);

SoftwareSerial Arduino_Serial(A8, A9);

#define SERVO_FREQ 50 // Analog servos run at ~60 Hz updates
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

#define SERVOMIN  150 // This is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  600 // This is the 'maximum' pulse length count (out of 4096)

uint8_t Servo_Loadcell = 0;
uint8_t Servo_Entrance = 1;
uint8_t Servo_Accepted = 2;

unsigned long previousMillis = 0;        // will store last time LED was updated
const long interval = 1000;           // interval at which to blink (milliseconds)

int weight;
float calibration_factor = 203640; // for me this value works just perfect 419640

#define LASER_RECIEVER  2
#define Small_Trig 6
#define Small_Echo 5
#define Medium_Trig 8
#define Medium_Echo 7
#define Large_Trig 10
#define Large_Echo 9

#define Bin_Bottles_Trig 12
#define Bin_Bottles_Echo 11

#define READY_INSERT  14
#define SEND_POINTS  15

volatile int pulses_reward = 0;
boolean Display = false;
boolean Display_Bin = false;
boolean Connect_Portal = false;

int Small_Bottle, Medium_Bottle, Large_Bottle;
int Small_Duration, Medium_Duration, Large_Duration;

int Bottle_Bin, Bottle_Duration;

void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  Arduino_Serial.begin(9600);
  lcd.begin(20, 4);
  pwm.begin();
  pwm.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~60 Hz updates
  pinMode(SEND_POINTS, OUTPUT);
  pinMode(READY_INSERT, INPUT_PULLUP);
  pinMode(LASER_RECIEVER, INPUT_PULLUP);

  pinMode(Small_Trig, OUTPUT);
  pinMode(Small_Echo, INPUT);
  pinMode(Medium_Trig, OUTPUT);
  pinMode(Medium_Echo, INPUT);
  pinMode(Large_Trig, OUTPUT);
  pinMode(Large_Echo, INPUT);

  pinMode(Bin_Bottles_Trig, OUTPUT);
  pinMode(Bin_Bottles_Echo, INPUT);

  digitalWrite(SEND_POINTS, HIGH);
  delay(1000);

  Serial.println("HX711 calibration sketch");
  Serial.println("Remove all weight from scale");
  Serial.println("After readings begin, place known weight on scale");
  Serial.println("Press + or a to increase calibration factor");
  Serial.println("Press - or z to decrease calibration factor");
  scale.set_scale();
  scale.tare(); //Reset the scale to 0
  long zero_factor = scale.read_average(); //Get a baseline reading
  Serial.print("Zero factor: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  Serial.println(zero_factor);

  setServoAngle(Servo_Loadcell, 0);
  setServoAngle(Servo_Entrance, 0);
  setServoAngle(Servo_Accepted, 0);
  lcd.setCursor (0, 0);
  lcd.print("--------------------");
  lcd.setCursor (0, 1);
  lcd.print(" WAITING CONNECTION ");
  lcd.setCursor (0, 2);
  lcd.print("    SWIFI PORTAL    ");
  lcd.setCursor (0, 3);
  lcd.print("--------------------");
  delay(5000);
}

// Function to set the angle of the servo
void setServoAngle(uint8_t n, uint8_t angle) {
  uint16_t pulse = map(angle, 0, 180, SERVOMIN, SERVOMAX);
  pwm.setPWM(n, 0, pulse);
}

void LOADCELL_VERIFICATION()
{
  scale.set_scale(calibration_factor); //Adjust to this calibration factor
  Serial.print("Reading: ");
  weight = scale.get_units(5);
  Serial.print("Kilogram:");
  Serial.print( weight);
  Serial.print(" Kg");
  Serial.print(" calibration_factor: ");
  Serial.print(calibration_factor);
  Serial.println();

  if ((weight <= 0) && (Small_Bottle < 7))
  {
    lcd.setCursor (0, 3);
    lcd.print("                    ");
    VERIFICATION_PROCESS();
  }

  else if ((weight >= 1) && (Small_Bottle < 7))
  {
    setServoAngle(Servo_Entrance, 90);
    lcd.setCursor(0, 3);
    lcd.print("Heavy Item Detected!");
    Serial.println("Remove the Item");
    delay(3000);
    lcd.setCursor (0, 3);
    lcd.print("Please Remove Item! ");
    delay(1000);
    lcd.setCursor (0, 3);
    lcd.print("                    ");
  }
}

void ULTRASONIC_READINGS()
{
  // ----------------------------- SMALL ULTRASONIC ---------------------------------//

  digitalWrite(Small_Trig, HIGH);
  delayMicroseconds(1000);
  digitalWrite(Small_Trig, LOW);
  Small_Duration = pulseIn(Small_Echo, HIGH);
  Small_Bottle = (Small_Duration / 2) / 29.1;
  Serial.print("ULTRASONIC_SMALL:");
  Serial.println(Small_Bottle);
  delay(20);

  // ----------------------------- MEDIUM ULTRASONIC ---------------------------------//

  digitalWrite(Medium_Trig, HIGH);
  delayMicroseconds(1000);
  digitalWrite(Medium_Trig, LOW);
  Medium_Duration = pulseIn(Medium_Echo, HIGH);
  Medium_Bottle = (Medium_Duration / 2) / 29.1;
  Serial.print("ULTRASONIC_MEDIUM:");
  Serial.println(Medium_Bottle);
  delay(20);

  // ----------------------------- LARGE ULTRASONIC ---------------------------------//

  digitalWrite(Large_Trig, HIGH); // eto yung para sa entrance
  delayMicroseconds(1000);
  digitalWrite(Large_Trig, LOW); // eto yung para sa entrance
  Large_Duration = pulseIn(Large_Echo, HIGH);
  Large_Bottle = (Large_Duration / 2) / 29.1;
  Serial.print("ULTRASONIC_LARGE:");
  Serial.println(Large_Bottle);
  delay(20);
}

void PROCESS_DETECTION()
{
  DEFAULT_LCD_DISPLAY();
  ULTRASONIC_ITEM_DETECTION();
}

void ULTRASONIC_ITEM_DETECTION()
{

  digitalWrite(Small_Trig, HIGH);
  delayMicroseconds(1000);
  digitalWrite(Small_Trig, LOW);
  Small_Duration = pulseIn(Small_Echo, HIGH);
  Small_Bottle = (Small_Duration / 2) / 29.1;
  Serial.print("ULTRASONIC_SMALL:");
  Serial.println(Small_Bottle);
  delay(20);

  if (Small_Bottle <= 7 )
  {
    setServoAngle(Servo_Entrance, 0);
    lcd.setCursor (0, 3);
    lcd.print("Please Wait.......  ");
    delay(1500);
    lcd.setCursor (0, 3);
    lcd.print("                    ");
    LOADCELL_VERIFICATION();
  }

}

void DEFAULT_LCD_DISPLAY()
{
  if ((Display == false))
  {
    lcd.setCursor (0, 0);
    lcd.print("  WELCOME TO SWIFI  ");
    lcd.setCursor (0, 1);
    lcd.print("INSERT RECYCLABLE TO");
    lcd.setCursor (0, 2);
    lcd.print("     EARN POINTS    ");
    lcd.setCursor (0, 3);
    lcd.print("                    ");
  }

  if (Display == true)
  {
    lcd.setCursor (0, 0);
    lcd.print("  WELCOME TO SWIFI  ");
    lcd.setCursor (0, 1);
    lcd.print("INSERT RECYCLABLE TO");
    lcd.setCursor (0, 2);
    lcd.print("     EARN POINTS    ");

    lcd.setCursor (0, 3);
    lcd.print("Toal Points:");
    lcd.print(pulses_reward);

  }
}

void VERIFICATION_PROCESS()
{

  int VAL_LASER;
  VAL_LASER = digitalRead(LASER_RECIEVER);
  ULTRASONIC_READINGS();


  //------------------------------------------ LARGE DETECTION ------------------------------------------ //
  if ((VAL_LASER == 1) && (Small_Bottle <= 10) && (Medium_Bottle <= 10) && (Large_Bottle <= 10))
  {

    lcd.setCursor (0, 3);
    lcd.print("Large Plastic Bottle");

    for (int i = 1; i <= 3; i++)
    {
      digitalWrite(SEND_POINTS, LOW);
      delay(200);
      digitalWrite(SEND_POINTS, HIGH);
      delay(200);
    }

    setServoAngle(Servo_Accepted, 90);
    delay(1500);
    setServoAngle(Servo_Loadcell, 90);
    delay(1500);
    setServoAngle(Servo_Loadcell, 0);
    delay(1500);
    setServoAngle(Servo_Accepted, 0);
    delay(500);

    pulses_reward += 3;
    weight = 0;

    lcd.setCursor (0, 3);
    lcd.print("                    ");
    Display = true;
  }

  //------------------------------------------ MEDIUM DETECTION ------------------------------------------ //
  else if ((VAL_LASER == 1) && (Small_Bottle <= 10) && (Medium_Bottle <= 10) && (Large_Bottle >= 11))
  {

    lcd.setCursor (0, 3);
    lcd.print("Medium PlasticBottle");

    for (int i = 1; i <= 2; i++)
    {
      digitalWrite(SEND_POINTS, LOW);
      delay(200);
      digitalWrite(SEND_POINTS, HIGH);
      delay(200);
    }

    setServoAngle(Servo_Accepted, 90);
    delay(1500);
    setServoAngle(Servo_Loadcell, 90);
    delay(1500);
    setServoAngle(Servo_Loadcell, 0);
    delay(1500);
    setServoAngle(Servo_Accepted, 0);
    delay(500);

    pulses_reward += 2;
    weight = 0;

    lcd.setCursor (0, 3);
    lcd.print("                    ");
    Display = true;
  }

  //------------------------------------------ SMALL DETECTION ------------------------------------------ //
  else if ((VAL_LASER == 1) && (Small_Bottle <= 10) && (Medium_Bottle >= 11) && (Large_Bottle >= 11))
  {

    lcd.setCursor (0, 3);
    lcd.print("Small Plastic Bottle");

    for (int i = 1; i <= 1; i++)
    {
      digitalWrite(SEND_POINTS, LOW);
      delay(200);
      digitalWrite(SEND_POINTS, HIGH);
      delay(200);
    }

    setServoAngle(Servo_Accepted, 90);
    delay(1500);
    setServoAngle(Servo_Loadcell, 90);
    delay(1500);
    setServoAngle(Servo_Loadcell, 0);
    delay(1500);
    setServoAngle(Servo_Accepted, 0);
    delay(500);

    pulses_reward += 1;
    weight = 0;

    lcd.setCursor (0, 3);
    lcd.print("                    ");
    Display = true;
  }

  //------------------------------------------ INVALID DETECTION ------------------------------------------ //
  else
  {
    lcd.setCursor (0, 3);
    lcd.print("Item Invalid!       ");
    setServoAngle(Servo_Loadcell, 90);
    delay(1500);
    setServoAngle(Servo_Loadcell, 0);
    delay(1500);
    lcd.setCursor (0, 3);
    lcd.print("Rejected Item!      ");
    Serial.println("Reject Bottle");
    delay(1000);
    lcd.setCursor (0, 3);
    lcd.print("                    ");
  }
}
void PORTAL_VERIFICATION()
{
  int VAL_PORTAL = digitalRead(READY_INSERT);

  if (VAL_PORTAL == 1)
  {
    if (Connect_Portal == true)
    {
      setServoAngle(Servo_Entrance, 90);
      PROCESS_DETECTION();
    }
  }
  else
  {
    lcd.setCursor (0, 0);
    lcd.print("  WELCOME TO SWIFI  ");
    lcd.setCursor (0, 1);
    lcd.print("  PLEASE CONNECT TO ");
    lcd.setCursor (0, 2);
    lcd.print("    WIFI PORTAL     ");
    lcd.setCursor (0, 3);
    lcd.print("                    ");
    Connect_Portal = true;
    pulses_reward = 0;
    Display = false;
    setServoAngle(Servo_Entrance, 0);
  }
}

void LCD_CLEAR()
{
  lcd.setCursor (0, 0);
  lcd.print("                    ");
  lcd.setCursor (0, 1);
  lcd.print("                    ");
  lcd.setCursor (0, 2);
  lcd.print("                    ");
  lcd.setCursor (0, 3);
  lcd.print("                    ");
}
