#include <Wire.h>
#include <LiquidCrystal.h>
#include <SPI.h>
#include <SD.h>
#include <avr/io.h>
#include <avr/interrupt.h>


// pin select for lcd:
const uint8_t rs = 4, en = 5, d4 = 6, d5 = 7, d6 = 8, d7 = 9;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// consts for SD card:
File dataFile;
String fileName = "TEST.bin";
const uint8_t CS_PIN = 10; // pin select variable
byte lsb,msb;
const uint16_t blockSize = 128; // sd card buffer variable
uint16_t YData[blockSize];
String text1, text2;
long fileSize;
bool hasFile = false;

// consts for timer1:
bool debugging = true;
uint16_t counterTopValue = 31249;  // sætter maxværdien i OCR1A-registeret ved 500Hz
byte prescalerBits = 0;

// consts for ADC:
int analogPin = 5;
uint8_t prescalerADC = 3;

// Initializing functions reference
void setupLCD(void);
void writeLCD(String, String);
void writeLCD_TOP(String);
void writeLCD_BOT(String);
void setupSD(int, String);
// void startTimer1(void);

// debugging vals and lcd printout variables
int lcd_val;

void setup(void) {
  // Serial.begin(9600); delay(1000); Serial.println("test");
  setupLCD();
// LCD setup Done
  
  setupSD(CS_PIN, fileName);
// SD setup Done

writeLCD_TOP("in Timer:");

  setupTimer1(); // setting up timer
// sprintf("")
if(int(prescalerBits) >= 3) {lcd_val = 64}
writeLCD_BOT("Prescaler: " + String(prescalerBits));

  // Timer1 setup done

writeLCD_TOP("Setup ADC:");
setupADC();
writeLCD_BOT("ADC prescaler: " + String(prescalerADC));

if (hasFile){
  dataFile = SD.open(fileName, FILE_WRITE);
  startTimer1();
} else {
  writeLCD("Issue with file", "No location found");
  Serial.print("Issue with file location");
  while(1) {}
}
  // for (int i = 0; i < blockSize; i++) {
  //   YData[i] = i;
  // }
  // Serial.print("Size of YData : ");
  // Serial.println(sizeof(YData));
  // dataFile = SD.open(fileName, FILE_WRITE);
  // if (dataFile){
  //   writeLCD_TOP("Write to ");
  //   writeLCD_BOT(fileName);
  //   // Write entire array as one block
  //   dataFile.write((const uint8_t *)&YData, sizeof(YData));   // Binary file
  //   fileSize = dataFile.size();
  //   writeLCD("File size",String(fileSize));          
  // }
  // dataFile.close();         // Close data file after writing is done 

  // // Now we read the data from SD card one byte at a time

  // dataFile = SD.open(fileName, FILE_READ);
  // fileSize = dataFile.size();
  // if (dataFile){
  //    writeLCD("File size",String(fileSize));
  //    writeLCD("Read data"," ");
  //    int i = 0;
  //    while(dataFile.available()){
  //      lsb = dataFile.read();  // Read lsByte
  //      msb = dataFile.read();  // Read msByte
  //      if (i < blockSize){
  //        YData[i] = (msb<<8) + lsb; // Join bytes to an int
  //        i++;
  //      }
  //      else{
  //       Serial.println("Data exceed blockSize");
  //       while(1);
  //      }
  //    }
  //   for (int i = 0; i < blockSize; i++){
  //    Serial.println(YData[i]);
  //   } 
  // }
  // else{
  //   writeLCD("File reopen","failed");
  // }
  writeLCD_TOP("Job done");
  writeLCD_BOT("Asta la vista");
}

void loop(void) {
  // put your main code here, to run repeatedly:

}

// function that sets bytes, (REGISTER, "01010110")
uint8_t editbyte(uint8_t byte_input, char string_input[8]){

  for(int i = 0; i < 8; i++) {
    
    int j = 7 - i; // 2nd iterator so the input matches with the register
    int compare = (int)string_input[i] - 48; //converts char variable to int, the char '0' is equal to 48, and '1' 49
    
    if (compare == 0) {
      byte_input &=~(1<<j);
    } else {
      byte_input |= (1<<j);
    }
  }
  return byte_input;
}

void printlnBits(byte value) {
  for (int i = 7; i >= 0; i--) {
    Serial.print((value >> i) & 1);
  }
  Serial.println();
}

//////////////////// START OF LCD SETUP ////////////////////////

void setupLCD(void){
  lcd.begin(16,2);
  lcd.clear();
  writeLCD("LCD", "initiated");
}

