//
// Bluetooth Library for RN42 written by Mike Gordon (12/1/2017)
// https://github.com/      /Arduino-Bluetooth_RN42-Library
//

#include "Bluetooth_RN42.h"

// RN-42 Bluetooth libary
// Assumptions:
//   Serial device is already configured for data rate.  This library does not modify the data rate.
//   From my experience, 115.2K isn't reliable, therefore it is recommended to set the baud rate in 
//   firmware to something rock solid.
//
//   Library assumes device is in slave mode.  Slave mode allows inbound and outbound connections
//   with remote devices.  Once a connection is established, it automatically switches to data mode.
//
//   For device status messages like connect/disconnect the escape sequence must be defined.  If not defined,
//   the init function will define it as a single percent '%' character and save to NVRAM.


//
// public functions
//
Bluetooth_RN42::~Bluetooth_RN42(){}

//
// Inits the firmware to provide the functionality the library implements.  Certain registers must
// be set for proper functionality.  This routine should be called in setup() before using the other
// library functions to ensure correct firmware configuration.
// Returns - true if init functions successful, false if unknown
//
bool Bluetooth_RN42::init()
{
    // Verify extended status string
    inCommandMode = false;  // assume device came up in data mode

    bool result = this->beginCommandMode();
    if (!result) {
        return result;
    }
    
    // Command mode, check status string
    resp = this->sendReceive(F("GO\n"), 200);
    resp.trim();
    if (resp == F("NULL"))  // check for not-defined value
    {
        if (pConsoleStream) {
            pConsoleStream->println(F("Warning: extended status string undefined.  Setting to default value."));
        }
        
        if (!this->sendReceiveValidate(F("SO,%\n"), F("AOK"), 200)) {
            if (pConsoleStream) {
                pConsoleStream->println(F("ERROR: setting extended status string failed?"));
            }
            return false;
        }
        
        resp = sendReceive(F("GO\n"), 200);
        resp.trim();
        if (resp != "%") {
            if (pConsoleStream)
                pConsoleStream->println(F("ERROR: querying extended status string failed"));
            return false;
        }
        
        if (this->sendReceiveValidate(F("R,1\n"), F("Reboot!"), 200)) {
            inCommandMode = false;
            return true;
        }
        
        return false;
    }
    else if (resp != "%") {  // check for non-default value
        if (pConsoleStream) {
            pConsoleStream->println(F("Warning: extended status string using non-default value."));
        }
        
        if (resp.length() >= 1) {
           escapeSeqStr = resp;
           if (pConsoleStream) {
               pConsoleStream->println("Warning: escape sequence changed to: " + escapeSeqStr);
           }
        }
    }

    return true;    
}

//
// Enter into command mode and if not successful, tries to recover.
// Returns - true if command mode verified, false if unknown
// 
bool Bluetooth_RN42::beginCommandMode() 
{
    if (inCommandMode && pConsoleStream) {
        pConsoleStream->println(F("Warning: entering command mode while device is in command mode."));
    }

    bool result = this->beginCommandModeInternal();
    if (result) {
        return true;
    }

    if (pConsoleStream) {
        pConsoleStream->println(F("ERROR: unsuccessful entering into command mode.  Maybe in command mode already?"));
    }
    
    // Assume we are in command mode, attempt to recover
    // send newline so device thinks $$$ is a command error, expect a '?' result
    if (this->sendReceiveValidate(F("\n"), F("?"), 200)) {
        inCommandMode = true;
        return true;
    }
  
    if (pConsoleStream) {
        pConsoleStream->println(F("ERROR: unsuccessful entering into command mode.  Check baud rate and circuit wiring."));
    }
    return false;  
}

//
// Exit command mode for data mode and if not successful, tries to recover.
// Returns - true if data mode verified, false if unknown
// 
bool Bluetooth_RN42::beginDataMode() 
{
    if (!inCommandMode && pConsoleStream) {
        pConsoleStream->println(F("Warning: exiting mode while device is not in command mode."));
    }

    bool result = beginDataModeInternal();
    if (result) {
        return true;
    }

    if (pConsoleStream) {
        pConsoleStream->println(F("ERROR: unsuccessful entering into data mode.  Maybe in data mode already?"));
    }

    // Assume we are in data mode, attempt to recover
    result = this->beginCommandModeInternal();
    if (result) {
        if (pConsoleStream) {
            pConsoleStream->println(F("INFO: successful entering command mode.  Attempting to exit command mode"));
        }
        return this->beginDataModeInternal();
    }

    return false;  
}

