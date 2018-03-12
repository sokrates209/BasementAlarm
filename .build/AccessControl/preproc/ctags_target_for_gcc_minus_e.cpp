# 1 "c:\\Infrastructure\\Arduino\\BasementAlarm\\Lib\\AccessControl.ino"
# 1 "c:\\Infrastructure\\Arduino\\BasementAlarm\\Lib\\AccessControl.ino"
# 2 "c:\\Infrastructure\\Arduino\\BasementAlarm\\Lib\\AccessControl.ino" 2
# 3 "c:\\Infrastructure\\Arduino\\BasementAlarm\\Lib\\AccessControl.ino" 2
# 4 "c:\\Infrastructure\\Arduino\\BasementAlarm\\Lib\\AccessControl.ino" 2
# 22 "c:\\Infrastructure\\Arduino\\BasementAlarm\\Lib\\AccessControl.ino"
boolean match = false; // initialize card match to false
boolean programMode = false; // initialize programming mode to false
boolean replaceMaster = false;

int successRead; // Variable integer to keep if we have Successful Read from Reader

byte storedCard[4]; // Stores an ID read from EEPROM
byte readCard[4]; // Stores scanned ID read from RFID Module
byte masterCard[4]; // Stores master card's ID read from EEPROM




MFRC522 mfrc522(10, 9); // Create MFRC522 instance.

///////////////////////////////////////// Setup ///////////////////////////////////
void setup() {
  //Arduino Pin Configuration
  pinMode(7 /* Set Led Pins*/, 0x1);
  pinMode(6, 0x1);
  pinMode(5, 0x1);
  pinMode(3 /* Button pin for WipeMode*/, 0x2); // Enable pin's pull up resistor
  pinMode(4 /* Set Relay Pin*/, 0x1);
  //Be careful how relay circuit behave on while resetting or power-cycling your Arduino
  digitalWrite(4 /* Set Relay Pin*/, 0x1); // Make sure door is locked
  digitalWrite(7 /* Set Led Pins*/, 0x1); // Make sure led is off
  digitalWrite(6, 0x1); // Make sure led is off
  digitalWrite(5, 0x1); // Make sure led is off

  //Protocol Configuration
  Serial.begin(9600); // Initialize serial communications with PC
  SPI.begin(); // MFRC522 Hardware uses SPI protocol
  mfrc522.PCD_Init(); // Initialize MFRC522 Hardware

  //If you set Antenna Gain to Max it will increase reading distance
  //mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);

  Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("Access Control v3.4"); &__c[0];}))))); // For debugging purposes
  ShowReaderDetails(); // Show details of PCD - MFRC522 Card Reader details

  //Wipe Code if Button Pressed while setup run (powered on) it wipes EEPROM
  if (digitalRead(3 /* Button pin for WipeMode*/) == 0x0) { // when button pressed pin should get low, button connected to ground
    digitalWrite(7 /* Set Led Pins*/, 0x0); // Red Led stays on to inform user we are going to wipe
    Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("Wipe Button Pressed"); &__c[0];})))));
    Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("You have 15 seconds to Cancel"); &__c[0];})))));
    Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("This will be remove all records and cannot be undone"); &__c[0];})))));
    delay(15000); // Give user enough time to cancel operation
    if (digitalRead(3 /* Button pin for WipeMode*/) == 0x0) { // If button still be pressed, wipe EEPROM
      Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("Starting Wiping EEPROM"); &__c[0];})))));
      for (int x = 0; x < EEPROM.length(); x = x + 1) { //Loop end of EEPROM address
        if (EEPROM.read(x) == 0) { //If EEPROM address 0
          // do nothing, already clear, go to the next address in order to save time and reduce writes to EEPROM
        }
        else {
          EEPROM.write(x, 0); // if not write 0 to clear, it takes 3.3mS
        }
      }
      Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("EEPROM Successfully Wiped"); &__c[0];})))));
      digitalWrite(7 /* Set Led Pins*/, 0x1); // visualize successful wipe
      delay(200);
      digitalWrite(7 /* Set Led Pins*/, 0x0);
      delay(200);
      digitalWrite(7 /* Set Led Pins*/, 0x1);
      delay(200);
      digitalWrite(7 /* Set Led Pins*/, 0x0);
      delay(200);
      digitalWrite(7 /* Set Led Pins*/, 0x1);
    }
    else {
      Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("Wiping Cancelled"); &__c[0];})))));
      digitalWrite(7 /* Set Led Pins*/, 0x1);
    }
  }
  // Check if master card defined, if not let user choose a master card
  // This also useful to just redefine Master Card
  // You can keep other EEPROM records just write other than 143 to EEPROM address 1
  // EEPROM address 1 should hold magical number which is '143'
  if (EEPROM.read(1) != 143) {
    Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("No Master Card Defined"); &__c[0];})))));
    Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("Scan A PICC to Define as Master Card"); &__c[0];})))));
    do {
      successRead = getID(); // sets successRead to 1 when we get read from reader otherwise 0
      digitalWrite(5, 0x0); // Visualize Master Card need to be defined
      delay(200);
      digitalWrite(5, 0x1);
      delay(200);
    }
    while (!successRead); // Program will not go further while you not get a successful read
    for ( int j = 0; j < 4; j++ ) { // Loop 4 times
      EEPROM.write( 2 + j, readCard[j] ); // Write scanned PICC's UID to EEPROM, start from address 3
    }
    EEPROM.write(1, 143); // Write to EEPROM we defined Master Card.
    Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("Master Card Defined"); &__c[0];})))));
  }
  Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("-------------------"); &__c[0];})))));
  Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("Master Card's UID"); &__c[0];})))));
  for ( int i = 0; i < 4; i++ ) { // Read Master Card's UID from EEPROM
    masterCard[i] = EEPROM.read(2 + i); // Write it to masterCard
    Serial.print(masterCard[i], 16);
  }
  Serial.println("");
  Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("-------------------"); &__c[0];})))));
  Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("Everything Ready"); &__c[0];})))));
  Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("Waiting PICCs to be scanned"); &__c[0];})))));
  cycleLeds(); // Everything ready lets give user some feedback by cycling leds
}


