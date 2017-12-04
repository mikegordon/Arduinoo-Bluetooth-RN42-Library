//
// Bluetooth Library for RN42 written by Mike Gordon (12/1/2017)
// https://github.com/      /Arduino-Bluetooth-RN42-Library
//

#ifndef Bluetooth_RN42_h
#define Bluetooth_RN42_h

#include <stdint.h>
#include <Arduino.h>
#include <Stream.h>

class Bluetooth_RN42
{
  public:
    Bluetooth_RN42(Stream &str) : 
      inCommandMode(false),
      stream(str),
      pConsoleStream(NULL),
      escapeSeqStr("%"),
      start(0),
      resp("")
    {
    };
      
    ~Bluetooth_RN42();
    
    bool    init();    
    bool    isCommandMode() { return inCommandMode; }
    String  getDisconnectStr() { return escapeSeqStr + F("DISCONNECT"); }
    bool    beginCommandMode();
    bool    beginDataMode();
    bool    isConnected();
    bool    connect();
    bool    connectToAddress(String mac_address);
    void    sendStr(String str);
    String  readStr(unsigned long timeout);
    void    attachConsole(Stream &stream) { pConsoleStream = &stream; }
      
  private:
    bool   inCommandMode;
    Stream &stream;
    Stream *pConsoleStream;
    String escapeSeqStr;
    unsigned long start;
    String resp;

    String readWithTimeout(unsigned long timeout);
    bool   beginCommandModeInternal();
    bool   beginDataModeInternal();
    bool   connectInternal(String command);
    bool   sendReceiveValidate(String sendStr, String expectedStr, unsigned long timeout);
    String sendReceive(String sendStr, unsigned long timeout);
};

#endif