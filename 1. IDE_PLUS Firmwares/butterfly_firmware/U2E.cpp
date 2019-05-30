#include "Arduino.h"
#include "U2E.h"

/**
 * @brief Construct a new Usr 2 Eth:: Usr 2 Eth object
 * 
 */
// 
Usr2Eth::Usr2Eth(HardwareSerial *serialPort, char *charBuffer,
             uint16_t bufferSize, IUSerial::PROTOCOL_OPTIONS protocol,
             uint32_t rate, char stopChar, uint16_t dataReceptionTimeout) :
    IUSerial(serialPort, charBuffer, bufferSize, protocol, rate,
             stopChar, dataReceptionTimeout)
{
    debugPrint("IUEthernet Constructor init...");
}

/**
 * @brief :setup the U2E hardware 
 * 
 */
void Usr2Eth::setupHardware(){

  begin();
  SetAT();
  delay(1000);
  dofullConfig();
  
  delay(2000);
  //ExitAT();
  Restart();

}

void Usr2Eth::dofullConfig(){
  // 
    _Serial->flush();
    SetAT();
    //clear UART Buffer
    bool uartbuff = UARTClearBuff("on");
    Serial.print("UART Buffer clear:");
    Serial.println(uartbuff);

    //configure the UART
    bool setuart = SetUART(DEFAULT_BAUD_RATE,8,1,"NONE","NFC");
    Serial.print("Set UART : ");
    Serial.println(setuart);
    //get macid
    m_ethernetMacAddress = GetMAC();
    debugPrint("Ethernet macid:",false);
    debugPrint(m_ethernetMacAddress,true);
  
    //get Firmware Version
    String Version = FirmVer();
    Serial.print("Version : ");Serial.println(Version);
    //Check TCPStatus
    isEthernetConnected = TCPStatus();
    Serial.print("TCP Status :");Serial.println(isEthernetConnected);
    
    //configure the network for DHCP
    String ip = NetworkConfig();
    Serial.print("DHCP IP :");
    Serial.println(ip);
    //Configure module to TCP Client Mode (SocketConfig)
    //bool isSocketSet = SocketConfig("TCPC","192.168.0.5",8090);            //need to be configurable from USB
    Serial.print("workMode 1: ");Serial.println(m_workMode);
    Serial.print("Remote_IP 1: ");Serial.println(m_remoteIP);
    Serial.print("Remote Port 1:");Serial.println(m_remotePort);
    bool isSocketSet = SocketConfig(m_workMode,m_remoteIP,m_remotePort);            //need to be configurable from USB
    
    Serial.print("Socket Config :");
    Serial.println(isSocketSet);
    //Configure the HeartDirection
    HeartDirection("COM");  // to Remote_IP
    HeartTime(5); // every 5 sec
    HeartData("{thresholds ---->vikas};");
    //exit ATCommand Mode
    ExitAT();
    

}
/**
 * @brief 
 * 
 * @param theSerial 
 */
void Usr2Eth::SelectSerial(HardwareSerial *theSerial)
{
  _Serial = theSerial;
}

/**
 * @brief 
 * 
 */
// void Usr2Eth::begin()
// {
//   _baud = DEFAULT_BAUD_RATE;			// Default baud rate 115200
//   _Serial->begin(_baud);
// }

/**
 * @brief 
 * 
 * @param NumofRetrys 
 * @param RetryDelays 
 */
void Usr2Eth::Retry(uint16_t NumofRetrys, uint16_t RetryDelays)
{
  NumofRetry = NumofRetrys;
  RetryDelay = RetryDelays;

}
/**
 * @brief 
 * 
 * @param baud 
 */
// void Usr2Eth::begin(uint32_t baud)
// {
//   _baud = baud;
//   _Serial->begin(_baud);
// }

/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
bool Usr2Eth::SetAT()
{
 
  //_Serial->print(F("+++"));
  port->write("+++");
  //debugPrint("+++",true);
  while (_readSerial().indexOf("a") == 0)
  {
    //_Serial->print(F("a"));
    port->write("a");
    _buffer = _readSerial(100);
  }

  if (_buffer.indexOf("+ok") == -1 )
  {
    debugPrint("SetAT Failed:",true);
    //debugPrint(true,true);
    return  true;
  }
  else{
    debugPrint("SetAT Sucess",true);
    //debugPrint(false,true);
    return  false;
  }
}
/**
 * @brief 
 * 
 * @param stat 
 * @return true 
 * @return false 
 */