///////////////////////////////////////// Main Loop ///////////////////////////////////
void loop () {
  do {
    successRead = getID(); // sets successRead to 1 when we get read from reader otherwise 0
    if (digitalRead(3 /* Button pin for WipeMode*/) == 0x0) {
      digitalWrite(7 /* Set Led Pins*/, 0x0); // Make sure led is off
      digitalWrite(6, 0x1); // Make sure led is off
      digitalWrite(5, 0x1); // Make sure led is off
      Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("Wipe Button Pressed"); &__c[0];})))));
      Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("Master Card will be Erased! in 10 seconds"); &__c[0];})))));
      delay(10000);
      if (digitalRead(3 /* Button pin for WipeMode*/) == 0x0) {
        EEPROM.write(1, 0); // Reset Magic Number.
        Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("Restart device to re-program Master Card"); &__c[0];})))));
        while (1);
      }
    }
    if (programMode) {
      cycleLeds(); // Program Mode cycles through RGB waiting to read a new card
    }
    else {
      normalModeOn(); // Normal mode, blue Power LED is on, all others are off
    }
  }
  while (!successRead); //the program will not go further while you not get a successful read
  if (programMode) {
    if ( isMaster(readCard) ) { //If master card scanned again exit program mode
      Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("Master Card Scanned"); &__c[0];})))));
      Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("Exiting Program Mode"); &__c[0];})))));
      Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("-----------------------------"); &__c[0];})))));
      programMode = false;
      return;
    }
    else {
      if ( findID(readCard) ) { // If scanned card is known delete it
        Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("I know this PICC, removing..."); &__c[0];})))));
        deleteID(readCard);
        Serial.println("-----------------------------");
        Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("Scan a PICC to ADD or REMOVE to EEPROM"); &__c[0];})))));
      }
      else { // If scanned card is not known add it
        Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("I do not know this PICC, adding..."); &__c[0];})))));
        writeID(readCard);
        Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("-----------------------------"); &__c[0];})))));
        Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("Scan a PICC to ADD or REMOVE to EEPROM"); &__c[0];})))));
      }
    }
  }
  else {
    if ( isMaster(readCard)) { // If scanned card's ID matches Master Card's ID enter program mode
      programMode = true;
      Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("Hello Master - Entered Program Mode"); &__c[0];})))));
      int count = EEPROM.read(0); // Read the first Byte of EEPROM that
      Serial.print((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("I have "); &__c[0];}))))); // stores the number of ID's in EEPROM
      Serial.print(count);
      Serial.print((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = (" record(s) on EEPROM"); &__c[0];})))));
      Serial.println("");
      Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("Scan a PICC to ADD or REMOVE to EEPROM"); &__c[0];})))));
      Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("Scan Master Card again to Exit Program Mode"); &__c[0];})))));
      Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("-----------------------------"); &__c[0];})))));
    }
    else {
      if ( findID(readCard) ) { // If not, see if the card is in the EEPROM
        Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("Welcome, You shall pass"); &__c[0];})))));
        granted(300); // Open the door lock for 300 ms
      }
      else { // If not, show that the ID was not valid
        Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("You shall not pass"); &__c[0];})))));
        denied();
      }
    }
  }
}

