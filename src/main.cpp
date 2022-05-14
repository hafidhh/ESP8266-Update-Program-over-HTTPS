#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>
#include "certs.h"
 
const String FirmwareVer={"1.9"};
unsigned long previousMillis_2 = 0;
unsigned long previousMillis = 0;

void ConnectWiFi()
{
    // change ssid and password
    const char* ssid = "your ssid";
    const char* password = "password";
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password); //connecting to wifi
    while (WiFi.status() != WL_CONNECTED)// while wifi not connected
    {
        delay(500);
        Serial.print("."); //print "...."
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println(WiFi.localIP());  // Print the IP address
}

void setClock()
{
    // Set time via NTP, as required for x.509 validation
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println("Waiting for NTP time sync");
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    Serial.println("");
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    Serial.print("Current time: ");
    Serial.println(asctime(&timeinfo));
}

void FirmwareUpdate()
{
    X509List cert(cert_DigiCert_Global_Root_CA);
    //change URL_fw_Version and URL_fw_Bin with your bin file and version url.
    #define URL_fw_Version "/hafidh7/ESP8266-Update-Program-over-HTTPS/master/version.txt"
    #define URL_fw_Bin "https://raw.githubusercontent.com/hafidh7/ESP8266-Update-Program-over-HTTPS/master/.pio/build/nodemcuv2/firmware.bin"
    WiFiClientSecure client;

    Serial.println("Cek Firmware Update");
    client.setTrustAnchors(&cert);
    if (!client.connect(firmware_host, firmware_port)) {
        Serial.print("Failed Connecting to ");
        Serial.println(firmware_host);
        return;
    }
    client.print(String("GET ") + URL_fw_Version + " HTTP/1.1\r\n" + "Host: " + firmware_host + "\r\n" + "User-Agent: BuildFailureDetectorESP8266\r\n" + "Connection: close\r\n\r\n");
    while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
            //Serial.println("Headers received");
            break;
        }
    }
    String payload = client.readStringUntil('\n');
    payload.trim();
    if(payload.equals(FirmwareVer)) {
        Serial.println("Device already on latest firmware version");
    }
    else {
        Serial.println("New firmware detected");
        Serial.println("Current firmware version "+FirmwareVer);
        Serial.println("Firmware version "+payload+" is avalable");
        ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
        t_httpUpdate_return ret = ESPhttpUpdate.update(client, URL_fw_Bin);
        Serial.println("Update firmware to version "+payload);
        switch (ret) {
            case HTTP_UPDATE_FAILED:
            Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
            break;

            case HTTP_UPDATE_NO_UPDATES:
            Serial.println("HTTP_UPDATE_NO_UPDATES");
            break;

            case HTTP_UPDATE_OK:
            Serial.println("HTTP_UPDATE_OK");
            break;
        }
    }
}

void setup()
{
    Serial.begin(115200);
    ConnectWiFi();
    pinMode(LED_BUILTIN, OUTPUT);
    setClock();
    FirmwareUpdate();
}

void loop()
{
    const long interval = 60000;
    const long mini_interval = 10000;
    unsigned long currentMillis = millis();
    if ((currentMillis - previousMillis) >= interval) {
        //Cek WiFi
        if (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0, 0, 0, 0))
        {
            //Reconnect WiFi
            Serial.println("Reconnect WiFi");
            WiFi.reconnect();
        }
        else
        {
            // ESP will cek firmware update every 60000 milisecond   
            setClock();
            FirmwareUpdate();
            previousMillis = currentMillis;
        } 
    }
    if ((currentMillis - previousMillis_2) >= mini_interval) {
        // LED
        // save the last time you blinked the LED
        static int idle_counter=0; 
        Serial.print(" Active fw version:");
        Serial.println(FirmwareVer);
        Serial.print("Idle Loop....");
        Serial.println(idle_counter++);
        previousMillis_2 = currentMillis;
        if (idle_counter%2==0)
            digitalWrite(LED_BUILTIN, HIGH);
        else 
            digitalWrite(LED_BUILTIN, LOW);
   }
}