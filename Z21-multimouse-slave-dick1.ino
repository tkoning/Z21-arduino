/*
   Z21 emulator for Roco command station 10764 and Roco multiMOUSE.

   Version: 04 modified dick koning dec 2016
   Works with XpressNet library version 1.8 (29.09.2015) from Philipp Gahtow 

  "Fathers":
  Initital development: http://pgahtow.de/wiki/index.php?title=XpressNet by Philipp Gahtow.
  Further development which this sketch based on: https://github.com/schabauerj/Roco_Z21_Arduino by Markus Heller

  
  Hardware:
  1) Arduino Mega 2560  of Arduino Uno
  2) Ethernet Shield W5100 
  3) RS485 interface board WaveShare to realize RS485 interface to connect to S-Bus slave of Roco command station:
   
   PinOut
    VCC => 5V of arduino;
    GND => GND of arduino
    RO => TX1 (18) pin (or viseversa, sheck if problem) UNO TX0
    DI => RX1 (19) pin (or viseversa, sheck if problem) UNO RX0
    RSE => Pin 9 (Digital)  originele schets pin 3
    ResetPin => Pin A5   (originele schets A1)  maak A5 bij het opstarten laag als je terug wilt naar het default IP adres van de sketch)
    WebPin   => Pin A4   Maank A4 bij het opstarten laag als je de webinterface wilt opstarten
 
  MAX485 RX/TX are connected to Serial1 pins of MEGA (18/19) => you can use Serial0 for debug log in Serial Terminal window of Arduino IDE app.

  LED on pin 13 is indicator of XpressNet connection: flashes - no connection, solid - OK: in notifyXNetStatus(uint8_t LedState)

  GENERAL Connections and adjustments (how to run):
  1) Plug XpressNet cable from z21 emulator to Slave socket of your ROCO command station (10764). Your multiMouse you should plug as Master.
  2) LAN cable plug to W5100 socket and to WLAN router (use separated with factory settings)
  3) Make WLAN on router with IP address space 192.168.0.* 
  4) Download Z21 app for Android/iOS
  5) Connect to router WIFI. Check IP adress space for 192.168.0.* in mobile device
  6) Edit your MAC adress of ethernet shield below in sketch (you have to know it)
  7) Edit IP adress below in sketch (192.168.0.11 is default).  set IP adress outside DHCP range
  8) If you connect WebPin to GND on startup you can modify settings with webbrowser outside the sketch (use IP adress of ethernetshield)
  9) Run Serial terminal window and check IP adress at start of z21 emulator.
  10)Put this address to settings of your z21 app.
  11)If you connect ResetPIn to GND on startup all settings will revert to default

 
  If you want to use debug.print statements  SerialDebug 1 ( arduino mega only) line 56
  if you want te use the webinterface  Webconfig 1 line 58
  if you ALWAYS want to use the IP adress and Xpressent adress as code in the sketch   FixedIP =true line 77
  
  
  Localisatie van gegevens in de code :
  TxRxpin  (line 72)   ResetPin (line 73)   WebPin (line 74)
  Ip adres (line 83)   Xnetadres (line 91)  FixIp  (line 77)

*/

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
#define SerialDEBUG 1   //For Serial Port Debugging (Arduino Mega only) Zet op 0 als je de dubug niet gebruikt, snellere en kleinere code
#endif
#define WebConfig 1     // Indien 1 wordt op het IP adres van de arduino een webpagina getoond waar de gegevens aanpasbaar zijn.
// Als je geen gebruik maakt van de webconfiguratie wordt de code een stuk kleiner door de WebConfig op 0 te zetten
// de webpagina is slechts de eerste 30 seconden na het opstarten zichtbaar als de WebPin laag is. Om de gegevens over te nemen moet je opnieuw opstarten en de FixIp optie uitzetten

#include <XpressNet.h>   // connect XpressNet lib
XpressNetClass XpressNet;

#include <SPI.h>         // needed for Arduino versions later than 0018
#include <Ethernet.h>    // Ethernet lib
#include <EthernetUdp.h> // UDP library
#include <EEPROM.h>

#define EEip 10          //Startddress im EEPROM für die IP
#define EEXNet 9         //Adres in EEPROM voor XNet-Bus
#define XNetTxRxPin 9    //Send/Receive Pin MAX
#define ResetPin A5      //Maak ResetPin laag to set to default IP when restarting!
#define WebPin A4        //Maak Webpin laag om bij het opstarten om een webpagina voor het instellen IP nummers te krijgen
// aanvullende settings

bool FixIP = false;      // Indien true wordt  het IP adres bij het opstarten altijd teruggezet naar het default IP adres, zet op false als je met de webconfig wilt werken

