#include <Microduino_MotorPlus.h>
#include <Microduino_NFC.h>
#include <U8glib.h>

// vault status code
#define VAULT_LOCKED 0
#define VAULT_OPENING 1
#define VAULT_OPENED_NOTE 2
#define VAULT_OPENED 3
#define VAULT_CLOSING 4
#define VAULT_CLOSED_NOTE 5
#define VAULT_ALERT 6

#define CARD_OK 100

#define DOOR_BRAKE -1

#define IS_CLOSE_BUTTON_PRESSED !digitalRead(4)
#define IS_DOOR_MOVE_COMPLETED  millis() > start_time + 1500
#define IS_REACH_OPEN_LIMIT  millis() > opened_time + 10000
