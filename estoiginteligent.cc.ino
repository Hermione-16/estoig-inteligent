/*
 * Copyright 2026 Adriana Montaña Samudio
 * 
 * 
 */

//Pantalla
#include <U8g2lib.h>
#include <Wire.h>
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

//Alarma
#include <EasyBuzzer.h>
#define ALARM 7

//Interruptor
#define INTERRUPTOR 8

//Targeta
#include <SPI.h>
#include <MFRC522.h>
#define RST_PIN   9  // Configurable, see typical pin layout above
#define SS_PIN   10  // Configurable, see typical pin layout above
#define IRQ_PIN   2  // Configurable, depends on hardware
MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance
MFRC522::MIFARE_Key key;
volatile bool bNewInt = false;
byte regVal = 0x7F;

const byte targetaAdriana[] = { 0x2F, 0xA2, 0x28, 0xC3}; 
bool autoritzat = false;
const int loopDelayMilis = 100;
int milisegonsAutoritzat = 0;
bool alarmaActivada = false;
bool obertDeManeraSegura = false;

void configurarLectorTargetes() {
  Serial.println("Configurant lector targetes...");
  SPI.begin();
  mfrc522.PCD_Init();
  pinMode(IRQ_PIN, INPUT_PULLUP); // Setup the IRQ pin
  // Allow the irq to be propagated to the IRQ pin
  regVal = 0xA0; // Rx IRQ
  mfrc522.PCD_WriteRegister(mfrc522.ComIEnReg, regVal);
  bNewInt = false; // Interrupt flag
  //  Activate the interrupt
  attachInterrupt(digitalPinToInterrupt(IRQ_PIN), llegirInterrupcio, FALLING);
  delay(100);
  Serial.println("Configurant lector targetes... ok.");
}


void configurarPantalla() {
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
  u8g2.begin();
}


void llegirInterrupcio(){
  bNewInt = true;  
}

void llegirTargeta(){
  if (bNewInt) {
    Serial.println(F("Interrupt."));
    
    bool hihaTargeta = mfrc522.PICC_ReadCardSerial(); // Read the tag data
    if (hihaTargeta){
      Serial.println(F("Hi ha targeta..."));
      byte *codi = mfrc522.uid.uidByte;
      bool esValida = true;  
      
      Serial.print(F("Codi UID:"));
      for(byte i = 0; i < mfrc522.uid.size; i++) {
        Serial.print(codi[i] < 0x10 ? " 0" : " ");
        Serial.print(codi[i], HEX);
        esValida = esValida && codi[i] == targetaAdriana[i];
      }
      Serial.println("");
      
      if(esValida) {
        Serial.println(F("Es valida"));
        autoritzat = true;
        alarmaActivada = false;      
      }
      else {
        Serial.println(F("No Es valida"));
      }
      
    }

    clearInt(mfrc522);
    mfrc522.PICC_HaltA();
    bNewInt = false;     
  }
  activateRec(mfrc522);
}

// The function sending to theMFRC522 the needed commands to activate the reception
void activateRec(MFRC522 mfrc522) {
  mfrc522.PCD_WriteRegister(mfrc522.FIFODataReg, mfrc522.PICC_CMD_REQA);
  mfrc522.PCD_WriteRegister(mfrc522.CommandReg, mfrc522.PCD_Transceive);
  mfrc522.PCD_WriteRegister(mfrc522.BitFramingReg, 0x87);
}
// The function to clear the pending interrupt bits after ISR
void clearInt(MFRC522 mfrc522) {
  mfrc522.PCD_WriteRegister(mfrc522.ComIrqReg, 0x7F);
}


void dibuixarPantalla() {
  u8g2.clearBuffer();
  if(autoritzat){ 
    u8g2.drawStr(0, 0, "Autoritzat");  
  }
  else {
    u8g2.drawStr(0, 0, "No autoritzat");
  }
  
  u8g2.sendBuffer();
}

void setup() {

  Serial.begin(9600);

  configurarPantalla();
  configurarLectorTargetes();
  
  pinMode(INTERRUPTOR, INPUT_PULLUP);
  pinMode(ALARM, OUTPUT);
  
} 

void beep() {
  tone(ALARM, 410, loopDelayMilis);
}

void stopBeep() {
  digitalWrite(ALARM, LOW);
}

void loop() {
  dibuixarPantalla();
  llegirTargeta();

  milisegonsAutoritzat += loopDelayMilis;
  if(milisegonsAutoritzat > 3000){
    autoritzat = false;
    milisegonsAutoritzat = 0;  
  }

  int estoigObert = digitalRead(INTERRUPTOR);
  if(estoigObert == HIGH){ 
    //estoig obert
    if(autoritzat == false) {
      if(obertDeManeraSegura == false) { 
        alarmaActivada = true; 
      } 
    }  
    else {
      //Sí autoritzat
      obertDeManeraSegura = true;
    }
  }
  else { //estoig tancat
    alarmaActivada = false; 
    obertDeManeraSegura = false; 
  }

  if(alarmaActivada){
    beep();
  }
  else{ 
    stopBeep();
  }
  
  delay(loopDelayMilis);
}
