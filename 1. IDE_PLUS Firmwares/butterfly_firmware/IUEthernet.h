#ifndef IUETHERNET_H
#define IUETHERNET_H


/**
 * Ethernet chip
 *
 * Component:
 *   Name:
 *   USR-IOT
 * Description:
 *
 */

#include <Arduino.h>
#include "Component.h"
//#include <IUSerial.h>
#include "U2E.h"
#include "IUFlash.h"

class IUEthernet : public Usr2Eth, public IUSerial,public Component
{
    public:
        
        // core
      //  IUEthernet();
      virtual  ~IUEthernet() {};   
        // IUEthernet(HardwareSerial *serialPort, char *charBuffer,
        //       uint16_t bufferSize, PROTOCOL_OPTIONS protocol, uint32_t rate,
        //       char stopChar, uint16_t dataReceptionTimeout);

        /***** Hardware and power management *****/
        virtual void setupHardware();
        
        /***** Network Configuration *****/
        void dofullConfig();
        void configEthernet();
        /***** AT Command Interface *******/
        void enterATCommandInterface();
        void exitATCommandInterface();
        
        
        /***** User Inbound communication *****/
        //void processUserMessage(char *buff, IUFlash *iuFlashPtr);
        
        /***** Guest Inbound communication *****/
        //bool processChipMessage();
        
        /***** Guest Outbound communication *****/
        
    private:
        /***** Configuring the WiFi *****/
        
        /***** Connection and auto-sleep *****/
        
        /***** Informative variables *****/
        
        /***** Callbacks *****/
        //void (*m_onConnect)() = NULL;
        //void (*m_onDisconnect)() = NULL;
};


#endif