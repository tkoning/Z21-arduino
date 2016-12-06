# Z21-arduino
Z21 arduino 
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