// Define MAC address of your Ethernet shield:
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // Check with your W5100 shield MAC address!!!

// IP address of z21. Depends on your LAN configuration. Should be the same sub net.
IPAddress ip(192, 168, 0, 11);

// XpressNet address: must be in range of 1-31; must be unique. Note that some IDs
// are currently used by default, like 2 for a LH90 or LH100 out of the box, or 30
// for PC interface devices like the XnTCP.

byte XNetAddress = 30;    //Adresse im XpressNet

// Defines form z21.h file:   // dubbele declaratie ?? uitschakelen heeft geen nadelig effect
//extern void z21Receive();
//extern void z21CheckActiveIP();
//extern void z21Setup();
//extern XpressNetClass XpressNet;

#define LAN_GET_SERIAL_NUMBER 0x10
#define LAN_GET_CONFIG 0x12
#define LAN_GET_HWINFO 0x1A
#define LAN_LOGOFF 0x30
#define LAN_XPRESS_NET 0x40
#define LAN_X_GENERAL 0x21
#define LAN_X_GET_VERSION 0x21
#define LAN_X_GET_STATUS 0x24
#define LAN_X_SET_TRACK_POWER_OFF 0x80
#define LAN_X_SET_TRACK_POWER_ON 0x81
#define LAN_X_CV_READ_0 0x23
#define LAN_X_CV_READ_1 0x11
#define LAN_X_CV_WRITE_0 0x24
#define LAN_X_CV_WRITE_1 0x12
#define LAN_X_GET_TURNOUT_INFO 0x43
#define LAN_X_SET_TURNOUT 0x53
#define LAN_X_SET_STOP 0x80
#define LAN_X_GET_LOCO_INFO_0 0xE3
#define LAN_X_GET_LOCO_INFO_1 0xF0
#define LAN_X_SET_LOCO_FUNCTION_0 0xE4
#define LAN_X_SET_LOCO_FUNCTION_1 0xF8
#define LAN_X_CV_POM 0xE6
#define LAN_X_CV_POM_WRITE 0x30
#define LAN_X_CV_POM_WRITE_BYTE 0xEC
#define LAN_X_CV_POM_WRITE_BIT 0xE8
#define LAN_X_GET_FIRMWARE_VERSION 0xF1  //0x141 0x21 0x21 0x00 
#define LAN_SET_BROADCASTFLAGS 0x50
#define LAN_GET_BROADCASTFLAGS 0x51
#define LAN_GET_LOCOMODE 0x60
#define LAN_SET_LOCOMODE 0x61
#define LAN_GET_TURNOUTMODE 0x70
#define LAN_SET_TURNOUTMODE 0x71
#define LAN_RMBUS_GETDATA 0x81
#define LAN_RMBUS_PROGRAMMODULE 0x82
#define LAN_SYSTEMSTATE_GETDATA 0x85    //0x141 0x21 0x24 0x05
#define LAN_RAILCOM_GETDATA 0x89
#define LAN_LOCONET_FROM_LAN 0xA2
#define LAN_LOCONET_DISPATCH_ADDR 0xA3

// End defines of z21. file ==================

// CPP defines:
byte XBusVer = 0x30;

// buffers for receiving and sending data
unsigned char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,

// IP structure definitions:
#define maxIP 10        //Speichergröße für IP-Adressen
#define ActTimeIP 20    //Aktivhaltung einer IP für (sec./2)
#define interval 2000   //Check active IP every 2 seconds. 
struct TypeActIP {
  byte ip0;    // Byte IP
  byte ip1;    // Byte IP
  byte ip2;    // Byte IP
  byte ip3;    // Byte IP
  byte time;   //Time
};

TypeActIP ActIP[maxIP];    //Speicherarray für IPs

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Udp;
unsigned int localPort = 21105;      // local port to listen on
// end of CPP defines ==========


#if WebConfig
EthernetServer server(80);  //Run ethernet shield in server mode
#endif


#define interval 2000   //interval at milliseconds

long previousMillis = 0;        // will store last time of IP decount updated

