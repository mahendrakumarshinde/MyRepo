
#include <ESP8266.h>

#define BUFFER_SIZE 1024

#define SSID  "lmaswireless"      // change this to match your WiFi SSID
#define PASS  "10F23A0763"  // change this to match your WiFi password
#define PORT  "8080"           // using port 8080 by default

char buffer[BUFFER_SIZE];

// By default we are looking for OK\r\n
char OKrn[] = "OK\r\n";
byte wait_for_esp_response(int timeout, char* term=OKrn) {
  unsigned long t=millis();
  bool found=false;
  int i=0;
  int len=strlen(term);
  // wait for at most timeout milliseconds
  // or if OK\r\n is found
  while(millis()<t+timeout) {
    if(Serial2.available()) {
      buffer[i++]=Serial2.read();
      if(i>=len) {
        if(strncmp(buffer+i-len, term, len)==0) {
          found=true;
          break;
        }
      }
    }
  }
  buffer[i]=0;
  Serial.print(buffer);
  return found;
}

void setup() {

  Serial.begin(115200);   // Teensy USB Serial Port
  delay(5000);

  Serial.println("Let's go!");
  ESP8266 wifi(SP2,13,115200,SP2);
  
  wifi.portInit();
  Serial.println("port initialized"); 
  wifi.init('lmaswireless','10F23A0763');
  Serial.println("initialized"); 
  String IP = wifi.getIP();
  Serial.println("got IP = ");
  Serial.println(IP); 
  

  // assume esp8266 operates at 115200 baud rate
  // change if necessary to match your modules' baud rate
  //Serial2.begin(115200);  // Teensy Hardware Serial port 1   (pins 0 and 1)
  
  delay(5000);
  Serial.println("begin.");  
  //setupWiFi();

  // print device IP address
  Serial.print("device ip addr: ");
  Serial2.println("AT+CIFSR");
  wait_for_esp_response(1000);
}

bool read_till_eol() {
  static int i=0;
  if(Serial2.available()) {
    buffer[i++]=Serial2.read();
    if(i==BUFFER_SIZE)  i=0;
    if(i>1 && buffer[i-2]==13 && buffer[i-1]==10) {
      buffer[i]=0;
      i=0;
      Serial.print(buffer);
      return true;
    }
  }
  return false;
}

void loop() {
  int ch_id, packet_len;
  char *pb;  
  if(read_till_eol()) {
    if(strncmp(buffer, "+IPD,", 5)==0) {
      // request: +IPD,ch,len:data
      sscanf(buffer+5, "%d,%d", &ch_id, &packet_len);
      if (packet_len > 0) {
        // read serial until packet_len character received
        // start from :
        pb = buffer+5;
        while(*pb!=':') pb++;
        pb++;
        if (strncmp(pb, "GET /", 5) == 0) {
          wait_for_esp_response(1000);
          Serial.println("-> serve homepage");
          serve_homepage(ch_id);
        }
      }
    }
  }
}

void serve_homepage(int ch_id) {
  
String header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\nRefresh: 300\r\n";

  String content="";
    content += "<!DOCTYPE html>";
    content += "<html>";
    content += "<body>";
    
    content += " <h1> Infinite Uptime </h1> <br/>  ";
    Serial.print("***************************************** UPTIME = ");
    Serial.println(millis());
    content += "<font color=#0000FF> ";
    content += String(millis());
    content += " milliseconds </font> </p>";
    
      
    content += "</body>";
    content += "</html>";
    content += "<br />\n";       
    content += "\r\n";       

  header += "Content-Length:";
  header += (int)(content.length());
  header += "\r\n\r\n";
  Serial2.print("AT+CIPSEND=");
  Serial2.print(ch_id);
  Serial2.print(",");
  Serial2.println(header.length()+content.length());
  if(wait_for_esp_response(10000)) {
   //delay(100);
   Serial2.print(header);
   Serial2.print(content);
  } 
  else {
  Serial2.print("AT+CIPCLOSE=");
  Serial2.println(ch_id);
  Serial.print(header);
  Serial.print(content);
  Serial.println(content.length()+header.length());
 }
}

void setupWiFi() {

  // turn on echo
  Serial2.println("ATE1");
  wait_for_esp_response(1000);
  
  // try empty AT command
  //Serial2.println("AT");
  //wait_for_esp_response(1000);

  // set mode 1 (client)
  Serial2.println("AT+CWMODE=3");
  wait_for_esp_response(1000); 
 
  // reset WiFi module
  Serial2.print("AT+RST\r\n");
  wait_for_esp_response(1500);

  //DHCP setting
  Serial2.print("AT+CWDHCP_CUR=1,1\r\n");
  wait_for_esp_response(1500);
  Serial2.print("AT+CWDHCP_CUR=1,0\r\n");
  wait_for_esp_response(1500);

   //join AP
  Serial2.print("AT+CWJAP=\"");
  Serial2.print(SSID);
  Serial2.print("\",\"");
  Serial2.print(PASS);
  Serial2.println("\"");
  wait_for_esp_response(5000);

  // start server
  Serial2.println("AT+CIPMUX=1");
   wait_for_esp_response(1000);
  
  //Create TCP Server in 
  Serial2.print("AT+CIPSERVER=1,"); // turn on TCP service
  Serial2.println(PORT);
   wait_for_esp_response(1000);
  
  Serial2.println("AT+CIPSTO=30");  
  wait_for_esp_response(1000);

  Serial2.println("AT+GMR");
  wait_for_esp_response(1000);
  
  Serial2.println("AT+CWJAP?");
  wait_for_esp_response(1000);
  
  Serial2.println("AT+CIPSTA?");
  wait_for_esp_response(1000);
  
  Serial2.println("AT+CWMODE?");
  wait_for_esp_response(1000);
  
  Serial2.println("AT+CIFSR");
  wait_for_esp_response(5000);
  
  Serial2.println("AT+CWLAP");
  wait_for_esp_response(5000);
  
  Serial.println("---------------*****##### READY TO SERVE #####*****---------------");
}