void writeLCD(String text1, String text2){
  lcd.clear();
  lcd.setCursor(0,0);    lcd.print(text1); 
  lcd.setCursor(0,1);    lcd.print(text2);
  delay(2000); 
}
void writeLCD_TOP(String text1){
  lcd.clear();
  lcd.setCursor(0,0);    lcd.print(text1);
  delay(2000); 
}
void writeLCD_BOT(String text2){
  lcd.setCursor(0,1);    lcd.print(text2);
  delay(2000); 
}

//////////////////// END OF LCD SETUP /////////////////////////

///////////////////////////////////////////////////////////////

//////////////////// START OF SD SETUP ////////////////////////

void setupSD(int CS_PIN, String fileName){ 
  //CS pin must be configured as an output
  pinMode(CS_PIN, OUTPUT);
  if (!SD.begin(CS_PIN)){
    writeLCD_BOT("Card Failure");
    while(1);
  }
  writeLCD_TOP("in setupSD()");
  writeLCD_BOT("Card Ready"); 
  text1 = "CS_PIN = "+ String(CS_PIN);
  writeLCD_BOT(text1);
  // If file exist, delete it
  if (SD.exists(fileName)){
    writeLCD_BOT("file exists"); 
    SD.remove(fileName);
    writeLCD_BOT("file deleted");
  }
  dataFile = SD.open(fileName, FILE_WRITE);
  delay(100);
  // Test if file can open, then close it
  if (dataFile){          
     dataFile.close();     
     writeLCD("new file",fileName);
  }
  else{
     writeLCD_BOT("Open failed");
     while(1);
  }
  hasFile = true; // bool that ensures the file is there

}
//////////////////// END OF SD SETUP ////////////////////////

////////////////////////////////////////////////////////////////

//////////////////// START OF TIMER SETUP ////////////////////////

void setupTimer1(void) {
  cli();  // Deaktiver global interrupts
  
  TCCR1A = 0;  // Normal mode for TCCR1A
  TCCR1B = 0;  // Clear TCCR1B registret
  TCCR1B |= (1 << WGM12);  // CTC mode aktiveres ved at skrive "1" til "WGM12"
  OCR1A = counterTopValue;  // Sætter OCR1A
  OCR1B = counterTopValue;  // Match OCR1B
  prescalerBits |= (1 << CS11) | (1 << CS10); // Prescaler = 64

  TIMSK1 |= (1 << OCIE1B);  // Aktiverer Compare B Match interrupt
  TCNT1 = 0;  // vi nulstil tæller

  sei();  // Aktiver global interrupts

}

void startTimer1(void) {
  TCCR1B |= prescalerBits;  // Start Timer med prescaler
  
  if (debugging) {
    Serial.print("TCCR1B \t = 0b"); 
    printlnBits(TCCR1B);
  }

}

void printTimer1(void) {
  Serial.println("Timer1 Config:");
  Serial.print("OCR1A \t = "); Serial.println(OCR1A);
  Serial.print("Prescaler Bits: "); printlnBits(prescalerBits);

}
//////////////////// END OF TIMER SETUP ////////////////////////

////////////////////////////////////////////////////////////////

//////////////////// START OF ADC SETUP ////////////////////////
void setupADC(void) {
  cli();
  ADMUX = 0;
  ADMUX |= (1 << REFS0);  // Vi sætter referencespædning til ADMUX til pin A0
  ADMUX |= analogPin;
  ADCSRA = (1 << ADPS2) | (1 << ADPS1);
  prescalerADC = (1 << ADPS2) | (1 << ADPS1);
  ADCSRA |= (1 << ADEN) | (1 << ADATE) | (1 << ADIE);   // vælger vores prescaler 64
  
  ADCSRB = (1 << ADTS2) | (1 << ADTS0);  // vi vælger hvilken trigger soruce ADC skal udløse på. I dette tilfælde er det CTC mode for
   // timer B.
  
  // Deaktiver digital input på A0 for at spare strøm
  DIDR0 = (1 << ADC0D);
  
  // Start ADC med Auto Trigger
  ADCSRA |= (1 << ADSC);
  sei();
}
//////////////////// END OF ADC SETUP ///////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////

//////////////////// START OF Interrupt Service routines ////////////////////////

ISR(TIMER1_COMPB_vect) {
//   if (debugging) {

//     // Serial.println("Timer1 interrupt triggered.");
//   }
}

ISR(ADC_vect) {
  uint16_t adcValue = ADC;  // Læs ADC-data
  // Behandl eller gem adcValue
}

//////////////////// END OF Interrupt Service routines ////////////////////////