// Pin 13 has an LED connected on most Arduino boards, for notifyXNetStatus(uint8_t LedState)
// give it a name:
int led = 13;
bool webchange = false;
bool webreturn = false;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// S E T U P
void setup() {
#if SerialDEBUG
  Serial.begin(9600); // Debug via terminal window at 9600
  Serial.println("Z21 - v04 modified");
#endif

  pinMode(ResetPin, INPUT); //define reset button pin as input pin
  digitalWrite(ResetPin, HIGH);  //PullUp to HIGH
  pinMode(WebPin, INPUT); //define reset button pin as input pin
  digitalWrite(WebPin, HIGH);  //PullUp to HIGH

  delay(100);

  // EEPROM usage for storing the IP adresses
  if (digitalRead(ResetPin) == LOW || FixIP == true || EEPROM.read(EEXNet) == 255) {
    EEPROM.write(EEXNet, XNetAddress);
    EEPROM.write(EEip, ip[0]);
    EEPROM.write(EEip + 1, ip[1]);
    EEPROM.write(EEip + 2, ip[2]);
    EEPROM.write(EEip + 3, ip[3]);
  }

  XNetAddress = EEPROM.read(EEXNet);
  ip[0] = EEPROM.read(EEip);
  ip[1] = EEPROM.read(EEip + 1);
  ip[2] = EEPROM.read(EEip + 2);
  ip[3] = EEPROM.read(EEip + 3);

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  pinMode(53, OUTPUT);  // tbv stabiliteit ethernet schield mega
#endif

  /* Disable SD card */
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);


  // start the Ethernet and UDP:
  Ethernet.begin(mac, ip);
  delay(1000);
#if WebConfig
  // ga bij het opstarten naar de webconfig pagina als je aan beide voorwaarden voldoet
  if (digitalRead(WebPin) == LOW && !FixIP) {
    server.begin();     //http server
    webconfiguratie();  //ga naar de webconfiguratie pagina
  }
#endif

  XpressNet.start(XNetAddress, XNetTxRxPin);   // Start XpressNet lib with XnetAddress and PIN 3 as control pin. If you use Mega, then XpressNet lin use Serial1 for MAX485 communication
  z21Setup();


#if SerialDEBUG
  Serial.print("Starting Ethernet at IP address:"); Serial.println(ip);
  Serial.println("Starting XPressNet");
  Serial.println("Starting Z21 Emulation");
  Serial.print("Xpressnet adres: "); Serial.println(XNetAddress);
  Serial.println("Setup finished");
#endif
}



// end of SETUP

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// L O O P
void loop() {
  z21Receive();
  XpressNet.receive();  // XpressNet work function: read and write to X-net bus.
  z21Receive();
  XpressNet.receive();  // XpressNet work function: read and write to X-net bus.

  //Check active IP every 2 seconds.
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis > interval) {
    previousMillis = currentMillis;
    z21CheckActiveIP();
  }
}

// end of L O O P ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Functions part:

/**********************************************************
   HELPERS
 * ********************************************************/

//--------------------------------------------------------------------------------------------
void clearIPSlots() {
  for (int i = 0; i < maxIP; i++) {
    ActIP[i].ip0 = 0;
    ActIP[i].ip1 = 0;
    ActIP[i].ip2 = 0;
    ActIP[i].ip3 = 0;
    ActIP[i].time = 0;
  }
}

String printIP(byte * ip) {
  static String ips;
  ips = String(ip[0]);
  ips += '.';
  ips += ip[1];
  ips += '.';
  ips += ip[2];
  ips += '.';
  ips += ip[3];
  return ips;
}

//--------------------------------------------------------------------------------------------
void addIPToSlot (byte ip0, byte ip1, byte ip2, byte ip3) {
  byte Slot = maxIP;
  for (int i = 0; i < maxIP; i++) {
    if (ActIP[i].ip0 == ip0 && ActIP[i].ip1 == ip1 && ActIP[i].ip2 == ip2 && ActIP[i].ip3 == ip3) {
      ActIP[i].time = ActTimeIP;
      return;
    }
    else if (ActIP[i].time == 0 && Slot == maxIP)
      Slot = i;
  }
  ActIP[Slot].ip0 = ip0;
  ActIP[Slot].ip1 = ip1;
  ActIP[Slot].ip2 = ip2;
  ActIP[Slot].ip3 = ip3;
  ActIP[Slot].time = ActTimeIP;
  notifyXNetPower(XpressNet.getPower());
}



//--------------------------------------------------------------------------------------------
//Send out data via Ethernet
void EthSendOut (unsigned int DataLen, unsigned int Header, byte Data[], boolean withXOR) {
  Udp.write(DataLen & 0xFF);
  Udp.write(DataLen & 0xFF00);
  Udp.write(Header & 0xFF);
  Udp.write(Header & 0xFF00);

  unsigned char XOR = 0;
  byte ldata = DataLen - 5; //Withiut Length und Header und XOR
  if (!withXOR)    //XOR present?
    ldata++;
  for (int i = 0; i < (ldata); i++) {
    XOR = XOR ^ Data[i];
    Udp.write(Data[i]);
  }
  if (withXOR)
    Udp.write(XOR);
}

