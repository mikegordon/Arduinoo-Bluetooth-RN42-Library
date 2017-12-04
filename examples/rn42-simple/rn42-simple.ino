#include <Bluetooth_RN42.h>

/*
  rn42-simple
  Test program for a Bluetooth RN-42 module that monitors for connects and disconnects.
*/

Bluetooth_RN42 bt(Serial1);
bool isConnected;
bool sendAgain;
bool isInited;

// the setup routine runs once when you press reset:
void setup() { 
  Serial.begin(115200); // Debug console
  Serial1.begin(9600);  // Bluetooth RN-42 running at 9600 baud for reliability
  delay(5000);

  bt.attachConsole(Serial);
  isInited = bt.init();
  isConnected = false;
}

// the loop routine runs over and over again forever:
void loop() {
  if (!isInited)
    return;
    
  if (!isConnected)
  {
    bool tempConnected = bt.isConnected();
    if (isConnected != tempConnected)
    {
      isConnected = tempConnected;
      if (isConnected)
      {
        Serial.println(F("Connected to remote device"));
        bt.beginDataMode();  // enter data mode
        sendAgain = true;
      }
      else
        Serial.println(F("Remote device disconnected"));
    }
  }
  else
  {
    if (sendAgain)
    {
      bt.sendStr(F("the quick brown fox jumped over the lazy dog.\n"));
      sendAgain = false;
    }
    String response = bt.readStr(500);
    if (response.indexOf(bt.getDisconnectStr()) != -1)
    {
      Serial.println(F("Connection lost"));
      isConnected = false;
    }
    else
    {
      if (response.length() > 0)
      {
        Serial.print(response);
        sendAgain = true;
      }
    }
  }
}