bool Usr2Eth::Echo(const char* stat)
{
  int count = 0;
  do {
    if (strcmp(stat, "on") == 0 || strcmp(stat, "off") == 0 || strcmp(stat, "ON") == 0 || strcmp(stat, "OFF") == 0)
    {
      _Serial->print(F("AT+E="));
      _Serial->print(stat);
      _Serial->print(F("\r\n"));

      _buffer = _readSerial();
      count++;
      delay(RetryDelay);
    }
  }
  while ((count < NumofRetry && count < MAX_Count)&& _buffer.indexOf("+OK") == -1);
  {
    if ( (_buffer.indexOf("+OK")) == -1)
    {
      return true;
    }
    else
      return false;
  }
}

/**
 * @brief 
 * 
 * @param stat 
 * @return true 
 * @return false 
 */
bool Usr2Eth::UARTClearBuff(const char* stat)
{
  int count = 0;
  do {
    if (strcmp(stat, "on") == 0 || strcmp(stat, "off") == 0 || strcmp(stat, "ON") == 0 || strcmp(stat, "OFF") == 0)
    {
      _Serial->print(F("AT+UARTCLBUF="));
      _Serial->print(stat);
      _Serial->print(F("\r\n"));

      _buffer = _readSerial();
      count++;
      delay(RetryDelay);
    }
  }
  while ((count < NumofRetry && count < MAX_Count)&& _buffer.indexOf("+OK") == -1);
  if ( (_buffer.indexOf("+OK")) == -1)
    {
      return true;
    }
    else{
      
      return false;
  }
}


/**
 * @brief 
 * 
 * @return String 
 */
String Usr2Eth::GetMAC()
{
  String mac = "";
  uint8_t indexOne;
  uint8_t indexTwo;
  String macid;
  delay(100);
  int count = 0;
  do {
    /*_Serial*/port->print(F("AT+MAC\r\n"));

    mac = _readSerial(100);
    count++;
    delay(RetryDelay);
  }

  while ( (count < NumofRetry && count < MAX_Count) && mac.indexOf("+OK") == -1);
  {

    if (mac.indexOf("+OK") == -1)
    {
      debugPrint("Get MAC Failed",true);
      return "ERROR";

    }
    else
    {
      debugPrint("Constructing MAC ID",true);
      indexOne = mac.indexOf("+OK=") + 4;
      indexTwo = mac.indexOf(("\r"), indexOne);
      mac = mac.substring(indexOne, indexTwo);
      for (int i = 0; i < 12; i = i + 2)
      {
        String mac2 = mac.substring(i, i + 2);
        if (i < 10)
        {
          macid += mac2 + ":";
        }
        else
        {
          macid += mac2;
        }
      }
      return macid;

    }
  }
}

/**
 * @brief 
 * 
 * @param usrmac 
 * @return true 
 * @return false 
 */