//--------------------------------------------------------------------------------------------
void EthSend (unsigned int DataLen, unsigned int Header, byte Data[], boolean withXOR, boolean BC) {
  if (BC) {
    IPAddress IPout = Udp.remoteIP();
    for (int i = 0; i < maxIP; i++) {
      if (ActIP[i].time > 0) {    //Still aktiv?
        IPout[0] = ActIP[i].ip0;
        IPout[1] = ActIP[i].ip1;
        IPout[2] = ActIP[i].ip2;
        IPout[3] = ActIP[i].ip3;
        Udp.beginPacket(IPout, Udp.remotePort());    //Broadcast
        EthSendOut (DataLen, Header, Data, withXOR);
        Udp.endPacket();
      }
    }
  }
  else {
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());    //Broadcast
    EthSendOut (DataLen, Header, Data, withXOR);
    Udp.endPacket();
  }
}

//--------------------------------------------------------------------------------------------
void notifyXNetPower (uint8_t State)
{
#if SerialDEBUG
  Serial.print("XNet Power: 0x");
  Serial.print(State, HEX);
#endif

  byte data[] = {0x61, 0x00};
  switch (State) {
    case csNormal: data[1] = 0x01;
#if SerialDEBUG
      Serial.println(" => ON");
#endif
      break;
    case csTrackVoltageOff: data[1] = 0x00;
#if SerialDEBUG
      Serial.println(" => Voltage OFF");
#endif
      break;
    case csServiceMode: data[1] = 0x02;
#if SerialDEBUG
      Serial.println(" => Service Mode");
#endif
      break;
    case csEmergencyStop:
      data[0] = 0x81;
      data[1] = 0x00;
#if SerialDEBUG
      Serial.println(" => Emergency Stop");
#endif
      break;
  }
  EthSend(0x07, 0x40, data, true, true);
}

//--------------------------------------------------------------------------------------------
void notifyLokFunc(uint8_t Adr_High, uint8_t Adr_Low, uint8_t F2, uint8_t F3 ) {
#if SerialDEBUG
  Serial.println("Loco Fkt: ");
  Serial.println(Adr_Low);
  Serial.println(", Fkt2: ");
  Serial.println(F2, BIN);
  Serial.println("; ");
  Serial.println(F3, BIN);
#endif
}


//--------------------------------------------------------------------------------------------
void notifyLokAll(uint8_t Adr_High, uint8_t Adr_Low, boolean Busy, uint8_t Steps, uint8_t Speed, uint8_t Direction, uint8_t F0, uint8_t F1, uint8_t F2, uint8_t F3, boolean Req) {
  byte DB2 = Steps;
  if (DB2 == 3)  //unavailable!
    DB2 = 4;
  if (Busy)
    bitWrite(DB2, 3, 1);
  byte DB3 = Speed;
  if (Direction == 1)
    bitWrite(DB3, 7, 1);
  byte data[9];
  data[0] = 0xEF;  //X-HEADER
  data[1] = Adr_High & 0x3F;
  data[2] = Adr_Low;
  data[3] = DB2; // steps
  data[4] = DB3; //speed
  data[5] = F0;    //F0, F4, F3, F2, F1
  data[6] = F1;    //F5 - F12; Funktion F5 is bit0 (LSB)
  data[7] = F2;  //F13-F20
  data[8] = F3;  //F21-F28
  EthSend (14, 0x40, data, true, true);  //Send Power und Funktions to all active Apps

#if SerialDEBUG
  Serial.print("notifyLokAll(): ADDR_HI: ");
  Serial.print(data[1], DEC);
  Serial.print(", ADDR_LO: ");
  Serial.print(data[2], DEC);
  Serial.print(", STEPS: ");
  Serial.print(data[3], DEC);
  Serial.print(", Speed: ");
  Serial.print(data[4], BIN);
  Serial.print(", F1-4: ");
  Serial.print(data[5], BIN);
  Serial.print(", F5-12: ");
  Serial.print(data[6], BIN);
  Serial.print(", F13-20: ");
  Serial.print(data[7], BIN);
  Serial.print(", F11-28: ");
  Serial.println(data[8], BIN);
#endif


}

//--------------------------------------------------------------------------------------------
void notifyTrnt(uint8_t Adr_High, uint8_t Adr_Low, uint8_t Pos) {
#if SerialDEBUG
  Serial.print("TurnOut: ");
  Serial.print(word(Adr_High, Adr_Low));
  Serial.print(", Position: ");
  Serial.println(Pos, BIN);
#endif

  //LAN_X_TURNOUT_INFO
  byte data[4];
  data[0] = 0x43;  //HEADER
  data[1] = Adr_High;
  data[2] = Adr_Low;
  data[3] = Pos;
  EthSend (0x09, 0x40, data, true, false);
}

