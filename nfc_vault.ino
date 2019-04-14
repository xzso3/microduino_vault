#include "nfc_vault.h"

MotorPlus motor(MOTOR_ADDR4);
NFC nfc;
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);

// debug mode switch;
boolean DEBUG_MODE = true;

// initialize global variable;
int VAULT_STATUS = VAULT_LOCKED;
unsigned long start_time, opened_time;

void setup()
{
  if (DEBUG_MODE) Serial.begin(115200);

  // Initialize motor, nfc, oled module.
  motor.begin();
  nfc_initialize();
  u8g.setColorIndex(1);

  // Initialize input button
  pinMode(4, INPUT);
  pinMode(12, INPUT);
  
  // Initialize communication pin
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
}

void loop()
{
  switch (VAULT_STATUS)
  {
    case VAULT_OPENED:
      vault_opened();
      break;
    case VAULT_OPENING:
      vault_opening();
      break;
    case VAULT_CLOSING:
      vault_closing();
      break;
    case VAULT_LOCKED:
      nfc_read_card();
      break;      
  }

}

// nfc module initialization;
void nfc_initialize()
{
  uint32_t versiondata = nfc.begin();

  if (!versiondata)
  {
    Serial.print("NFC MODULE NOT FOUND!");
    while(1); // halt
  }

  nfc.setPassiveActivationRetries(0xFF);

  // serial port print debug information;
  if (DEBUG_MODE)
  {
    Serial.print("Found chip PN5");
    Serial.println((versiondata>>24) & 0xFF, HEX); 
    Serial.print("Firmware ver. ");
    Serial.print((versiondata>>16) & 0xFF, DEC); 
    Serial.print('.');
    Serial.println((versiondata>>8) & 0xFF, DEC);
    Serial.println("Waiting for an ISO14443A card");
  }
}

// nfc module read card event;
void nfc_read_card()
{
  if (IS_SOMEBODY_APPROACHING)
  {
    oled_draw(VAULT_ALERT);
  }
  else
  {
    oled_draw(VAULT_LOCKED);
  }
  
  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;   

  // Load ISO14443A Cards Data.
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success)
  {
    oled_draw(CARD_OK);
    delay(1000);
    open_vault();
    // Card read....
    if (DEBUG_MODE)
    {
      Serial.println("Found a card!");
      Serial.print("UID Length: ");
      Serial.print(uidLength, DEC);
      Serial.println(" bytes");
      Serial.print("UID Value: ");
      for (uint8_t i=0; i < uidLength; i++) 
      {
        Serial.print(" 0x");
        Serial.print(uid[i], HEX); 
      }
      Serial.println("");
     }
  }
  else
  {
    // Wait for cards....
    if (DEBUG_MODE)
    {
      Serial.println("Wait for cards....");
    }
  }

}

// oled screen draw event;
void oled_draw(int opCode)
{
  // communicate with sub-processor;
  tx_msg(opCode);

  // draw event;
  u8g.firstPage();  
  do {
    u8g.setFont(u8g_font_9x15);
    u8g.drawStr(0, 10, "VAULT  NO.492V");
    u8g.drawHLine(0, 19, 128);
    switch (opCode)
    {
      case VAULT_LOCKED:
        break;
      case VAULT_OPENING:
        u8g.drawStr(0, 32, "WELCOME, USER!");
        u8g.drawStr(0, 50, "VAULT OPENING...");
        break;
      case VAULT_OPENED:
        u8g.drawStr(0, 32, "WELCOME, USER!");
        u8g.drawStr(0, 50, "VAULT OPENED...");
        break;
      case VAULT_CLOSING:
        u8g.drawStr(0, 32, "BYE");
        u8g.drawStr(0, 50, "VAULT CLOSING...");
        break;
      case VAULT_ALERT:
        u8g.drawStr(0, 32, "WELCOME, USER!");
        u8g.drawStr(0, 50, "");
        break;
      case CARD_OK:
        u8g.drawStr(0, 32, "CARD OK!");
        u8g.drawStr(0, 50, "WELCOME, USER!");
        break;  
    }
  } while( u8g.nextPage() );  
}

// commuicate with sub-processor
void tx_msg(int opCode)
{
  switch (opCode)
  {
    case VAULT_LOCKED:
      digitalWrite(7, LOW);
      digitalWrite(8, LOW);
      digitalWrite(9, LOW);
      break;
    case VAULT_OPENING:
      digitalWrite(7, LOW);
      digitalWrite(8, HIGH);
      digitalWrite(9, LOW);
      break;
    case VAULT_OPENED_NOTE:
      digitalWrite(7, LOW);
      digitalWrite(8, HIGH);
      digitalWrite(9, LOW);
      break;    
    case VAULT_OPENED:
      digitalWrite(7, LOW);
      digitalWrite(8, HIGH);
      digitalWrite(9, HIGH);
      break;
    case VAULT_CLOSING:
      digitalWrite(7, HIGH);
      digitalWrite(8, LOW);
      digitalWrite(9, HIGH);
      break;
    case VAULT_CLOSED_NOTE:
      digitalWrite(7, HIGH);
      digitalWrite(8, LOW);
      digitalWrite(9, HIGH);
      break; 
    case VAULT_ALERT:
      digitalWrite(7, HIGH);
      digitalWrite(8, HIGH);
      digitalWrite(9, LOW);
      break;
  
  }
}

void motor_event(int opCode)
{
  switch (opCode)
  {
    case VAULT_OPENING:
      motor.setSpeed2(80);
      break;
    case VAULT_CLOSING:
      motor.setSpeed2(-80);
      break;
    case DOOR_BRAKE:
      motor.setSpeed2(BRAKE);
      break;    
  }
}

void open_vault()
{
   oled_draw(VAULT_OPENING);
   VAULT_STATUS = VAULT_OPENING;
   start_time = millis();
   motor_event(VAULT_STATUS);
}

void vault_opening()
{
  oled_draw(VAULT_OPENING);
  if (IS_DOOR_MOVE_COMPLETED)
  {
    oled_draw(VAULT_OPENED);
    VAULT_STATUS = VAULT_OPENED;
    motor_event(DOOR_BRAKE);
    opened_time = millis();
  }
}

void vault_opened()
{
  oled_draw(VAULT_OPENED);
  if (IS_SOMEBODY_APPROACHING)
  {
    opened_time = millis();  
  }
  
  if (IS_REACH_OPEN_LIMIT || IS_CLOSE_BUTTON_PRESSED)
  {
    oled_draw(VAULT_CLOSING);
    VAULT_STATUS = VAULT_CLOSING;
    motor_event(VAULT_CLOSING);
    start_time = millis();
  }
}

void vault_closing()
{
  oled_draw(VAULT_CLOSING);
  if (IS_DOOR_MOVE_COMPLETED)
  {
    oled_draw(VAULT_LOCKED);
    VAULT_STATUS = VAULT_LOCKED;
    motor_event(DOOR_BRAKE);
  }
}
