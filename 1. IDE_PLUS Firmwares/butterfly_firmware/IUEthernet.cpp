#include "IUEthernet.h"


// IUEthernet::IUEthernet(HardwareSerial *serialPort, char *charBuffer,
//              uint16_t bufferSize, IUSerial::PROTOCOL_OPTIONS protocol,
//              uint32_t rate, char stopChar, uint16_t dataReceptionTimeout) :
//     IUSerial(serialPort, charBuffer, bufferSize, protocol, rate,
//              stopChar, dataReceptionTimeout)
// {
//     //debugPrint("IUEthernet Constructor init...");
// }


//IUEthernet::IUEthernet(){
//}
/*
    Ethernet setupHardware
    return - none

*/

void IUEthernet::setupHardware(){
    
   //debugPrint("Ethernet setupHardware in Process",true); 
   //setup the Serial
   //begin();
   enterATCommandInterface();
   delay(5000);
   exitATCommandInterface();

}

void IUEthernet::dofullConfig(){

}

void IUEthernet::configEthernet(){

}
/**
 * @brief 
 * 
 */
void IUEthernet::enterATCommandInterface(){
     //set AT CommandMode
     bool result = SetAT();
     debugPrint("AT MODE RESULT:",false);
     debugPrint(result);

}

void IUEthernet::exitATCommandInterface(){
    // exit from AT MODE
    bool result = ExitAT();
    debugPrint("Exit AT Mode :",false);
    debugPrint(result);

}


