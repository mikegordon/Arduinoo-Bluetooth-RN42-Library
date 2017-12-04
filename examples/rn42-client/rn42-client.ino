#include <Bluetooth_RN42.h>

/*
  rn42-client
  Test program for a Bluetooth RN-42 module that connect to a remote host.
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
  isConnected = bt.isConnected();
  sendAgain = true;
}

// the loop routine runs over and over again forever:
void loop() {
  if (!isInited)
    return;
    
  if (!isConnected)
  {
    Serial.println("Connecting to remote device");
    if (bt.connect())
    {
      Serial.println("Connected to remote device");
      isConnected = true;
      sendAgain = true;
    }
    else
    {
      Serial.println("Connection failed!");
      isConnected = false;
    }
  }
  else
  {
    if (sendAgain)
    {
      bt.sendStr("the quick brown fox jumped over the lazy dog.\n");
      sendAgain = false;
    }
    String response = bt.readStr(500);
    if (response.indexOf(bt.getDisconnectStr()) != -1)
    {
      Serial.println("Connection lost");
      bt.beginCommandMode();
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