/////////////////////////////////////////  Access Granted    ///////////////////////////////////
void granted (int setDelay) {
  digitalWrite(5, 0x1); // Turn off blue LED
  digitalWrite(7 /* Set Led Pins*/, 0x1); // Turn off red LED
  digitalWrite(6, 0x0); // Turn on green LED
  digitalWrite(4 /* Set Relay Pin*/, 0x0); // Unlock door!
  delay(setDelay); // Hold door lock open for given seconds
  digitalWrite(4 /* Set Relay Pin*/, 0x1); // Relock door
  delay(1000); // Hold green LED on for a second
}

///////////////////////////////////////// Access Denied  ///////////////////////////////////
void denied() {
  digitalWrite(6, 0x1); // Make sure green LED is off
  digitalWrite(5, 0x1); // Make sure blue LED is off
  digitalWrite(7 /* Set Led Pins*/, 0x0); // Turn on red LED
  delay(1000);
}


///////////////////////////////////////// Get PICC's UID ///////////////////////////////////
int getID() {
  // Getting ready for Reading PICCs
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) { //Since a PICC placed get Serial and continue
    return 0;
  }
  // There are Mifare PICCs which have 4 byte or 7 byte UID care if you use 7 byte PICC
  // I think we should assume every PICC as they have 4 byte UID
  // Until we support 7 byte PICCs
  Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("Scanned PICC's UID:"); &__c[0];})))));
  for (int i = 0; i < 4; i++) { //
    readCard[i] = mfrc522.uid.uidByte[i];
    Serial.print(readCard[i], 16);
  }
  Serial.println("");
  mfrc522.PICC_HaltA(); // Stop reading
  return 1;
}

void ShowReaderDetails() {
  // Get the MFRC522 software version
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.print((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("MFRC522 Software Version: 0x"); &__c[0];})))));
  Serial.print(v, 16);
  if (v == 0x91)
    Serial.print((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = (" = v1.0"); &__c[0];})))));
  else if (v == 0x92)
    Serial.print((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = (" = v2.0"); &__c[0];})))));
  else
    Serial.print((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = (" (unknown),probably a chinese clone?"); &__c[0];})))));
  Serial.println("");
  // When 0x00 or 0xFF is returned, communication probably failed
  if ((v == 0x00) || (v == 0xFF)) {
    Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("WARNING: Communication failure, is the MFRC522 properly connected?"); &__c[0];})))));
    Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("SYSTEM HALTED: Check connections."); &__c[0];})))));
    while (true); // do not go further
  }
}

