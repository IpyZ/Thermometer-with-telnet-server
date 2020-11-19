// *****************************************************************************
//
//
// Created by: IpyZ
// On Arduino Mega
//
//
// *****************************************************************************








#include <AM2320.h>
#include <RTClib.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <LiquidCrystal_I2C.h>
#include <Ethernet.h>

// Modules
RTC_DS1307 rtc;
AM2320 bme(&Wire);
LiquidCrystal_I2C lcd (0x3F, 16, 2);
EthernetServer server (23);
EthernetClient client = 0;

// Files
File file;
File settingsFile;

char fileName[] = "PLIK00.CSV";
int lp = 0;

// Ethernet
byte mac[] = {0xFE, 0xEE, 0x19, 0x21, 0x37, 0x69};
#define textBuffSize 20
char textBuff[textBuffSize];
int charsReceived = 0;

boolean connectFlag = false;

unsigned long timeOfLastActivity;
#define allowedConnectTime 300000

boolean etherGood = false;

int writeDelay = 5000;
unsigned long lastMillis = 0;

int mode = 0;
bool gettingDate = false;
int m_year, m_month, m_day, m_hour, m_minute, m_second;

// "Grades" char
byte customChar[] = {
    B00100,
    B01010,
    B00100,
    B00000,
    B00000,
    B00000,
    B00000,
    B00000
};

void setup() {
    
    // If RTC is unpluged stop 
    if (!rtc.begin ()) {
        while (1);
    }

    // LCD
    lcd.init ();
    lcd.init ();
    lcd.createChar (0, customChar);
    lcd.backlight ();
    lcd.setCursor (0, 0);
    lcd.print ("IpyZ production");
    delay (3000);
    lcd.clear ();
    lcd.setCursor (0, 0);

    // Thermometer
    switch (bme.Read ()) {
        case 2:
            lcd.print ("AM2320 err");
            while (1);
            break;
        case 1:
            lcd.print ("AM2320 err");
            while (1);
            break;
        default:
            break;
    }

    // SD card
    if (!SD.begin (4)) {
        lcd.print ("Insert SD card");
        lcd.setCursor (0, 1);
        lcd.print ("and reset device...");
        while (1);
    }

    // Name for a file
    for (int i = 0; i < 100; i++) {
        if (i >= 10) {
            char tmp[2];
            strcpy (tmp, String (i).c_str ());
            strcpy (fileName, "PLIK");
            strcat (fileName, tmp);
            strcat (fileName, ".CSV");
        }
        else {
            strcpy (fileName, "PLIK0");
            strcat (fileName, String (i).c_str ());
            strcat (fileName, ".CSV");
        }

        if (!SD.exists (fileName)) {
            break;
        }
    }

    // If can't create file stop
    file = SD.open (fileName, FILE_WRITE);
    if (!file) {
        lcd.print ("File err");
        while (1);
    }
    file.println ("Lp Rok Miesiac Dzien Godzina Minuta Sekunda Temperatura");
    file.close ();

    // Check ethernet status
    if (!Ethernet.begin (mac)) {
        lcd.print ("Ethernet err");
        lcd.setCursor (0, 1);
        lcd.print ("Skipping...");
        delay (3000);
    }
    else {
        server.begin ();
        etherGood = true;
    }

    // Configuration file on SD CARD
    if (!SD.exists ("CONF.CSV")) {
        settingsFile = SD.open ("CONF.CSV", FILE_WRITE);
        if (!settingsFile) {
            lcd.clear ();
            lcd.setCursor (0, 0);
            lcd.print ("CONF file err");
            lcd.setCursor (0, 1);
            lcd.print ("Skipping...");
            delay (3000);
        }
        else {
            settingsFile.print (writeDelay);
            settingsFile.close ();
            lcd.clear ();
            lcd.setCursor (0, 0);
            lcd.print ("Write delay:");
            lcd.setCursor (0, 1);
            lcd.print (writeDelay);
            delay (3000);
        }
    }
    else {
        settingsFile = SD.open ("CONF.CSV");
        if (!settingsFile) {
            lcd.clear ();
            lcd.setCursor (0, 0);
            lcd.print ("CONF file err");
            lcd.setCursor (0, 1);
            lcd.print ("Skipping...");
            delay (3000);
        }
        else {
            int x = 0;
            char y;
            while (settingsFile.available ()) {
                y = settingsFile.read ();
                if (!isDigit (y)) {
                    continue;
                }
                x = 10 * x + (int)y - 48;
            }
            writeDelay = x;
            settingsFile.close ();
            lcd.clear ();
            lcd.setCursor (0, 0);
            lcd.print ("Write delay:");
            lcd.setCursor (0, 1);
            lcd.print (writeDelay);
            delay (3000);
        }
    }
    
}