//--------------------------------------------------------------------------------------------
void notifyCVInfo(uint8_t State ) {
#if SerialDEBUG
  Serial.print("CV Prog STATE: ");
  Serial.println(State);
#endif
  if (State == 0x01 || State == 0x02) {  //Busy or No Data

    //LAN_X_CV_NACK
    byte data[2];
    data[0] = 0x61;  //HEADER
    data[1] = 0x13; //DB0
    EthSend (0x07, 0x40, data, true, false);
  }
}

//--------------------------------------------------------------------------------------------
void notifyCVResult(uint8_t cvAdr, uint8_t cvData ) {
  //LAN_X_CV_RESULT
  byte data[5];
  data[0] = 0x64; //HEADER
  data[1] = 0x14;  //DB0
  data[2] = 0x00;  //CVAdr_MSB
  data[3] = cvAdr;  //CVAdr_LSB
  data[4] = cvData;  //Value
  EthSend (0x0A, 0x40, data, true, false);
}

//--------------------------------------------------------------------------------------------
void notifyXNetVersion(uint8_t Version, uint8_t ID ) {
  XBusVer = Version;
}

//--------------------------------------------------------------------------------------------
void notifyXNetStatus(uint8_t LedState )
{
  digitalWrite(led, LedState);
}

// Parser functon to read X-Bus input and send to z21 Client App
void xPressNetParse(byte* packetBuffer, byte* data) {
  boolean ok = false;
  switch (packetBuffer[4]) { //X-Header
    case LAN_X_GENERAL:
      switch (packetBuffer[5]) {  //DB0

        case LAN_X_GET_VERSION:
          data[0] = 0x63;
          data[1] = 0x21;
          data[3] = XBusVer;   //X-Bus Version
          data[4] = 0x12;  //ID der Zentrale
          data[5] = 0;
          EthSend (0x09, 0x40, data, true, false);
#if SerialDEBUG
          Serial.println("LAN_X_GET_VERSION");
#endif
          break;

        case LAN_X_GET_STATUS:
          data[0] = 0x62;
          data[1] = 0x22;
          data[2] = XpressNet.getPower();
          EthSend (0x08, 0x40, data, true, false);
#if SerialDEBUG
          Serial.println("LAN_X_GET_STATUS"); //This is asked very often ...
#endif
          break;

        case LAN_X_SET_TRACK_POWER_OFF:
          ok = XpressNet.setPower(csTrackVoltageOff);
#if SerialDEBUG
          Serial.println("LAN_X_SET_TRACK_POWER_OFF");
          if (ok == false) {
            Serial.println("Power Send FEHLER");
          }
#endif
          break;

        case LAN_X_SET_TRACK_POWER_ON:
          ok = XpressNet.setPower(csNormal);
#if SerialDEBUG
          Serial.println("LAN_X_SET_TRACK_POWER_ON");
          if (ok == false) {
            Serial.println("Power Send FEHLER");
          }
#endif
          break;
      }
      break;

    case LAN_X_CV_READ_0:
      if (packetBuffer[5] == LAN_X_CV_READ_1) {  //DB0
#if SerialDEBUG
        Serial.println("LAN_X_CV_READ");
#endif
        byte CV_MSB = packetBuffer[6];
        byte CV_LSB = packetBuffer[7];
        XpressNet.readCVMode(CV_LSB + 1);
      }
      break;

    case LAN_X_CV_WRITE_0:
      if (packetBuffer[5] == LAN_X_CV_WRITE_1) {  //DB0
#if SerialDEBUG
        Serial.println("LAN_X_CV_WRITE");
#endif
        byte CV_MSB = packetBuffer[6];
        byte CV_LSB = packetBuffer[7];
        byte value = packetBuffer[8];
        XpressNet.writeCVMode(CV_LSB + 1, value);
      }
      break;

    case LAN_X_GET_TURNOUT_INFO:
#if SerialDEBUG
      Serial.println("LAN_X_GET_TURNOUT_INFO");
#endif
      XpressNet.getTrntInfo(packetBuffer[5], packetBuffer[6]);
      break;

    case LAN_X_SET_TURNOUT:
#if SerialDEBUG
      //Serial.println("LAN_X_SET_TURNOUT");
#endif
      XpressNet.setTrntPos(packetBuffer[5], packetBuffer[6], packetBuffer[7] & 0x0F);
      break;

    case LAN_X_SET_STOP:
      ok = XpressNet.setPower(csEmergencyStop);
#if SerialDEBUG
      Serial.println("LAN_X_SET_STOP");
      if (ok == false) {
        Serial.println("Power Send FEHLER");
      }
#endif
      break;

    case LAN_X_GET_LOCO_INFO_0:

      if (packetBuffer[5] == LAN_X_GET_LOCO_INFO_1) {  //DB0=

        //Ask Xbus for current loko infor:
        //                  LAN_X_LOCO_INFO  Adr_MSB - Adr_LSB
        XpressNet.getLocoInfo (packetBuffer[6] & 0x3F, packetBuffer[7]); // >> run notifyLokAll()

        //XpressNet.getLocoFunc (packetBuffer[6] & 0x3F, packetBuffer[7]); // >> run notifyLokFunc() run F13 bis F28 //under revision

#if SerialDEBUG
        Serial.print("LAN_X_GET_LOCO_INFO on address: ");
        Serial.print(packetBuffer[6] & 0x3F, DEC);
        Serial.print(",  ");
        Serial.println(packetBuffer[7], DEC);
#endif
      }
      break;

    case LAN_X_SET_LOCO_FUNCTION_0: //SET loco func and speed here:
      if (packetBuffer[5] == LAN_X_SET_LOCO_FUNCTION_1) {  //DB0 - X Header
        //LAN_X_SET_LOCO_FUNCTION  Adr_MSB-DB1        Adr_LSB-DB2     Type (EIN/AUS/UM) DB3   FunktionDB3
        XpressNet.setLocoFunc(packetBuffer[6] & 0x3F, packetBuffer[7], packetBuffer[8] >> 5, packetBuffer[8] & B00011111);

      }
      else {
        //LAN_X_SET_LOCO_DRIVE            Adr_MSB          Adr_LSB      DB0                    Dir+Speed
        XpressNet.setLocoDrive(packetBuffer[6] & 0x3F, packetBuffer[7], packetBuffer[5] & B11, packetBuffer[8]);
      }
      // LAN_X_GET_LOCO_INFO_0 -> Client:
      XpressNet.getLocoInfo (packetBuffer[6] & 0x3F, packetBuffer[7]); // >> run notifyLokAll()
      break;

    case LAN_X_CV_POM:
#if SerialDEBUG
      Serial.println("LAN_X_CV_POM");
#endif
      if (packetBuffer[5] == LAN_X_CV_POM_WRITE) {  //DB0
        byte Option = packetBuffer[8] & B11111100;  //Option DB3
        byte Adr_MSB = packetBuffer[6] & 0x3F;  //DB1
        byte Adr_LSB = packetBuffer[7];    //DB2
        int CVAdr = packetBuffer[9] | ((packetBuffer[8] & B11) << 7);
        if (Option == LAN_X_CV_POM_WRITE_BYTE) {
#if SerialDEBUG
          Serial.println("Client ask: LAN_X_CV_POM_WRITE_BYTE");
#endif
          byte value = packetBuffer[10];  //DB5
        }
        if (Option == LAN_X_CV_POM_WRITE_BIT) {
#if SerialDEBUG
          Serial.println("Client ask: LAN_X_CV_POM_WRITE_BIT");
#endif
          //Nicht von der APP Unterstützt
        }
      }
      break;
    case LAN_X_GET_FIRMWARE_VERSION:
#if SerialDEBUG
      Serial.println("Client ask: LAN_X_GET_FIRMWARE_VERSION");
#endif
      data[0] = 0xf3;
      data[1] = 0x0a;
      data[3] = 0x01;   //V_MSB
      data[4] = 0x23;  //V_LSB

      EthSend (0x09, 0x40, data, true, false);
      break;
  }
}