///////////////////////////////////////// Cycle Leds (Program Mode) ///////////////////////////////////
void cycleLeds() {
  digitalWrite(7 /* Set Led Pins*/, 0x1); // Make sure red LED is off
  digitalWrite(6, 0x0); // Make sure green LED is on
  digitalWrite(5, 0x1); // Make sure blue LED is off
  delay(200);
  digitalWrite(7 /* Set Led Pins*/, 0x1); // Make sure red LED is off
  digitalWrite(6, 0x1); // Make sure green LED is off
  digitalWrite(5, 0x0); // Make sure blue LED is on
  delay(200);
  digitalWrite(7 /* Set Led Pins*/, 0x0); // Make sure red LED is on
  digitalWrite(6, 0x1); // Make sure green LED is off
  digitalWrite(5, 0x1); // Make sure blue LED is off
  delay(200);
}

//////////////////////////////////////// Normal Mode Led  ///////////////////////////////////
void normalModeOn () {
  digitalWrite(5, 0x0); // Blue LED ON and ready to read card
  digitalWrite(7 /* Set Led Pins*/, 0x1); // Make sure Red LED is off
  digitalWrite(6, 0x1); // Make sure Green LED is off
  digitalWrite(4 /* Set Relay Pin*/, 0x1); // Make sure Door is Locked
}

//////////////////////////////////////// Read an ID from EEPROM //////////////////////////////
void readID( int number ) {
  int start = (number * 4 ) + 2; // Figure out starting position
  for ( int i = 0; i < 4; i++ ) { // Loop 4 times to get the 4 Bytes
    storedCard[i] = EEPROM.read(start + i); // Assign values read from EEPROM to array
  }
}

///////////////////////////////////////// Add ID to EEPROM   ///////////////////////////////////
void writeID( byte a[] ) {
  if ( !findID( a ) ) { // Before we write to the EEPROM, check to see if we have seen this card before!
    int num = EEPROM.read(0); // Get the numer of used spaces, position 0 stores the number of ID cards
    int start = ( num * 4 ) + 6; // Figure out where the next slot starts
    num++; // Increment the counter by one
    EEPROM.write( 0, num ); // Write the new count to the counter
    for ( int j = 0; j < 4; j++ ) { // Loop 4 times
      EEPROM.write( start + j, a[j] ); // Write the array values to EEPROM in the right position
    }
    successWrite();
    Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("Succesfully added ID record to EEPROM"); &__c[0];})))));
  }
  else {
    failedWrite();
    Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("Failed! There is something wrong with ID or bad EEPROM"); &__c[0];})))));
  }
}

///////////////////////////////////////// Remove ID from EEPROM   ///////////////////////////////////
void deleteID( byte a[] ) {
  if ( !findID( a ) ) { // Before we delete from the EEPROM, check to see if we have this card!
    failedWrite(); // If not
    Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("Failed! There is something wrong with ID or bad EEPROM"); &__c[0];})))));
  }
  else {
    int num = EEPROM.read(0); // Get the numer of used spaces, position 0 stores the number of ID cards
    int slot; // Figure out the slot number of the card
    int start; // = ( num * 4 ) + 6; // Figure out where the next slot starts
    int looping; // The number of times the loop repeats
    int j;
    int count = EEPROM.read(0); // Read the first Byte of EEPROM that stores number of cards
    slot = findIDSLOT( a ); // Figure out the slot number of the card to delete
    start = (slot * 4) + 2;
    looping = ((num - slot) * 4);
    num--; // Decrement the counter by one
    EEPROM.write( 0, num ); // Write the new count to the counter
    for ( j = 0; j < looping; j++ ) { // Loop the card shift times
      EEPROM.write( start + j, EEPROM.read(start + 4 + j)); // Shift the array values to 4 places earlier in the EEPROM
    }
    for ( int k = 0; k < 4; k++ ) { // Shifting loop
      EEPROM.write( start + j + k, 0);
    }
    successDelete();
    Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("Succesfully removed ID record from EEPROM"); &__c[0];})))));
  }
}

///////////////////////////////////////// Check Bytes   ///////////////////////////////////
boolean checkTwo ( byte a[], byte b[] ) {
  if ( a[0] != __null ) // Make sure there is something in the array first
    match = true; // Assume they match at first
  for ( int k = 0; k < 4; k++ ) { // Loop 4 times
    if ( a[k] != b[k] ) // IF a != b then set match = false, one fails, all fail
      match = false;
  }
  if ( match ) { // Check to see if if match is still true
    return true; // Return true
  }
  else {
    return false; // Return false
  }
}

