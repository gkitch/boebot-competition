//Onboard LED code for continent display
//Line Following, Integrated code
//uses the ratio (r, g, b_int / sum) instead of absolute values of r_int, g_int etc
//includes communication w/ other bots and LCD display for final score
#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include <Servo.h>

#define Rx 17 //DOUT to pin 17
#define Tx 16 //DIN to pin 16

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_614MS, TCS34725_GAIN_1X);
char incoming;//declares incoming, print it to PC
char outgoing = 'z';
const int TEAM_NUM = 33;
int tower_state = -1;
int arr[40] = {};
int team_total = 0;
int num_bots = 6; //number of bots required to print 0-6 on LCD. 

//pins for the RGB LED
int red_light_pin= 44;
int green_light_pin = 45;
int blue_light_pin = 46;

//LED on breadboard (green, red LEDs)
int red_LED = 2;
int green_LED = 9;

//PING sensor pin
const int pingPin = 50;
long threshold_dist = 15; //threshold for detection of skyscraper in centimeters

int leftSensor = 49;//sensors for QTI
int middleSensor = 51;
int rightSensor = 53;
Servo servoLeft;//declare servos
Servo servoRight;

int Delay = 90;//delay for each if/else if statement, microsecs
//i.e. how long the robot goes forward/turns/rotates

int threshold = 70;//threshold for light/dark, light: < 70, dark: > 70
long i = 1;//index to count iterations of the loop, to determine when to sense continent

void setup() {//setup, written as two functions 
  //including the line-following and continent-display's setup
  line_follow_setup();
  continent_display_setup();
  task_setup();
  comms_setup();
}

void loop() {
  Serial3.write(18);//backlight is off
  line_follow();//executes line-following every loop.
  //on every 50th iteration of loop and on the 5th iteration
  //calculate the continent. 
  if (i % 80 == 0 || i == 20){
        Stop(); delay(100);
        continent_display();
  }
  i++;
}

//all line-following functions for the loop() (including  
//line-following wrapper function which does all the
//line-following work)
void line_follow(){
  int qtiL = RCTime(leftSensor);//calculates QTI output for left sensor
  //Serial.println(qtiL);//print for debugging
  delay(5);
  int qtiM = RCTime(middleSensor);//calculates QTI output for middle sensor
  //Serial.println(qtiM);
  delay(5);
  int qtiR = RCTime(rightSensor);//calculates QTI output for right sensor
  //Serial.println(qtiR);  
  delay(5);
  if (qtiL > threshold && qtiM > threshold && qtiR > threshold) { 
    //if all three values are reading black, at the hashes e.g.
    delay(1000); //pause for 1 second
    i = 1;//reset for continent sensing
    rotate_and_sense_at_hash();//rotate, sense object, rotate back
    goForward();
    delay(Delay); //go forward, then reanalyze the situation (returns to the top of loop)
    Stop();
  }
  else if (qtiL < threshold && qtiR < threshold && qtiM > threshold) { 
    //only the middle is reading black
    goForward();
    delay(Delay); //go forward, then reanalyze
    Stop();
  }
  else if (qtiL > threshold && qtiM > threshold) { 
    //both left and middle read black, need to rotate
    goForward(); ///this got CHANGED !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! from turnLeft() to goForward()
    delay(Delay); //turn left, then reanalyze
    Stop();
  }
  else if (qtiR > threshold && qtiM > threshold) { 
    //both right and middle read black, need to rotate
    turnRight();
    delay(Delay); //turn right, then reanalyze
    Stop();
  }
  else if (qtiR > threshold){
    //if only the right reads black
    turnRight();//turn right then reanalyze
    delay(Delay);
    Stop();
  }
  else if (qtiL > threshold){
    //if only the left reads black
    goForward();///this got CHANGED !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! from turnLeft() to goForward()
    delay(Delay);//turn left and reanalyze
    Stop();
  }
  else if (qtiL < threshold && qtiR < threshold && qtiM < threshold){
    //if all three sensors are white (e.g. for the square)
    //only meant to turn clockwise
    while (qtiM < threshold){

      turnRight();
      
      //rotate();//rotate clockwise until hitting black
      delay(200);
      qtiM = RCTime(middleSensor);
    }
  }
}
//for line-following, QTI sensors
long RCTime(int sensorIn){//function for determining how reflective the surface is
   //sensorIn can be leftSensor, middleSensor, rightSensor (the three QTI sensors)
   
   long duration = 0;
   pinMode(sensorIn, OUTPUT);     // Make pin OUTPUT
   digitalWrite(sensorIn, HIGH);  // Pin HIGH (discharge capacitor)
   delay(1);                      // Wait 1ms
   pinMode(sensorIn, INPUT);      // Make pin INPUT
   digitalWrite(sensorIn, LOW);   // Turn off internal pullups
   while(digitalRead(sensorIn)){  // Wait for pin to go LOW
      duration++;
   }
   return duration;
}
//helper functions for turning the bot
void turnRight(){//turns right
  servoRight.writeMicroseconds(1550);
  servoLeft.writeMicroseconds(1500);
}

