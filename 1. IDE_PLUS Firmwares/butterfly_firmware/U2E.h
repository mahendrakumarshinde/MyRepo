
#ifndef Usr2Eth_h
#define Usr2Eth_h
#include "Arduino.h"
#include<IUSerial.h>
#include "Component.h"

#define BUFFER_RESERVE_MEMORY	    255
#define DEFAULT_BAUD_RATE		      115200
#define TIME_OUT_READ_SERIAL	    5000
#define RESET_PIN                 13

//#define Macro(x)  (((x) > (10)) ? (abc()) : 2)
class Usr2Eth : public IUSerial,public Component
{
  private:
    int NumofRetry = 8;
    int RetryDelay = 100;  //in ms
    int MAX_Count = 10;
    uint32_t _baud;
    String _buffer;
    bool _sleepMode;
    String _readSerial();
    String _readSerial(uint32_t timeout);
    HardwareSerial *_Serial = port;
    /***** Callbacks *****/
    void (*m_onConnect)() = NULL;
    void (*m_onDisconnect)() = NULL;
    // General Variables
    bool isheartbeatEnabled = true;
        
  public:
    //Usr2Eth();setupHardware
    Usr2Eth(HardwareSerial *serialPort, char *charBuffer,
              uint16_t bufferSize, PROTOCOL_OPTIONS protocol, uint32_t rate,
              char stopChar, uint16_t dataReceptionTimeout);
    int _timeout;
    void SelectSerial(HardwareSerial *theSerial);
    //void begin();					//Default baud 115200
    //void begin(uint32_t baud);
    virtual void setupHardware();
    /***** Network Configuration *****/
    void dofullConfig();
    void configEthernet();
    
    /***** Network Commands ******/
    bool SetAT();
    bool Echo(const char* stat);
    String GetMAC();
    bool SetMAC(const char* usrmac);
    bool ExitAT();
    bool UARTClearBuff(const char* stat);
    bool Restart();
    String FirmVer();
    bool Restore();
    bool SetUART();
    bool SetUART(uint32_t bauduart);
    bool SetUART(uint32_t bauduart, int data_bits, int stop_bits, const char* parity, const char* flow_control);
    String NetworkConfig();
    bool SetDNS(const char* DNS);
    String getDNS();
    String NetworkConfig(const char* IP_Address, const char* Mask, const char* Gateway);
    bool SocketConfig(const char*  Protocol, const char* Remote_IP, uint16_t Remote_Port);
    bool TCPStatus();
    bool PostHTTP(const char*  URL);
    bool PostHTTP(const char*  URL, const char* HTP_HEAD);
    bool GetHTTP(const char*  URL);
    bool GetHTTP(const char*  URL, const char* HTP_HEAD);
    bool HeartTime(uint16_t heart_time);
    bool EnableHeart(const char* _status);
    bool HeartStatus();
    bool HeartDirection(const char* _status);
    bool HeartData(const char* data);
    bool SetPacketIdentity(const char* Packet_status);
    bool SendingMethodPacketIdentity(const char* Method);
    bool UserEditablePacketIdentity(const char* Packet_data);
    String PacketIdentityStatus();
    bool SetUserPassword(const char* username, const char* password);
    String ReadUsernamePassword();
    String SerialRead();
    void HardReset();
    void Retry(uint16_t NumofRetrys, uint16_t RetryDelays);

    void setOnConnect(void (*callback)()) { m_onConnect = callback; }
    void setOnDisconnect(void (*callback)()) { m_onDisconnect = callback; }

    virtual bool readMessages();
    String getServerConfiguration();
    bool updateNetworkMode(String serverIP,uint16_t port);
    bool controlhttpHeaderResponse(const char* _status);
   // Variable Member
   bool isEthernetConnected = true;
   String m_ethernetMacAddress;
   const char* m_workMode;
   const char* m_remoteIP;
   uint16_t m_remotePort;
   //heartbeat Configuration
   //"hearbeatInterval":5,"heartbeatDir":"NET","heartbeatMsg":"Tigar Zinda hai !!!"}
   const char* m_enableHeartbeat = "ON";
   const char* m_heartbeatDir = "COM";
   uint16_t m_heartbeatInterval = 5;    // Default 5 sec
   const char* m_heartbeatMsg = "abc"; 
   // web credentials
   const char* m_username = "admin";    //default username
   const char* m_password = "iut";      // default password
   
   // Default server details
   const char* m_defaultConfigDomain = "onprem-configs.iu";
   uint16_t m_defaultPort = 8000;
   const char* m_defaultURL = "/iu/configs";

   // Timeout Variables
   long m_atTimeout = 30000;  // sec
   uint32_t m_ConnectionTimeout=  30000;
   uint32_t m_lastDone = 0;

   bool responseIsNotAvailabel = false;
   bool m_exitATMode = false;  
   bool m_enterATMode = false;
};

#endif