// ENSD of xPressNetParse ///////////////////////////////////

/********************************
   Public Members
 * *****************************/

void z21Setup() {
  Udp.begin(localPort);  //UDP Z21 Port
  clearIPSlots();  //Delete stored active IP's
}

void z21CheckActiveIP() {
  for (int i = 0; i < maxIP; i++) {
    if (ActIP[i].time > 0) {
      ActIP[i].time--;    //Zeit herrunterrechnen
    }
    else {
      //clear IP DATA
      ActIP[i].ip0 = 0; ActIP[i].ip1 = 0; ActIP[i].ip2 = 0; ActIP[i].ip3 = 0; ActIP[i].time = 0;
    }
  }

}

//--------------------------------------------------------------------------------------------
void z21Receive() {
  int packetSize = Udp.parsePacket();
  if (packetSize > 0) {
    addIPToSlot(Udp.remoteIP()[0], Udp.remoteIP()[1], Udp.remoteIP()[2], Udp.remoteIP()[3]);
    Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE); // read the packet into packetBufffer
    // send a reply, to the IP address and port that sent us the packet we received
    int header = (packetBuffer[3] << 8) + packetBuffer[2];
    //    int datalen = (packetBuffer[1]<<8) + packetBuffer[0];
    byte data[16];
    boolean ok = false;
    //packetBuffer[packetSize]= 0;
#if SerialDEBUG
    //Serial.println("z21 packetBuffer: ", (char*) packetBuffer);
#endif
    switch (header) {
      case LAN_GET_SERIAL_NUMBER:
        data[0] = 0xF5;  //Seriennummer 32 Bit (little endian)
        data[1] = 0x0A;
        data[2] = 0x00;
        data[3] = 0x00;
        EthSend (0x08, 0x10, data, false, false);
#if SerialDEBUG
        Serial.println("Client ask: LAN_GET_SERIAL_NUMBER");
        //Serial.println("z21 Serial Number: ", (char*) data);
#endif
        break;
      case LAN_GET_CONFIG:
#if SerialDEBUG
        Serial.println("Client ask: Z21-Settings");
#endif
        break;
      case LAN_GET_HWINFO:
        data[0] = 0x00;  //HwType 32 Bit
        data[1] = 0x00;
        data[2] = 0x02;
        data[3] = 0x01;
        data[4] = 0x20;  //FW Version 32 Bit
        data[5] = 0x01;
        data[6] = 0x00;
        data[7] = 0x00;
        EthSend (0x0C, 0x1A, data, false, false);
#if SerialDEBUG
        Serial.println("Client ask: LAN_GET_HWINFO");
#endif
        break;
      case LAN_LOGOFF:
#if SerialDEBUG
        Serial.println("Client ask: LAN_LOGOFF");
#endif
        //Antwort von Z21: keine
        break;
      case LAN_XPRESS_NET:
        xPressNetParse(packetBuffer, data); // PARSE AND send to X-BUS =====================>>
        break;
      case LAN_SET_BROADCASTFLAGS:
#if SerialDEBUG
        Serial.println("Client ask: LAN_SET_BROADCASTFLAGS: ");
        Serial.println(packetBuffer[4], BIN);  // 1=BC Power, Loco INFO, Trnt INFO; B100=BC Sytemstate Datachanged
#endif
        break;
      case LAN_GET_BROADCASTFLAGS:
#if SerialDEBUG
        Serial.println("Client ask: LAN_GET_BROADCASTFLAGS");
#endif
        break;
      case LAN_GET_LOCOMODE:
#if SerialDEBUG
        Serial.print("Client ask: LAN_GET_LOCOMODE with address:");
        Serial.println(packetBuffer[7]);
#endif
        break;
      case LAN_SET_LOCOMODE:
#if SerialDEBUG
        Serial.print("Client ask: LAN_SET_LOCOMODE with address: ");
        Serial.println(packetBuffer[7]);
#endif
        break;
      case LAN_GET_TURNOUTMODE:
#if SerialDEBUG
        Serial.println("Client ask: LAN_GET_TURNOUTMODE");
#endif
        break;
      case LAN_SET_TURNOUTMODE:
#if SerialDEBUG
        Serial.println("Client ask: LAN_SET_TURNOUTMODE");
#endif
        break;
      case LAN_RMBUS_GETDATA:
#if SerialDEBUG
        Serial.println("Client ask: LAN_RMBUS_GETDATA");
#endif
        break;
      case LAN_RMBUS_PROGRAMMODULE:
#if SerialDEBUG
        Serial.println("Client ask: LAN_RMBUS_PROGRAMMODULE");
#endif
        break;
      case LAN_SYSTEMSTATE_GETDATA:
#if SerialDEBUG
        Serial.println("Client ask: LAN_SYSTEMSTATE_GETDATA");  //LAN_SYSTEMSTATE_DATACHANGED
#endif
        data[0] = 0x00;  //MainCurrent mA
        data[1] = 0x00;  //MainCurrent mA
        data[2] = 0x00;  //ProgCurrent mA
        data[3] = 0x00;  //ProgCurrent mA
        data[4] = 0x00;  //FilteredMainCurrent
        data[5] = 0x00;  //FilteredMainCurrent
        data[6] = 0x00;  //Temperature
        data[7] = 0x20;  //Temperature
        data[8] = 0x0F;  //SupplyVoltage
        data[9] = 0x00;  //SupplyVoltage
        data[10] = 0x00;  //VCCVoltage
        data[11] = 0x03;  //VCCVoltage
        data[12] = XpressNet.getPower();  //CentralState
        data[13] = 0x00;  //CentralStateEx
        data[14] = 0x00;  //reserved
        data[15] = 0x00;  //reserved
        EthSend (0x14, 0x84, data, false, false);
        break;
      case LAN_RAILCOM_GETDATA:

        break;
      case LAN_LOCONET_FROM_LAN:
        break;
      case LAN_LOCONET_DISPATCH_ADDR:
        break;
      default:
#if SerialDEBUG
        Serial.println("Client ask: LAN_X_UNKNOWN_COMMAND");
#endif
        data[0] = 0x61;
        data[1] = 0x82;
        EthSend (0x07, 0x40, data, true, false);
    }
  }
}
//--------------------------------------------------------------------------------------------