///////////////////////////////////////// Find Slot   ///////////////////////////////////
int findIDSLOT( byte find[] ) {
  int count = EEPROM.read(0); // Read the first Byte of EEPROM that
  for ( int i = 1; i <= count; i++ ) { // Loop once for each EEPROM entry
    readID(i); // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) { // Check to see if the storedCard read from EEPROM
      // is the same as the find[] ID card passed
      return i; // The slot number of the card
      break; // Stop looking we found it
    }
  }
}

///////////////////////////////////////// Find ID From EEPROM   ///////////////////////////////////
boolean findID( byte find[] ) {
  int count = EEPROM.read(0); // Read the first Byte of EEPROM that
  for ( int i = 1; i <= count; i++ ) { // Loop once for each EEPROM entry
    readID(i); // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) { // Check to see if the storedCard read from EEPROM
      return true;
      break; // Stop looking we found it
    }
    else { // If not, return false
    }
  }
  return false;
}

///////////////////////////////////////// Write Success to EEPROM   ///////////////////////////////////
// Flashes the green LED 3 times to indicate a successful write to EEPROM
void successWrite() {
  digitalWrite(5, 0x1); // Make sure blue LED is off
  digitalWrite(7 /* Set Led Pins*/, 0x1); // Make sure red LED is off
  digitalWrite(6, 0x1); // Make sure green LED is on
  delay(200);
  digitalWrite(6, 0x0); // Make sure green LED is on
  delay(200);
  digitalWrite(6, 0x1); // Make sure green LED is off
  delay(200);
  digitalWrite(6, 0x0); // Make sure green LED is on
  delay(200);
  digitalWrite(6, 0x1); // Make sure green LED is off
  delay(200);
  digitalWrite(6, 0x0); // Make sure green LED is on
  delay(200);
}

///////////////////////////////////////// Write Failed to EEPROM   ///////////////////////////////////
// Flashes the red LED 3 times to indicate a failed write to EEPROM
void failedWrite() {
  digitalWrite(5, 0x1); // Make sure blue LED is off
  digitalWrite(7 /* Set Led Pins*/, 0x1); // Make sure red LED is off
  digitalWrite(6, 0x1); // Make sure green LED is off
  delay(200);
  digitalWrite(7 /* Set Led Pins*/, 0x0); // Make sure red LED is on
  delay(200);
  digitalWrite(7 /* Set Led Pins*/, 0x1); // Make sure red LED is off
  delay(200);
  digitalWrite(7 /* Set Led Pins*/, 0x0); // Make sure red LED is on
  delay(200);
  digitalWrite(7 /* Set Led Pins*/, 0x1); // Make sure red LED is off
  delay(200);
  digitalWrite(7 /* Set Led Pins*/, 0x0); // Make sure red LED is on
  delay(200);
}

///////////////////////////////////////// Success Remove UID From EEPROM  ///////////////////////////////////
// Flashes the blue LED 3 times to indicate a success delete to EEPROM
void successDelete() {
  digitalWrite(5, 0x1); // Make sure blue LED is off
  digitalWrite(7 /* Set Led Pins*/, 0x1); // Make sure red LED is off
  digitalWrite(6, 0x1); // Make sure green LED is off
  delay(200);
  digitalWrite(5, 0x0); // Make sure blue LED is on
  delay(200);
  digitalWrite(5, 0x1); // Make sure blue LED is off
  delay(200);
  digitalWrite(5, 0x0); // Make sure blue LED is on
  delay(200);
  digitalWrite(5, 0x1); // Make sure blue LED is off
  delay(200);
  digitalWrite(5, 0x0); // Make sure blue LED is on
  delay(200);
}

////////////////////// Check readCard IF is masterCard   ///////////////////////////////////
// Check to see if the ID passed is the master programing card
boolean isMaster( byte test[] ) {
  if ( checkTwo( test, masterCard ) )
    return true;
  else
    return false;
}