//
// Checks to see if a remote device is connected.  The device will be placed into command mode
// and queried for a current connection.  The module will be in command mode after the function returns.
// Returns - true if remote device is connected, false if not
//
bool Bluetooth_RN42::isConnected() 
{
    if (!inCommandMode && pConsoleStream) {
        pConsoleStream->println(F("Warning: attempting to query connect status while not in command mode."));
    }
      
    if (this->sendReceiveValidate(F("GK\n"), F("1,0,0"), 200)) {
        inCommandMode = true;
        return true;
    }
  
    // Assume we are in data mode, attempt to recover
    bool result = this->beginCommandModeInternal();
    if (result) {
        if (pConsoleStream) {
            pConsoleStream->println(F("INFO: successful entering command mode.  Attempting to query connect status"));
        }
        if (this->sendReceiveValidate(F("GK\n"), F("1,0,0"), 200)) {
            return true;
        }
    }

    return false;
}

//
// Connects to the last remote device that connected successfully.  If there is no remote device 
// address stored, the command will fail.
// Returns - true if connect succeeds, false otherwise
//
bool Bluetooth_RN42::connect()
{
    if (!inCommandMode && pConsoleStream) {
        pConsoleStream->println(F("Warning: attempting to connect while not in command mode."));
    }
    return this->connectInternal(F("C\n"));
}

//
// Connects to the remote device with the specified MAC address
// Returns - true if connect succeeds, false otherwise
//
bool Bluetooth_RN42::connectToAddress(String macAddress)
{
    if (!inCommandMode && pConsoleStream) {
        pConsoleStream->println(F("Warning: attempting to connect while not in command mode."));
    }

    return this->connectInternal("C," + macAddress + F("\n"));
}

//
// Sends specified string to remote device
//
void Bluetooth_RN42::sendStr(String str)
{
    if (inCommandMode && pConsoleStream) {
        pConsoleStream->println(F("Warning: attempting to send data while device is in command mode."));
    }

    stream.print(str);
}

//
// Reads received charaters from the remote device until the specified timeout period
// Returns - string of characters received
//
String Bluetooth_RN42::readStr(unsigned long timeout)
{
    if (inCommandMode && pConsoleStream) {
        pConsoleStream->println(F("Warning: attempting to receive data while device is in command mode."));
    }

    resp = this->readWithTimeout(timeout);
    if (resp.indexOf(escapeSeqStr + F("DISCONNECT")) != -1 && pConsoleStream) {
        pConsoleStream->println(F("ERROR: readStr failed because remote device disconnected"));
    }
    return resp;
}


//
// private functions
//
bool Bluetooth_RN42::beginCommandModeInternal()
{
    if (this->sendReceiveValidate(F("$$$"), F("CMD"), 200)) {
        inCommandMode = true;
        return true;
    }
    return false;
}    

bool Bluetooth_RN42::beginDataModeInternal()
{
    if (this->sendReceiveValidate(F("---\n"), F("END"), 200)) {
        inCommandMode = false;
        return true;
    }
    return false;
}    

bool Bluetooth_RN42::connectInternal(String command)
{
    if (this->sendReceiveValidate(command, F("TRYING"), 200)) {
        // wait for a {ESC SEQ}CONNECT message or "failed"
        for (int i = 0; i < 10; i++) {
            resp = this->readWithTimeout(500);
            resp.trim();
            
            if (resp.indexOf("failed") != -1) {
                inCommandMode = true;
                return false;
            }
            else if (resp.indexOf(escapeSeqStr + F("CONNECT,")) != -1) {
                inCommandMode = false;
                return true;
            }
        }
    }
    return false;
}    

// Internal function to send a string and return a bool whether the response matches an expectedStr
// NOTE: Will delay processing by number of milliseconds specified by timeout while reading the response.
bool Bluetooth_RN42::sendReceiveValidate(String sendStr, String expectedStr, unsigned long timeout)
{
    resp = sendReceive(sendStr, timeout);
    if (resp.indexOf(expectedStr) != -1) {
        return true;
    }
    return false;
}

String Bluetooth_RN42::sendReceive(String sendStr, unsigned long timeout)
{
    stream.print(sendStr);
    resp = readWithTimeout(timeout);
    return resp;
}

String Bluetooth_RN42::readWithTimeout(unsigned long timeout)
{
    resp = "";

    start = millis();
    Serial.flush();
    while(start + timeout > millis()) {
        while(stream.available()) {
            resp.concat(char(stream.read()));
        }
    }

    return resp;
}