void turnLeft(){//turns left
  servoRight.writeMicroseconds(1500);
  servoLeft.writeMicroseconds(1450);
}

void goForward(){//goes forward
  servoLeft.writeMicroseconds(1400);
  servoRight.writeMicroseconds(1600);
}

void Stop(){//stop
  servoLeft.writeMicroseconds(1500);
  servoRight.writeMicroseconds(1500);
  //i = 1;
}
void rotate(){//rotate clockwise
  servoLeft.writeMicroseconds(1600);
  servoRight.writeMicroseconds(1600);
}
void rotateCounter(){//rotate counterclockwise
  servoLeft.writeMicroseconds(1400);
  servoRight.writeMicroseconds(1400);
}
//this is for sensing the object and all communication
void rotate_and_sense_at_hash(){
  Stop();
  delay(100);
  rotate();
  delay(657);//rotates clockwise, 90 deg
  Stop();
  delay(100);
  long centimeters = measureDistance();
  if (centimeters < threshold_dist){//if dist is less than 15 cm
    digitalWrite(green_LED, HIGH);//the object is present
    tower_state = 1;
  }
  else {digitalWrite(red_LED, HIGH); tower_state = 0;}//if dist > 15cm, object absent
  delay(2000);
  int c = 0;//int c to keep track of iterations through the while loop
  while (!arr_state()){//while the array is NOT full (==6, true; != 6, false in arr_state())
    Serial3.write(12);                                                                            //ADDED ONE LINE (clear LCD)
    print_team_total();                                                                             //ADDED ONE LINE
    if (c % 132 == 0) send_sig(tower_state);//print our signal other bots
    if (Serial2.available()){//if serial2 receives something
      incoming = Serial2.read();//reads the char
      Serial.println(incoming);
      int state = (int) incoming % 2; //1 or 0
      int team_index = (int) incoming / 2;
      arr[team_index] = state;
    }
    c++;//every 100th iteration through the while loop, send our tower's state. 
    //this is so that after every iteration, the tower checks if we've received, 
    //but doesn't send every iteration (to avoid crowding out Serial2 of other bots) 
    print_team_total();                                                                             //ADDED ONE LINE
    //print_team_total prints the values of each bot (e.g. 100--1)
    delay(18);
  }
  team_total = 0;
  for (int i = 31; i <= 36; i++){
    if (arr[i]!=-1) team_total += arr[i];
  }
  //Serial3.write(17);//backlight on (LCD)
  Serial3.write(12);
  delay(5);
  //print the team total, next challenge. 
  Serial3.print(team_total); print_challenge(team_total);
  while (true){//send my signals every 2 seconds after I've received everything
    send_sig(tower_state);
    delay(2000);
  }
  Serial3.write(12);//clear
  Serial3.write(18);//backlight off
  digitalWrite(green_LED, LOW); digitalWrite(red_LED, LOW);
  rotateCounter();//rotate counterclockwise, 90 deg
  delay(657);
  Stop();
  delay(100);
}