void loop() {

    // Telnet
    if (etherGood) {
        if (server.available () && !connectFlag) {
            connectFlag = true;
            client = server.available ();
            client.println ("==========================");
            client.println ();
            client.println ("Thermometer by IpyZ");
            client.println ();
            client.println ("==========================");
            client.println ();
            client.println ("? for help");
            client.println ();
            printPrompt ();
        }

        if (client.connected () && client.available ()) getReceivedText ();
        if (connectFlag) checkConnectionTimeout ();
    }

    if (millis () < lastMillis + writeDelay) return;
    lastMillis = millis ();
    lcd.clear ();
    lcd.setCursor (0, 0);

    // Print temp to file
    float tmp = bme.cTemp;
    DateTime _now = rtc.now ();
    file = SD.open (fileName, FILE_WRITE);
    file.print (lp);
    file.print (" ");
    file.print (_now.year (), DEC);
    file.print (" ");
    file.print (_now.month (), DEC);
    file.print (" ");
    file.print (_now.day (), DEC);
    file.print (" ");
    file.print (_now.hour (), DEC);
    file.print (" ");
    file.print (_now.minute (), DEC);
    file.print (" ");
    file.print (_now.second (), DEC);
    file.print (" ");
    file.print (tmp);
    file.println ();
    file.close ();
    lp++;

    // Print temp to LCD
    int arrSize = 0;
    if (tmp >= 10) arrSize = 4;
    else if (tmp > 0 && tmp < 10) arrSize = 3;
    else if (tmp == 0) arrSize = 1;
    else if (arrSize < 0 && arrSize > -10) arrSize = 4;
    else arrSize = 5;

    char abba[10];
    strcpy (abba, String (tmp).c_str ());

    lcd.print ("Temperature: ");
    lcd.setCursor (0, 1);
    for (int i = 0; i < arrSize; i++) {
        lcd.print (abba[i]);
    }
    lcd.write (0);
}

// Telnet promt
void printPrompt () {
    timeOfLastActivity = millis ();
    client.flush ();
    charsReceived = 0;
    client.println ();
    if (gettingDate == false)
        client.print ("IpyZ production > ");
    else {
        if (mode == 0) {
            client.print ("Year: ");
        }
        else if (mode == 1) {
            client.print ("Month: ");
        }
        else if (mode == 2) {
            client.print ("Day: ");
        }
        else if (mode == 3) {
            client.print ("Hour: ");
        }
        else if (mode == 4) {
            client.print ("Minute: ");
        }
        else if (mode == 5) {
            client.print ("Second: ");
        }
    }
}

// If connection time out - disconect
void checkConnectionTimeout () {
    if (millis () - timeOfLastActivity > allowedConnectTime) {
        client.println ();
        client.println ("Timeout disconnect");
        client.stop ();
        connectFlag = false;
    }
}

// ------

// Get recived text
void getReceivedText () {
    char c;
    int charsWaiting = 0;

    charsWaiting = client.available ();

    do {
        c = client.read ();
        textBuff[charsReceived] = c;
        charsReceived++;
        charsWaiting--;
    } while (charsReceived <= textBuffSize && c != 0x0d && charsWaiting > 0);

    if (c == 0x0d) {
        parseReceivedText ();
        printPrompt ();
    }

    if (charsReceived >= textBuffSize) {
        errMsg ();
        printPrompt ();
    }
}

