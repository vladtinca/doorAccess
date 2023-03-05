#include <Servo.h>
#include <Keypad.h>
#include <SPI.h>
#include <MFRC522.h>

//pin define
#define piezo A0                
#define butonProgramare 2      
#define blue 3
#define green 4
#define red 5
#define doorMotorPin 6
#define RST_PIN 9
#define SS_PIN 10
//pins 0,1 reserved for Serial communication with Bluetooth
//pins 11,12,13 reserved for SPI communication with RFID

//Constants
const int minim = 10;            //minimul dupa care se considera bataie
const int rejectValue = 30;        
const int mediaRejectValue = 20; 
const int knockDebounce = 100;      //time after 1 knock (debounce time)
const int maxKnocks = 15;       // Maximum number of knocks to listen for.
const int knockComplete = 1000;      //delay after knocking
const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {A1, A2, A3, A4}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {A5,7,8}; //connect to the column pinouts of the keypad

// Variables.
int codSecret[maxKnocks] = {100, 100, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  //initial knock
int knockReadings[maxKnocks];   //delay between knocks
int piezoValoare = 0;           // piezo reading.
int state = 0;
int doorState = 0;
int i=0;
int ok=1;
char inKeys[10]={};
char memKeys[10]={'1','2','3','4'};

//Initialisation
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
MFRC522 mfrc522(SS_PIN, RST_PIN); 
Servo motorDoor;

void setup() {
  pinMode(blue, OUTPUT);
  pinMode(red, OUTPUT);
  pinMode(green, OUTPUT);
  pinMode(butonProgramare, INPUT);
  
  digitalWrite(green, LOW); 
  digitalWrite(red, LOW);
  digitalWrite(blue, LOW);

  motorDoor.attach(doorMotorPin);
  reset();

Serial.begin(9600);
SPI.begin();
mfrc522.PCD_Init();
//Serial.println("Program start."); 
}
void loop() {
 digitalWrite(red, LOW);
 digitalWrite(blue, LOW);
 digitalWrite(green, HIGH);
 
 //From KeyPad
 
char key = keypad.getKey();
  if (key != NO_KEY){
    Serial.println(key);
                    if(key!='*'&&key!='#'){
                                          inKeys[i]=key;
                                          //Serial.println(key);
                                          i++;
                                          digitalWrite(green, LOW);
                                          delay(100);        
                                          }
                                          else{
                                              if(key=='*'){
                                                //Check
                                                          for(int y;y<10;y++){
                                                                              if(inKeys[y]!=memKeys[y]){
                                                                                                        ok=0;
                                                                                                        }
                                                                              }
                                                          if(ok==1){
                                                                    if(doorState==0){
                                                                                     deschide();
                                                                                    }
                                                                      else{
                                                                           if(doorState==1){
                                                                                            inchide();
                                                                                           }
                                                                          }
                                                                    }
                                                                    else{
                                                                        wrong();
                                                                        for(int y=0;y<10;y++){
                                                                                              inKeys[y]={};
                                                                                              }
                                                                        ok=1;
                                                                        i=0;
                                                                        }
                                                          }
                                                
                                                else {
                                                      if(key=='#'){
                                                                  wrong();
                                                                  for(int y=0;y<10;y++){
                                                                                        inKeys[y]={};
                                                                                        }
                                                                  ok=1;
                                                                  i=0;
                                                                  }
                                                     }
                                                }
                     }
           

  //From App
if(Serial.available() > 0){
    state = Serial.read();
    if (state == '1') {
          deschide();
              } 
    if (state == '0') {
          inchide();
              }
    if (state == '2') {
          reset();
              }
 }
  //Piezo
  piezoValoare = analogRead(piezo);
  if (digitalRead(butonProgramare)==0)
      {
     
     //for(int i=0;i<maxKnocks;i++){
      //    Serial.println(codSecret[i]);}
      ascultaProgramare();
      //for(int i=0;i<maxKnocks;i++){
        //  Serial.println(codSecret[i]);}
      }
  else{
    if (piezoValoare >=minim)
              {
          ascultaBataie();
         if(validare()){
              if(doorState==0){
                  deschide();}
          else{
               if(doorState==1){
                  inchide();}
          }
         }
         else{
          wrong();
          }
          //Serial.println("Cod Gresit!");
               }
               }
//RFID

if (mfrc522.PICC_IsNewCardPresent()) 
  {
  // Select one of the cards
  if (mfrc522.PICC_ReadCardSerial()) 
  {
  //Show UID on serial monitor
  Serial.print("UID tag :");
  String content= "";
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  Serial.print("Message : ");
  content.toUpperCase();
  if (content.substring(1) == "5A E7 DD 58") //change here the UID of the card/cards that you want to give access
  {
    Serial.println("Authorized access");
    if(doorState==0){
                   deschide();
                  }
    else{
         if(doorState==1){
                          inchide();
                         }
        }
  }
 
 else   {
   Serial.println(" Access denied");
   wrong();
  }}}
   
}
void ascultaBataie(){
   //Serial.println("Knock starting");   

//reset knockReadings
  for (int i=0;i<maxKnocks;i++){
    knockReadings[i]=0;
  }
  
  int currentKnockNumber=0;
  int startTime=millis();
  int now;
  
//Green Blink and delay
  digitalWrite(green, LOW);
  delay(knockDebounce);
  digitalWrite(green, HIGH);
    
  do {
    //listen for the next knock
    piezoValoare = analogRead(piezo);
    if (piezoValoare>=minim){                  
     // Serial.println("knock.");
      
      now=millis();
      knockReadings[currentKnockNumber] = now-startTime;
      currentKnockNumber ++;                      
      startTime=now; 
  //Green Blink and delay    
      digitalWrite(green, LOW);
      delay(knockDebounce);
      digitalWrite(green, HIGH); 
    }

    now=millis();
    
    //timeout or run out of knocks?
  } while ((now-startTime < knockComplete) and (currentKnockNumber < maxKnocks));
}


boolean validare(){
  int currentKnockCount = 0;
  int secretKnockCount = 0;
  int maxKnockInterval = 0; 
  
  for (int i=0;i<maxKnocks;i++){
    if (knockReadings[i] > 0){
      currentKnockCount++;
    }
    if (codSecret[i] > 0){
      secretKnockCount++;
    }
    
    if (knockReadings[i] > maxKnockInterval){
      maxKnockInterval = knockReadings[i];
    }}
    
  int totaltimeDifferences=0;
  int timeDiff=0;
  for (int i=0;i<maxKnocks;i++){ 
    knockReadings[i]= map(knockReadings[i],0, maxKnockInterval, 0, 100);      
    timeDiff = abs(knockReadings[i]-codSecret[i]);
    
    if (timeDiff > rejectValue){
      
      return false;
    }
    totaltimeDifferences += timeDiff;
  }
  
  if (totaltimeDifferences/secretKnockCount>mediaRejectValue){
    
    return false; 
  }


  return true;
  
}

void ascultaProgramare(){
  int prog=1;
  delay(200);
  for(int i=0;i<100;i++){
   if (digitalRead(butonProgramare)==0)
    {prog=0;
      }
    delay(10);
    }
if(prog==0){
  int i=0;
  int out=0;
  for(int y=0;y<10;y++){
      memKeys[y]={};}
  //Programare KeyPad
  
  digitalWrite(green, LOW);
  while(out==0){
    digitalWrite(red, HIGH);
    digitalWrite(blue, HIGH);
    if(char key = keypad.getKey())
    {
      digitalWrite(red, LOW);
      digitalWrite(blue, LOW);
      delay(100);
    Serial.println(key);
      if(key!='*'&&key!='#'){
      memKeys[i]=key;
      i++;
      }
      else{out=1;}
      }
      }
    digitalWrite(blue, LOW);
  for (int i=0;i<3;i++){
      delay(100);
      digitalWrite(red, HIGH);
      digitalWrite(green, LOW);
      delay(100);
      digitalWrite(red, LOW);
      digitalWrite(green, HIGH);      
    }
  }
  else{
    //Programare Knock

 // Serial.println("Knock programming");   
//reset knockReadings
    digitalWrite(red, HIGH);
  for (int i=0;i<maxKnocks;i++){
    knockReadings[i]=0;
  }

  while(analogRead(piezo)<=minim){}
  int currentKnockNumber=0;
  int startTime=millis();
  int now;
  int maxKnockInterval;
  
//Green Blink and delay
  digitalWrite(green, LOW);
  digitalWrite(red, LOW);
  delay(knockDebounce);
  digitalWrite(green, HIGH);
  digitalWrite(red, HIGH);
    
  do {
    //listen for the next knock
    piezoValoare = analogRead(piezo);
    if (piezoValoare>=minim){                  
    //  Serial.println("knock.");
      
      now=millis();
      knockReadings[currentKnockNumber] = now-startTime;
      currentKnockNumber ++;                      
      startTime=now; 
  //Green Blink and delay    
      digitalWrite(green, LOW);
      digitalWrite(red, LOW);
      delay(knockDebounce);
      digitalWrite(green, HIGH);
      digitalWrite(red, HIGH); 
    }

    now=millis();
    
    //timeout or run out of knocks?
  } while ((now-startTime < knockComplete) and (currentKnockNumber < maxKnocks));
  //Replace codsecret
  
for(int i=0;i<maxKnocks;i++){
  codSecret[i]=knockReadings[i];
 }
 for(int i=0;i<maxKnocks;i++){
 if (codSecret[i] > maxKnockInterval){
      maxKnockInterval = codSecret[i];
  }}
    for(int i=0;i<maxKnocks;i++){
      codSecret[i]= map(codSecret[i],0, maxKnockInterval, 0, 100);
    }
 
 //Serial.println("Noul cod memorat");
  for (int i=0;i<3;i++){
      delay(100);
      digitalWrite(red, HIGH);
      digitalWrite(green, LOW);
      delay(100);
      digitalWrite(red, LOW);
      digitalWrite(green, HIGH);      
    }
}}
void deschide(){
  if(doorState==0){
  digitalWrite(red, LOW);
  digitalWrite(green, LOW);
  digitalWrite(blue, HIGH);
  delay(500);
  Serial.println("OPEN");
 
  for (int pos = 42; pos <= 140; pos += 1) {
    motorDoor.write(pos);
    delay(5); }
 doorState = 1;}
 state = 0;
 delay(500);
  }
  
void inchide(){
  if(doorState==1){
  digitalWrite(red, LOW);
  digitalWrite(green, LOW);
  digitalWrite(blue, HIGH);
  delay(500);
  Serial.println("CLOSED");
   for (int pos = 140; pos >= 42; pos -= 1) {
    motorDoor.write(pos);
    delay(5);}
   doorState = 0;}
  state = 0;
  delay(500);
 }
 
 void reset(){

  for(int i=55;i>=45;i=i-10){
      motorDoor.write(i);
      delay(300);}
  motorDoor.write(42);
  doorState = 0;
  state=0;
  delay(500);
}
 void wrong(){
          digitalWrite(green, LOW);
          for(int i=0;i<3;i++){
          digitalWrite(red, HIGH);
          delay(100);
          digitalWrite(red, LOW);
          delay(100);}
  
  
  }
  
  