//All code for the RGB sensor and color sensor for the loop()

//code for changing RGB's color
void RGB_color(int red_light_value, int green_light_value, int blue_light_value)
 {
  analogWrite(red_light_pin, red_light_value);
  analogWrite(green_light_pin, green_light_value);
  analogWrite(blue_light_pin, blue_light_value);
}

//helper methods for changing the RGB LED's color
void yellowLight(){
  RGB_color(255, 0, 50);
}
void blueLight(){
  RGB_color(0, 255, 140); //originally (0, 255, 0)
}
void magentaLight(){
  RGB_color(0, 0, 255);
}
void redLight(){
  RGB_color(255, 0, 255);
}
void greenLight(){
  RGB_color(255, 255, 0);
}
void whiteLight(){
  RGB_color(0, 0, 0);
}

//function to ensure that all integers are signed ints
int hex_to_dec(uint16_t b){
  String c = String(b, DEC);
  int c_int = (int) b;
  return c_int;
}

//code for displaying the continent
void continent_display(){
uint16_t r, g, b, c, colorTemp, lux;
  delay(1);
  tcs.getRawData(&r, &g, &b, &c);//get raw data for the color sensor
  //these are obsolete integers kept here for convenience. 
  double r_int, g_int, b_int;//declare ints that have values r, g, b
  r_int = hex_to_dec(r); g_int =hex_to_dec(g); b_int = hex_to_dec(b);

  double sum = r_int + g_int + b_int; 
  //all the conditions for detecting the continents and writing LEDs. 
  if (0.38<r_int/sum && r_int/sum<0.46 && 0.36< g_int/sum && g_int/sum < 0.45 && 0.12 < b_int/sum && b_int/sum < 0.22){
    greenLight();
    Serial.println("green");
    //Serial.println(r_int);
  }
  if (0.3 < r_int/sum && r_int/sum < 0.4 && 0.25 < g_int/sum && g_int/sum<0.4 && 0.27<b_int/sum && b_int/sum<0.4){
    blueLight();
    Serial.println("blue");
  }
  if (0.72<r_int/sum && r_int/sum<1 && 0<g_int/sum && g_int/sum <0.16 && 0<b_int/sum && b_int/sum<0.16){
    redLight();
    Serial.println("red");
  }
  if (0.57<r_int/sum && r_int/sum<0.63 && 0.17<g_int/sum && g_int/sum<0.23 && 0.17<b_int/sum && b_int/sum<0.23){
    magentaLight();
        Serial.println("magenta");

  }
  if (0.44<r_int/sum && r_int/sum<0.50 && 0.28<g_int/sum && g_int/sum<0.34 && 0.19<b_int/sum && b_int/sum<0.25){
    whiteLight();
        Serial.println("white");
  }
  //printing for trouble shooting only
  Serial.println(r_int/sum); Serial.println(g_int/sum); Serial.println(b_int/sum);
  if (0.5<r_int/sum && r_int/sum<0.55 && 0.3<g_int/sum && g_int/sum<0.35 && 0.1<b_int/sum && b_int/sum<0.15){
    yellowLight();
    Serial.println("yellow");
  }
}

//code for PING sensor

//helper
long microsecondsToInches(long microseconds) {
  // According to Parallax's datasheet for the PING))), there are 73.746
  // microseconds per inch (i.e. sound travels at 1130 feet per second).
  // This gives the distance travelled by the ping, outbound and return,
  // so we divide by 2 to get the distance of the obstacle.
  // See: https://www.parallax.com/package/ping-ultrasonic-distance-sensor-downloads/
  return microseconds / 74 / 2;
}