// Check commands and execute
void parseReceivedText () {
    if (gettingDate == true) {
        int x = 0;
        for (int i = 0; textBuff[i] != 0x0d && i < textBuffSize; i++) {
            if (!isDigit (textBuff[i])) {
                if (i == 0) continue;
                else {
                    gettingDate = false;
                    client.println ("Format err");
                    return;
                }
            }
            x = (x * 10) + (int)textBuff[i] - 48;
        }

        if (mode == 0) m_year = x;
        else if (mode == 1) m_month = x;
        else if (mode == 2) m_day = x;
        else if (mode == 3) m_hour = x;
        else if (mode == 4) m_minute = x;
        else if (mode == 5) m_second = x;

        mode++;

        if (mode == 6) {
            gettingDate = false;

            rtc.adjust (DateTime (m_year, m_month, m_day, m_hour, m_minute, m_second));

            DateTime _now = rtc.now ();

            client.print (_now.year (), DEC);
            client.print ("/");
            client.print (_now.month (), DEC);
            client.print ("/");
            client.print (_now.day (), DEC);
            client.print (" ");
            client.print (_now.hour (), DEC);
            client.print (":");
            client.print (_now.minute (), DEC);
            client.print (":");
            client.print (_now.second (), DEC);
            client.println ();
        }
        return;
    }
    switch (textBuff[0]) {
        case 'm': editWriteDelay (0);   break;
        case 'b': getTemperature ();    break;
        case 'c': closeConnection ();   break;
        case 'f': cleanScreen ();       break;
        case 's': setClock ();          break;
        case '?': getHelp ();           break;
        case 0x0d:                      break;
        default: 
            switch (textBuff[1]) {
                case 'm': editWriteDelay (1);   break;
                case 'b': getTemperature ();    break;
                case 'c': closeConnection ();   break;
                case 'f': cleanScreen ();       break;
                case 's': setClock ();         break;
                case '?': getHelp ();           break;
                case 0x0d:                      break;
                default: errMsg ();             break;
            }
    }
}

// ------

// Edit write delay via telnet
void editWriteDelay (int y) {
    if (textBuff[1 + y] != ' ') {
        errMsg ();
        return;
    }

    int x = 0;

    if (textBuff[2 + y] == 'd') {
        if (SD.exists ("CONF.CSV")) SD.remove ("CONF.CSV");
        writeDelay = 5000;
        return;
    }

    for (int i = 2 + y; textBuff[i] != 0x0d && i < textBuffSize; i++) {
        if (!isDigit (textBuff[i])) {
            break;
        }
        x = x * 10 + (int)textBuff[i] - 48;
    }

    writeDelay = x;

    if (SD.exists ("CONF.CSV")) {
        SD.remove ("CONF.CSV");
    }

    settingsFile = SD.open ("CONF.CSV", FILE_WRITE);
    if (settingsFile) {
        settingsFile.print (writeDelay);
        settingsFile.close ();
    }
    else {
        client.println ("CONF file err");
    }
}

// Display temperatur via telnet

void getTemperature () {
    bme.Read ();
    client.println (bme.cTemp);
}

// Get commands via telnet
void getHelp () {
    client.println ("==========================");
    client.println ();
    client.println ("Help:");
    client.println ("m [milliseconds] - sets write delay");
    client.println ("m d - sets write delay to default value (5000)");
    client.println ("b - displays actual temperature");
    client.println ("c - closes the connection");
    client.println ("f - clear screen");
    client.println ("s - adjust clock");
    client.println ("? - displays this message");
    client.println ();
    client.println ("==========================");
    client.println ();
}

// Close telnet connection
void closeConnection () {
    client.println ("==========================");
    client.println ();
    client.println ("Closing connection...");
    client.println ();
    client.println ("==========================");
    client.println ();
    client.stop ();
    connectFlag = false;
}

// Send err message via telnet
void errMsg () {
    client.println ("Unrecognized command\n? for help");
}

// Clean terminal via telnet
void cleanScreen () {
    for (int i = 0; i < 100; i++) {
        client.println ();
    }
}

// Set clock via telnet
void setClock () {
    
    mode = 0;
    gettingDate = true;

}