#if WebConfig
void(* resetFunc) (void) = 0;    //declare reset function at address 0


void webconfiguratie() {
  unsigned long stopMillis = millis();
#if SerialDEBUG
  Serial.println("Start webconfig pagina");
#endif

  while (millis() - stopMillis < 60000) {    // time out van de webpagina 1 minuut
    Webconfig();
    delay(10);
    if (webchange) {
#if SerialDEBUG
      Serial.println("Reset met de nieuwe gegevens");
#endif
      delay(2000);
      resetFunc();
      break;
    }
    if (webreturn) {
      delay(100);
      break;
    }
  }
}


void Webconfig() {
  int S88Module = 0; //  Dummy variablele :ik gebruik de default webpagina waar je ook een S88 kunt instellen


  EthernetClient client = server.available();
  if (client) {
    String receivedText = String(50);
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (receivedText.length() < 50) {
          receivedText += c;
        }
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          //client.println("Connection: close");  // the connection will be closed after completion of the response
          //client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
          //Website:
          client.println("<meta http-equiv='Cache-Control' content='no-cache, no-store, must-revalidate' />");
          client.println("<html><head><title>Z21</title></head><body>");
          client.println("<h1>Z21</h1><br />");
          //----------------------------------------------------------------------------------------------------
          int firstPos = receivedText.indexOf("?");
          if (firstPos > -1) {
            webreturn = true;  // de header bevat een vraagteken dat betekent dat er op de return knop gedrukt is (of een verkeerde link is gebruikt)
            client.println("AutoRESET after change of IP or Xpressnet paramters ");
            byte lastPos = receivedText.indexOf(" ", firstPos);
            String theText = receivedText.substring(firstPos + 3, lastPos); // 10 is the length of "?A="
            byte S88Pos = theText.indexOf("&S88=");
            S88Module = theText.substring(S88Pos + 5, theText.length()).toInt();
            byte XNetPos = theText.indexOf("&XNet=");
            XNetAddress = theText.substring(XNetPos + 6, S88Pos).toInt();
            byte Aip = theText.indexOf("&B=");
            byte Bip = theText.indexOf("&C=", Aip);
            byte Cip = theText.indexOf("&D=", Bip);
            byte Dip = theText.substring(Cip + 3, XNetPos).toInt();
            Cip = theText.substring(Bip + 3, Cip).toInt();
            Bip = theText.substring(Aip + 3, Bip).toInt();
            Aip = theText.substring(0, Aip).toInt();
            ip[0] = Aip;
            ip[1] = Bip;
            ip[2] = Cip;
            ip[3] = Dip;
            //if (EEPROM.read(EES88Moduls) != S88Module) {
            //  EEPROM.write(EES88Moduls, S88Module);
            //  SetupS88();
            //}
            if (EEPROM.read(EEXNet) != XNetAddress) {
              EEPROM.write(EEXNet, XNetAddress);
              webchange = true;
            }
            if (EEPROM.read(EEip) != Aip) {
              EEPROM.write(EEip, Aip);
              webchange = true;
            }
            if (EEPROM.read(EEip + 1) != Bip) {
              EEPROM.write(EEip + 1, Bip);
              webchange = true;
            }
            if (EEPROM.read(EEip + 2) != Cip) {
              EEPROM.write(EEip + 2, Cip);
              webchange = true;
            }
            if (EEPROM.read(EEip + 3) != Dip) {
              EEPROM.write(EEip + 3, Dip);
              webchange = true;
            }
          }
          else {
            client.println("");
          }
          //----------------------------------------------------------------------------------------------------
          client.print("<form method=get>IP-Adr.: <input type=number min=10 max=254 name=A value=");
          client.println(ip[0]);
          client.print(">.<input type=number min=0 max=254 name=B value=");
          client.println(ip[1]);
          client.print(">.<input type=number min=0 max=254 name=C value=");
          client.println(ip[2]);
          client.print(">.<input type=number min=0 max=254 name=D value=");
          client.println(ip[3]);
          client.print("><br /> XBus Adr.: <input type=number min=1 max=31 name=XNet value=");
          client.print(XNetAddress);
          client.print("><br /> S88 8x Module: <input type=number min=0 max=62 name=S88 value=");
          client.print(S88Module);
          client.println("><br /><br />");
          client.println("<input type=submit></form>");
          client.println("</body></html>");

          break;
        }
        if (c == '\n')
          currentLineIsBlank = true; // you're starting a new line
        else if (c != '\r')
          currentLineIsBlank = false; // you've gotten a character on the current line
      }
    }
    client.stop();  // close the connection:
  }
}
#endif