//helper
long microsecondsToCentimeters(long microseconds) {
  // The speed of sound is 340 m/s or 29 microseconds per centimeter.
  // The ping travels out and back, so to find the distance of the object we
  // take half of the distance travelled.
  return microseconds / 29 / 2;
}

//measure distance of object
long measureDistance(){
  long duration, inches, cm;

  // The PING))) is triggered by a HIGH pulse of 2 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  pinMode(pingPin, OUTPUT);
  digitalWrite(pingPin, LOW);
  delayMicroseconds(2);
  digitalWrite(pingPin, HIGH);//sends off a pulse
  delayMicroseconds(5);
  digitalWrite(pingPin, LOW);

  // The same pin is used to read the signal from the PING))): a HIGH pulse
  // whose duration is the time (in microseconds) from the sending of the ping
  // to the reception of its echo off of an object.
  pinMode(pingPin, INPUT);
  duration = pulseIn(pingPin, HIGH);
  //pulseIn: (pingPin, HIGH/LOW). Reads signal from pingPin, either reads HIGH
  //or LOW signal. Here, pulseIn waits for pingPin to go from LOW to HIGH, starts
  //timing, and then when pingPin goes to LOW again, stops timing. 

  // convert the time into a distance
  inches = microsecondsToInches(duration);
  cm = microsecondsToCentimeters(duration);
  return cm;
}

//code for communication
void send_sig(int tower_state){
  outgoing = (char) tower_state + 2 * TEAM_NUM;
  arr[33] = tower_state;
  Serial2.print(outgoing);
}

//Checks if we've received something from all 6 bots
boolean arr_state(){
  int count = 0;
  for (int i = 31; i <= 36; i++){
    if (arr[i] != -1) count++;
    Serial.print(arr[i]);
  }
  Serial.println(" " + count);
  return count == num_bots;
}

//prints the received signal from each team (1 or 0 or "-")
void print_team_total(){
  Serial3.print(team_total + " ");
  for (int i = 31; i <= 36; i++){
    if (arr[i] != -1) Serial3.print(arr[i]);
    else Serial3.print("-");
  }
  Serial3.print(" ");
}

//prints the next grand challenge based on team_total
void print_challenge(int team_total){
  if (team_total == 0){
    Serial3.print("Nitrogen");
  }
  if (team_total == 1){
    Serial3.print("HInformatics");
  }
  if (team_total == 2){
    Serial3.print("Medicine");
  }
  if (team_total == 3){
    Serial3.print("VirtualR");
  }
  if (team_total == 4){
    Serial3.print("Discovery");
  }
  if (team_total == 5){
    Serial3.print("Brain");
  }
  if (team_total == 6){
    Serial3.print("PLearning");
  }
}


//setup wrapper functions for line following and continent display and
//detecting our object (task_setup()). 
void line_follow_setup(){
  servoLeft.attach(12);
  servoRight.attach(11);
  Serial.begin(9600);//sets up servos and serial in case of 
  //troubleshooting. 
}
void continent_display_setup(){
  tcs.begin();//begins the tcs after it's initialised
  pinMode(red_light_pin, OUTPUT);//pins for the RGB LED
  pinMode(green_light_pin, OUTPUT);
  pinMode(blue_light_pin, OUTPUT);
  RGB_color(255, 255, 255);//turns off the RGB LED initially
}
void task_setup(){
  pinMode(red_LED, OUTPUT);
  pinMode(green_LED, OUTPUT);
  digitalWrite(red_LED, LOW);
  digitalWrite(green_LED, LOW);
}
void comms_setup(){
  for (int i = 0; i < 40; i++) arr[i] = -1;//initialises the array arr with -1
  Serial2.begin(9600);//XBee
  Serial3.begin(9600);//LCD
  Serial3.write(12);//clear everything
  Serial3.write(18);//backlight is off
}
