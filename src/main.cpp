#include <Arduino.h>
#include <sfm.hpp>

#define SFM_RX 19
#define SFM_TX 18
#define SFM_IRQ 5
#define SFM_VCC 15

#define lockPin 21 

int lockOpenTime = 2000; // Adjust as needed

SFM_Module SFM(SFM_VCC, SFM_IRQ, SFM_TX, SFM_RX);

uint8_t fingerprints = 0; // Used to store the number of templates in the sensor
bool lastTouchState = 0; 
int touchHold = 0;  // Used touch and hold menu system

void sfmPinInt1() {
  SFM.pinInterrupt();
}

uint8_t temp = 0; // used to get recognition return
uint16_t tempUid = 0; // used to get recognized uid

void unlock(){
  digitalWrite(lockPin, HIGH);
  Serial.println("Lock is opened....");
  delay(lockOpenTime);
  digitalWrite(lockPin, LOW); 
  Serial.println("Lock is closed....");
}

void enroll() {
  // Enroll fingerprints
  Serial.println("Start register...");
  SFM.setRingColor(SFM_RING_YELLOW);
  Serial.println("Please put your finger");

  temp = SFM.register_3c3r_1st();
  if (temp == SFM_ACK_SUCCESS) {
    Serial.println("Please release your finger");
    delay(1000);
    SFM.setRingColor(SFM_RING_PURPLE);

    Serial.println("Please put your finger again");
    temp = SFM.register_3c3r_2nd();
    if (temp == SFM_ACK_SUCCESS) {
      Serial.println("Please release your finger");
      delay(1000);
      SFM.setRingColor(SFM_RING_BLUE);

      Serial.println("Please put your finger a final time");
      tempUid = 0;
      temp = SFM.register_3c3r_3rd(tempUid);

      if (temp == SFM_ACK_SUCCESS && tempUid != 0) {
        Serial.printf("Register successful with return UID: %d\n", tempUid);
        for (int i = 0; i < 4; i++) {  
          SFM.setRingColor(SFM_RING_GREEN);
          delay(200);
          SFM.setRingColor(SFM_RING_OFF);
          delay(200);
        }
      } else {
        Serial.println("Register failed, please re-submit the register command");
        SFM.setRingColor(SFM_RING_RED);
      }
    } else {
      Serial.println("Error in register step #2");
    }
  } else {
    Serial.println("Error in register step #1");
    
  }
}

void setup() {
  pinMode(lockPin, OUTPUT);
  SFM.setPinInterrupt(sfmPinInt1); // must perform this step in setup() to attach the inner interrupt.
  Serial.begin(115200); // not affiliated with module
  Serial.println("Init.......");
  SFM.enable();
  delay(200);
  SFM.setRingColor(SFM_RING_OFF);
  fingerprints = SFM.getUserCount();
  if (SFM.isConnected()) {
    Serial.println("Found fingerprint sensor!");
    if (fingerprints == 0) {
      Serial.println("Sensor doesn't contain any fingerprints");
      Serial.println("Enroll admin fingerprint.......");
      enroll();
      
    } 
    else{
      Serial.print("Sensor contains ");
      Serial.print(fingerprints); 
      Serial.println(" templates");
    
    }
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); } // Loops until fingerprint sensor is found. Prevents entering the loop if the sensor isn't found
  }
  //delay(1000);
}

void deleteAll(){
  Serial.println("Deleting all fingerprints...");
  fingerprints = SFM.getUserCount();
  Serial.printf("Current fingerprint count: %d\n", fingerprints);

  temp = SFM.deleteAllUser();
  delay(2000);

  if (temp == SFM_ACK_SUCCESS) {
    Serial.println("Successfully deleted all users");
    SFM.setRingColor(SFM_RING_RED);
    delay(1500);
    SFM.setRingColor(SFM_RING_OFF);
  } else {
    Serial.println("Failed to delete users");
  }
}

void selectiveDelete(){
  Serial.println("Initializing selective fingerprint deletion......");
  Serial.println("Place the finger that needs to be deleted");
  SFM.setRingColor(SFM_RING_RED);
  temp = SFM.recognition_1vN(tempUid);
    if (tempUid != 0) {
      if(SFM.deleteUser(tempUid) == SFM_ACK_SUCCESS){
        Serial.printf("\nSuccessfully deleted UID: %d\n", tempUid);
        for (int i = 0; i < 4; i++){
          SFM.setRingColor(SFM_RING_RED);
          delay(200);
          SFM.setRingColor(SFM_RING_OFF);
          delay(200);
        }
      }
    }
    else{
      Serial.println("Fingerprint not found");
      SFM.setRingColor(SFM_RING_OFF);
    }

}

uint8_t lockLoop() {
  if (SFM.isTouched() != lastTouchState) {  
    lastTouchState = SFM.isTouched();  // Update touch state
    
    if (SFM.isTouched()) {
      temp = SFM.recognition_1vN(tempUid);      
      SFM.setRingColor(SFM_RING_CYAN); 
      delay(200);
      SFM.setRingColor(SFM_RING_OFF);

      if (tempUid != 0) {
        SFM.setRingColor(SFM_RING_GREEN);
        Serial.printf("\nSuccessfully matched with UID: %d\n", tempUid);
        unlock();
        if (tempUid > 1){
          Serial.println("User finger...");
          //unlock();
          while(SFM.isTouched()){
            delay(1000);
            touchHold += 1;
            Serial.println(touchHold);
            if (touchHold > 10){
              delay(100);
              Serial.printf("\nDeleting UID: %d\n", tempUid);
              for(int i = 0; i < 4; i++){
                SFM.setRingColor(SFM_RING_RED);
                delay(500);
                SFM.setRingColor(SFM_RING_OFF);
                delay(500);
              }
              if(SFM.deleteUser(tempUid) == SFM_ACK_SUCCESS){
                Serial.printf("\nSuccessfully deleted UID: %d\n", tempUid);
              }
            }
          }
        }
        if (tempUid == 1){
          Serial.println("Admin finger...");
          //unlock();
          while(SFM.isTouched()){
            delay(1000);
            touchHold += 1;
            Serial.println(touchHold);
            if(touchHold > 10){
              for( int i = 0; i < 2; i++){
                SFM.setRingColor(SFM_RING_GREEN);
                delay(100);
                SFM.setRingColor(SFM_RING_OFF);
                delay(100);
            }
            if(touchHold > 10 && !SFM.isTouched()){
              Serial.println("Enrolling user fingerprint..........");
              for(int i = 0; i < 4; i++){
                SFM.setRingColor(SFM_RING_GREEN, SFM_RING_OFF);
                delay(1000);
                touchHold = 0;
              }
              enroll();
              }

            }
            if (touchHold > 15){
              delay(200);
              deleteAll();
              for(int i = 0; i < 2; i++){
                SFM.setRingColor(SFM_RING_RED, SFM_RING_OFF);
                delay(1000);
                touchHold = 0;
              }
              Serial.println("Enroll admin fingerprint.......");
              enroll();
            }
          }
        }
        touchHold = 0;
        return tempUid;
      }
    } else {
      SFM.setRingColor(SFM_RING_OFF);
      delay(300);
    }
  }
  return 0; 
}

void loop() {
  
  lockLoop();

}