bool Usr2Eth::SetMAC(const char* usrmac)
{

  int count = 0;
  do {
    _Serial->print(F("AT+USERMAC="));
    _Serial->print(usrmac);
    _Serial->print(F("\r\n"));
    _buffer = _readSerial(100);
    count++;
    delay(RetryDelay);
  }

  while ((count < NumofRetry && count < MAX_Count)&& _buffer.indexOf("+OK") == -1);
  {   // why ??
    if (_buffer.indexOf("+OK") == -1)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
}

/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
bool Usr2Eth::ExitAT()
{
  int count = 0;
  do {
    _Serial->print(F("AT+ENTM\r\n"));
    //port->write("AT+ENTM\r\n");
    debugPrint("Executing ExitAT()..",true);
    _buffer = _readSerial(300);
    count++;
    delay(RetryDelay);
    debugPrint("Retry Count:",false);
    debugPrint(count,true);
  }
  while ((count < NumofRetry && count < MAX_Count)&& _buffer.indexOf("+OK") == -1);
  { //why ??
    if (_buffer.indexOf("+OK") == -1 )
    {
      debugPrint("Return Failed",true);
      return  true;
    }
    else{
      debugPrint("Return Sucess",true);
      return false;
    }
  }
}


/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
bool Usr2Eth::Restart()
{
  int count = 0;
  do {
    _Serial->print(F("AT+Z\r\n"));
    _buffer = _readSerial(100);
    count++;
    delay(RetryDelay);
  }

  while ((count < NumofRetry && count < MAX_Count)&& _buffer.indexOf("+OK") == -1);
  {   //why ??
    if (_buffer.indexOf("+OK") == -1 )
    {
      return  true;
    }
    else
      return  false;
  }
}

/**
 * @brief 
 * 
 * @return String 
 */
String Usr2Eth::FirmVer()
{
  uint8_t indexOne;
  uint8_t indexTwo;
  String F_Ver;
  int count = 0;
  do {
    _Serial->print(F("AT+VER\r\n"));
    F_Ver = _readSerial(100);
    count++;
    delay(RetryDelay);
  }

  while ((count < NumofRetry && count < MAX_Count)&& F_Ver.indexOf("+OK") == -1);
  { //why ??
    if (F_Ver.indexOf("+OK") == -1 )
    {
      debugPrint("Version Read Failed",true);
      return "ERROR";
    }
    else
    {
      debugPrint("Version Read Sucess",true);
      indexOne = F_Ver.indexOf("OK=") + 3;
      indexTwo = F_Ver.indexOf("\r", indexOne);
      return (F_Ver.substring(indexOne, indexTwo));
    }
  }
}

/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
bool Usr2Eth::Restore()
{
  int count = 0;
  do {
    _Serial->print(F("AT+RELD\r\n"));
    _buffer = _readSerial(100);
    count++;
    delay(RetryDelay);
  }

  while ((count < NumofRetry && count < MAX_Count)&& _buffer.indexOf("+OK") == -1);
  {  //why ??
    if (_buffer.indexOf("+OK") == -1 )
    {
      return  true;
    }
    else
      return  false;
  }
}

/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
bool Usr2Eth::SetUART()
{
  int count = 0;
  do {
    _Serial->print(F("AT+UART=115200,8,1,NONE,NFC\r\n"));
    _buffer = _readSerial();
    count++;
    delay(RetryDelay);
  }

  while ((count < NumofRetry && count < MAX_Count)&& _buffer.indexOf("+OK") == -1);
  {
    if (_buffer.indexOf("+OK") == -1)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
}

/**
 * @brief 
 * 
 * @param bauduart 
 * @return true 
 * @return false 
 */
bool Usr2Eth::SetUART(uint32_t bauduart)
{
  int count = 0;
  do {
    if (bauduart >= 600 && bauduart <= 230400)
    {
      _Serial->print(F("AT+UART="));
      _Serial->print((bauduart));
      _Serial->print(F(",8,1,NONE,NFC\r\n"));
    }
    else
    {
      return true;
    }
    _buffer = _readSerial();
    count++;
    delay(RetryDelay);
  }

  while ((count < NumofRetry && count < MAX_Count)&& _buffer.indexOf("+OK") == -1);
  {
    if (_buffer.indexOf("+OK") == -1)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
}

/**
 * @brief 
 * 
 * @param bauduart 
 * @param data_bits 
 * @param stop_bits 
 * @param parity 
 * @param flow_control 
 * @return true 
 * @return false 
 */
bool Usr2Eth::SetUART(uint32_t bauduart, int data_bits, int stop_bits, const char* parity, const char* flow_control)
{
  int count = 0;
  do {
    if ((bauduart >= 600 && bauduart <= 230400) || (data_bits >= 5 && data_bits <= 8) || (stop_bits == 1 || stop_bits == 2) || (strcmp(parity, "NONE") == 0 || strcmp(parity, "EVEN") == 0 || strcmp(parity, "ODD") == 0 || strcmp(parity, "MASK") == 0 || strcmp(parity, "SPACE") == 0) || (strcmp(flow_control, "NFC") == 0 || strcmp(flow_control, "FC") == 0))
    {
      _Serial->print(F("AT+UART="));
      _Serial->print((bauduart));
      _Serial->print(F(","));
      _Serial->print((data_bits));
      _Serial->print(F(","));
      _Serial->print((stop_bits));
      _Serial->print(F(","));
      _Serial->print((parity));
      _Serial->print(F(","));
      _Serial->print((flow_control));
      _Serial->print(F("\r\n"));
    }
    else
    {
      return true;
    }
    _buffer = _readSerial(100);
    count++;
    delay(RetryDelay);
  }

  while ((count < NumofRetry && count < MAX_Count)&& _buffer.indexOf("+OK") == -1);
  {
    if (_buffer.indexOf("+OK") == -1)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
}

/**
 * @brief 
 * 
 * @return String 
 */
String Usr2Eth::NetworkConfig()
{
  uint8_t indexOne;
  uint8_t indexTwo;
  String IP;
  int count = 0;
  do {
    _Serial->print(F("AT+WANN=DHCP\r\n"));
    delay(100);
    _Serial->print(F("AT+WANN\r\n"));
    delay(100);
    IP = _readSerial(200);
    count++;
    delay(RetryDelay);
  }

  while ((count < NumofRetry && count < MAX_Count)&& IP.indexOf("=DHCP") == -1);
  {
    if (IP.indexOf("=DHCP") == -1 )
    {
      return "ERROR";
    }
    else
    {
      indexOne = IP.indexOf(",") + 1;
      indexTwo = IP.indexOf(",", indexOne);
      return (IP.substring(indexOne, indexTwo));
    }
  }
}

/**
 * @brief 
 * 
 * @param IP_Address 
 * @param Mask 
 * @param Gateway 
 * @return String 
 */
String Usr2Eth::NetworkConfig(const char* IP_Address, const char* Mask, const char* Gateway)
{
  uint8_t indexOne;
  uint8_t indexTwo;
  String IP;
  int count = 0;
  do {
    _Serial->print(F("AT+WANN=STATIC,"));
    _Serial->print(IP_Address);
    _Serial->print(F(","));
    _Serial->print(Mask);
    _Serial->print(F(","));
    _Serial->print(Gateway);
    _Serial->print(F("\r\n"));
    delay(100);
    _Serial->print(F("AT+WANN\r\n"));
    IP = _readSerial(200);
    count++;
    delay(RetryDelay);
  }

  while ((count < NumofRetry && count < MAX_Count)&& IP.indexOf("=STATIC") == -1);
  {
    if (IP.indexOf("=STATIC") == -1 )
    {
      return "ERROR";
    }
    else
    {
      indexOne = IP.indexOf(",") + 1;
      indexTwo = IP.indexOf(",", indexOne);
      return (IP.substring(indexOne, indexTwo));
    }
  }
}

/**
 * @brief 
 * 
 * @param DNS 
 * @return true 
 * @return false 
 */
bool Usr2Eth::SetDNS(const char* DNS)
{
  int count = 0;
  do {
    _Serial->print(F("AT+DNS="));
    _Serial->print(DNS);
    _Serial->print(F("\r\n"));
    _buffer = _readSerial(100);
    count++;
    delay(RetryDelay);
  }

  while ((count < NumofRetry && count < MAX_Count)&& _buffer.indexOf("+OK") == -1);
  {
    if (_buffer.indexOf("+OK") == -1)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
}

/**
 * @brief 
 * 
 * @param Protocol 
 * @param Remote_IP 
 * @param Remote_Port 
 * @return true   on ERROR
 * @return false  on Sucess
 */
bool Usr2Eth::SocketConfig(const char*  Protocol, const char* Remote_IP, uint16_t Remote_Port)
{
  int count = 0;
  do {
    if ((strcmp(Protocol, "TCPS") == 0 || strcmp(Protocol, "TCPC") || strcmp(Protocol, "UDPS") || strcmp(Protocol, "UDPC") || strcmp(Protocol, "HTPC")) && (Remote_Port >= 1 && Remote_Port <= 65535))
    {
      _Serial->print(F("AT+SOCK="));
      _Serial->print(Protocol);
      _Serial->print(F(","));
      _Serial->print(Remote_IP);
      _Serial->print(F(","));
      _Serial->print(Remote_Port);
      _Serial->print(F("\r\n"));
    }
    else
    {
      return true;
    }
    _buffer = _readSerial(300);
    count++;
    delay(RetryDelay);
  }

  while ((count < NumofRetry && count < MAX_Count)&& _buffer.indexOf("+OK") == -1);
  {
    if (_buffer.indexOf("+OK") == -1)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
}

/**
 * @brief 
 * 
 * @return true  on ERROR
 * @return false on Sucess
 */
bool Usr2Eth::TCPStatus()
{
  int count = 0;
  do {
    _Serial->print(F("AT+SOCKLK\r\n"));
    _buffer = _readSerial(100);
    count++;
    delay(RetryDelay);
  }

  while ((count < NumofRetry && count < MAX_Count)&& _buffer.indexOf("+OK") == -1);
  {
    if (_buffer.indexOf("+OK=connect") == -1)
    {
      return true;            // ERROR
    }
    else
    {
      return false;           // CONNECTED
    }
  }
}

/**
 * @brief 
 * 
 * @param URL 
 * @return true 
 * @return false 
 */
bool Usr2Eth::GetHTTP(const char*  URL)
{
  uint8_t flag1;
  uint8_t flag2;
  int count = 0;
  do {
    _Serial->print(F("AT+HTPTP=GET\r\n"));
    _buffer = _readSerial(100);
    count++;
    delay(RetryDelay);
  }

  while ((count < NumofRetry && count < MAX_Count)&& _buffer.indexOf("+OK") == -1);
  {
    if (_buffer.indexOf("+OK") == -1)
    {
      flag1 = 1;
    }
    else {
      flag1 = 0;
    }
  }
  delay(100);
  count = 0;
  do {
    _Serial->print(F("AT+HTPURL="));
    _Serial->print(URL);
    _Serial->print(F("\r\n"));
    _buffer = _readSerial(100);
    count++;
    delay(RetryDelay);
  }

  while ((count < NumofRetry && count < MAX_Count)&& _buffer.indexOf("+OK") == -1);
  {
    if (_buffer.indexOf("+OK") == -1)
    {
      flag2 = 1;
    }
    else {
      flag2 = 0;
    }
    if (flag1 == 1 && flag2 == 1)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
}

/**
 * @brief 
 * 
 * @param URL 
 * @param HTP_HEAD 
 * @return true 
 * @return false 
 */
bool Usr2Eth::GetHTTP(const char*  URL, const char* HTP_HEAD)
{
  uint8_t flag1;
  uint8_t flag2;
  uint8_t flag3;
  int count = 0;
  do {
    _Serial->print(F("AT+HTPTP=GET\r\n"));
    _buffer = _readSerial(100);
    count++;
    delay(RetryDelay);
  }

  while ((count < NumofRetry && count < MAX_Count)&& _buffer.indexOf("+OK") == -1);
  {
    if (_buffer.indexOf("+OK") == -1)
    {
      flag1 = 1;
    }
    else {
      flag1 = 0;
    }
  }
  delay(100);
  count = 0;
  do {
    _Serial->print(F("AT+HTPURL="));
    _Serial->print(URL);
    _Serial->print(F("\r\n"));
    _buffer = _readSerial(100);
    count++;
    delay(RetryDelay);
  }

  while ((count < NumofRetry && count < MAX_Count)&& _buffer.indexOf("+OK") == -1);
  {
    if (_buffer.indexOf("+OK") == -1)
    {
      flag2 = 1;
    }
    else {
      flag2 = 0;
    }
    delay(100);
    _Serial->print(F("AT+HTPHEAD="));
    _Serial->print(HTP_HEAD);
    _Serial->print(F("\r\n"));
    _buffer = _readSerial(100);
    if (_buffer.indexOf("+OK") == -1)
    {
      flag3 = 1;
    }
    else {
      flag3 = 0;
    }
    if ((flag1 == 1 && flag2 == 1) && flag3 == 1)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
}

/**
 * @brief 
 * 
 * @param URL 
 * @return true 
 * @return false 
 */
bool Usr2Eth::PostHTTP(const char*  URL)
{
  uint8_t flag1;
  uint8_t flag2;
  int count = 0;
  do {
    _Serial->print(F("AT+HTPTP=POST\r\n"));
    _buffer = _readSerial(100);
    count++;
    delay(RetryDelay);
  }

  while ((count < NumofRetry && count < MAX_Count)&& _buffer.indexOf("+OK") == -1);
  {
    if (_buffer.indexOf("+OK") == -1)
    {
      flag1 = 1;
    }
    else {
      flag1 = 0;
    }
  }
  delay(100);
  count = 0;
  do {
    _Serial->print(F("AT+HTPURL="));
    _Serial->print(URL);
    _Serial->print(F("\r\n"));
    _buffer = _readSerial(100);
    count++;
    delay(RetryDelay);
  }

  while ((count < NumofRetry && count < MAX_Count)&& _buffer.indexOf("+OK") == -1);
  {
    if (_buffer.indexOf("+OK") == -1)
    {
      flag2 = 1;
    }
    else {
      flag2 = 0;
    }
    if (flag1 == 1 && flag2 == 1)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
}

/**
 * @brief 
 * 
 * @param URL 
 * @param HTP_HEAD 
 * @return true 
 * @return false 
 */
bool Usr2Eth::PostHTTP(const char*  URL, const char* HTP_HEAD)
{
  uint8_t flag1;
  uint8_t flag2;
  uint8_t flag3;
  int count = 0;
  do {
    _Serial->print(F("AT+HTPTP=POST\r\n"));
    _buffer = _readSerial(100);
    count++;
    delay(RetryDelay);
  }

  while ((count < NumofRetry && count < MAX_Count)&& _buffer.indexOf("+OK") == -1);
  {
    if (_buffer.indexOf("+OK") == -1)
    {
      flag1 = 1;
    }
    else {
      flag1 = 0;
    }
  }
  delay(100);
  count = 0;
  do {
    _Serial->print(F("AT+HTPURL="));
    _Serial->print(URL);
    _Serial->print(F("\r\n"));
    _buffer = _readSerial(100);
    count++;
    delay(RetryDelay);
  }

  while ((count < NumofRetry && count < MAX_Count)&& _buffer.indexOf("+OK") == -1);
  {
    if (_buffer.indexOf("+OK") == -1)
    {
      flag2 = 1;
    }
    else {
      flag2 = 0;
    }
  }
  delay(100);
  count = 0;
  do {
    _Serial->print(F("AT+HTPHEAD="));
    _Serial->print(HTP_HEAD);
    _Serial->print(F("\r\n"));
    _buffer = _readSerial(100);
    count++;
    delay(RetryDelay);
  }

  while ((count < NumofRetry && count < MAX_Count)&& _buffer.indexOf("+OK") == -1);
  {
    if (_buffer.indexOf("+OK") == -1)
    {
      flag3 = 1;
    }
    else {
      flag3 = 0;
    }
    if ((flag1 == 1 && flag2 == 1) && flag3 == 1)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
}

/**
 * @brief 
 * 
 * @param data 
 * @return true 
 * @return false 
 */
bool Usr2Eth::HeartData(const char* data)
{
  int len = strlen(data);
  int count = 0;
  do {
    if (len >= 1 && len <= 40)
    {
      _Serial->print(F("AT+HEARTDT="));
      _Serial->print(data);
      _Serial->print(F("\r\n"));

      _buffer = _readSerial();
      count++;
      delay(RetryDelay);
    }
  }
  while ((count < NumofRetry && count < MAX_Count)&& _buffer.indexOf("+OK") == -1);
  {
    if ( (_buffer.indexOf("+OK")) == -1)
    {
      return true;
    }
    else
      return false;
  }
}

/**
 * @brief 
 * 
 * @param _status 
 * @return true   on ERROR
 * @return false  on Sucess
 */
bool Usr2Eth::HeartDirection(const char* _status)
{
  int count = 0;
  do {
    if (strcmp(_status, "net") == 0 || strcmp(_status, "com") == 0 || strcmp(_status, "NET") == 0 || strcmp(_status, "COM") == 0)
    {
      _Serial->print(F("AT+HEARTTP="));
      _Serial->print(_status);
      _Serial->print(F("\r\n"));
    }
    else
    {
      return true;
    }
    _buffer = _readSerial();
    count++;
    delay(RetryDelay);
  }

  while ((count < NumofRetry && count < MAX_Count)&& _buffer.indexOf("+OK") == -1);
  {
    if ( (_buffer.indexOf("+OK")) == -1)
    {
      return true;
    }
    else
      return false;
  }
}

/**
 * @brief 
 * 
 * @param _status 
 * @return true 
 * @return false 
 */
bool Usr2Eth::EnableHeart(const char* _status)
{
  int count = 0;
  do {
    if (strcmp( _status, "on") == 0 || strcmp( _status, "off") == 0 || strcmp( _status, "ON") == 0 || strcmp( _status, "OFF") == 0)
    {
      _Serial->print(F("AT+HEARTEN="));
      _Serial->print( _status);
      _Serial->print(F("\r\n"));
    }
    else
    {
      return true;
    }
    _buffer = _readSerial();
    count++;
    delay(RetryDelay);
  }

  while ((count < NumofRetry && count < MAX_Count)&& _buffer.indexOf("+OK") == -1);
  {
    if ( (_buffer.indexOf("+OK")) == -1)
    {
      return true;
    }
    else
      return false;
  }
}

/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
bool Usr2Eth::HeartStatus()
{
  int count = 0;
  do {
    _Serial->print(F("AT+HEARTEN\r\n"));

    _buffer = _readSerial(100);
    count++;
    delay(RetryDelay);
  }

  while ((count < NumofRetry && count < MAX_Count)&& _buffer.indexOf("+OK") == -1);
  {
    if ( (_buffer.indexOf("+OK=ON")) == -1)
    {
      return true;
    }
    else
      return false;
  }
}

/**
 * @brief 
 * 
 * @param heart_time 
 * @return true 
 * @return false 
 */
bool Usr2Eth::HeartTime(uint16_t heart_time)
{
  int count = 0;
  do {
    if (heart_time >= 1 && heart_time <= 65535)
    {
      _Serial->print(F("AT+HEARTTM="));
      _Serial->print(heart_time);
      _Serial->print(F("\r\n"));

      _buffer = _readSerial();
      count++;
      delay(RetryDelay);
    }
  }
  while ((count < NumofRetry && count < MAX_Count)&& _buffer.indexOf("+OK") == -1);
  {

    if ( (_buffer.indexOf("+OK")) == -1)
    {
      return true;
    }
    else
      return false;
  }
}

/**
 * @brief 
 * 
 * @param Packet_status 
 * @return true 
 * @return false 
 */
bool Usr2Eth::SetPacketIdentity(const char* Packet_status)
{
  int count = 0;
  do {
    _Serial->print(F("AT+REGEN="));
    _Serial->print(Packet_status);
    _Serial->print(F("\r\n"));
    _buffer = _readSerial(100);
    count++;
    delay(RetryDelay);
  }
  while ((count < NumofRetry && count < MAX_Count)&& _buffer.indexOf("+OK") == -1);
  {

    if (_buffer.indexOf("+OK") == -1)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
}

/**
 * @brief 
 * 
 * @param Method 
 * @return true 
 * @return false 
 */
bool Usr2Eth::SendingMethodPacketIdentity(const char* Method)
{
  int count = 0;
  do {
    if ((strcmp(Method, "FIRST") == 0) || (strcmp(Method, "EVERY") == 0) || (strcmp(Method, "ALL") == 0))
    {
      _Serial->print(F("AT+REGTCP="));
      _Serial->print(Method);
      _Serial->print(F("\r\n"));
    }
    else return true;
    _buffer = _readSerial(100);
    count++;
    delay(RetryDelay);
  }
  while ((count < NumofRetry && count < MAX_Count)&& _buffer.indexOf("+OK") == -1);
  {
    if (_buffer.indexOf("+OK") == -1)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
}

/**
 * @brief 
 * 
 * @param Packet_data 
 * @return true 
 * @return false 
 */
bool Usr2Eth::UserEditablePacketIdentity(const char* Packet_data)
{
  int Length = strlen(Packet_data);
  int count = 0;
  do {
    if (Length >= 1 && Length <= 40)
    {
      _Serial->print(F("AT+REGUSR="));
      _Serial->print(Packet_data);
      _Serial->print(F("\r\n"));
    }
    else return true;
    _buffer = _readSerial(100);
    count++;
    delay(RetryDelay);
  }
  while ((count < NumofRetry && count < MAX_Count)&& _buffer.indexOf("+OK") == -1);
  {
    if (_buffer.indexOf("+OK") == -1)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
}

/**
 * @brief 
 * 
 * @return String 
 */
String Usr2Eth::PacketIdentityStatus()
{
  uint8_t indexOne;
  uint8_t indexTwo;
  String Read_packet;
  int count = 0;
  do {
    _Serial->print(F("AT+REGEN\r\n"));
    Read_packet = _readSerial(200);
    count++;
    delay(RetryDelay);
  }
  while ((count < NumofRetry && count < MAX_Count)&& Read_packet.indexOf("+OK") == -1);
  {
    if (Read_packet.indexOf("+OK") == -1 )
    {
      return "ERROR";
    }
    else
    {
      indexOne = Read_packet.indexOf("+OK=") + 4;
      indexTwo = Read_packet.indexOf("\r", indexOne);
      return (Read_packet.substring(indexOne, indexTwo));
    }
  }
}

/**
 * @brief 
 * 
 * @param username 
 * @param password 
 * @return true 
 * @return false 
 */
bool Usr2Eth::SetUserPassword(const char* username, const char* password)
{
  int len_usrname = strlen(username);
  int len_password = strlen(password);
  int count = 0;
  do {
    if ((len_usrname >= 1 && len_usrname <= 5) && (len_password >= 1 && len_password <= 5))
    {
      _Serial->print(F("AT+WEBU="));
      _Serial->print(username);
      _Serial->print(F(","));
      _Serial->print(password);
      _Serial->print(F("\r\n"));
    }
    else
    {
      return true;
    }
    _buffer = _readSerial();
    count++;
    delay(RetryDelay);
  }
  while ((count < NumofRetry && count < MAX_Count)&& _buffer.indexOf("+OK") == -1);
  {
    if ( (_buffer.indexOf("+OK")) == -1)
    {
      return true;
    }
    else
      return false;
  }
}

String Usr2Eth::ReadUsernamePassword()
{
  uint8_t indexOne;
  uint8_t indexTwo;
  String Read_packet;
  int count = 0;
  do {
    _Serial->print(F("AT+WEBU\r\n"));
    Read_packet = _readSerial(200);
    count++;
    delay(RetryDelay);
  }
  while ((count < NumofRetry && count < MAX_Count)&& Read_packet.indexOf("+OK") == -1);
  {

    if (Read_packet.indexOf("+OK") == -1 )
    {
      return "ERROR";
    }
    else
    {
      indexOne = Read_packet.indexOf("+OK=") + 4;
      indexTwo = Read_packet.indexOf("\r", indexOne);
      return (Read_packet.substring(indexOne, indexTwo));
    }
  }
}

/**
 * @brief 
 * 
 * @return String 
 */
String Usr2Eth::SerialRead()
{
  _timeout = 0;
  while  (!_Serial->available() && _timeout < 12000  )
  {
    delay(13);
    _timeout++;
  }
  if (_Serial->available()) {
    return _Serial->readString();
  }
}

/**
 * @brief 
 * 
 */
void Usr2Eth::HardReset()
{
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, 1);
  digitalWrite(RESET_PIN, 0);
  delay(200);
  digitalWrite(RESET_PIN, 1);
}
//
////
//PRIVATE METHODS
//
/**
 * @brief 
 * 
 * @return String 
 */
String Usr2Eth::_readSerial()
{
  uint64_t timeOld = millis();
  //while (!_Serial->available() && !(millis() > timeOld + TIME_OUT_READ_SERIAL))
  while (!port->available() && !(millis() > timeOld + TIME_OUT_READ_SERIAL))
  {
    delay(35);
  }
  String str = "";
  while (port->available())
  {
    if (port->available() > 0)
    {
      str += (char) port->read();
    }
  }
  return str;
}

/**
 * @brief 
 * 
 * @param timeout 
 * @return String 
 */
String Usr2Eth::_readSerial(uint32_t timeout)
{
  uint64_t timeOld = millis();
  while (!_Serial->available() && !(millis() > timeOld + timeout))
  //while (!port->available() && !(millis() > timeOld + timeout))
  {
    delay(35);
  }
  String str = "";
  while (_Serial->available())
  {
    if (_Serial->available() > 0)
    {
      str += (char) _Serial->read();
    }
  }
  return str;
}

bool Usr2Eth::readMessages()
{
    // bool atLeastOneNewMsg = false;
    // while(true)
    // {
    //     if (!readUptoOneMessage())
    //     {
    //         //debugPrint("Inside readUptoOneMessage, Failed",true);
    //         break;
    //     }
    //     atLeastOneNewMsg = true;
    //     if (debugMode && m_protocol == PROTOCOL_OPTIONS::LEGACY_PROTOCOL)
    //     {   
    //         debugPrint(millis(), false);
    //         debugPrint("=> Serial input is: ", false);
    //         debugPrint(m_buffer);
    //     }
    //     if (m_newMessageCB != NULL)
    //     {   
    //         debugPrint("CALLBACK :",false);
    //         debugPrint("CALLBACK is received",true);
    //         m_newMessageCB(this);
    //     }
    //     resetBuffer();  // Clear buffer
    // }
    // return atLeastOneNewMsg;
    bool atLeastOneNewMessage = IUSerial::readMessages();

    return atLeastOneNewMessage;
}