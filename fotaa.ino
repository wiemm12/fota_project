#include <LittleFS.h>
#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include <PubSubClient.h>
#include <addons/TokenHelper.h>

/**************************************************  MACROS - Variables  ****************************************************/
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#elif __has_include(<WiFiNINA.h>)
#include <WiFiNINA.h>
#elif __has_include(<WiFi101.h>)
#include <WiFi101.h>
#elif __has_include(<WiFiS3.h>)
#include <WiFiS3.h>
#endif

/* 1. Define the WiFi credentials */
#define WIFI_SSID             "Fixbox-790DC2"
#define WIFI_PASSWORD         "ZjlhZGE0"

/* 2. Define the API Key */
#define API_KEY               "AIzaSyApO9b80C6vxFvABEDm5wASg3zVPlk6IBU"

/* 3. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL            "chebbi.wiemm@gmail.com"
#define USER_PASSWORD         "PSG12@12!"

/* 4. Define the Firebase storage bucket ID e.g bucket-name.appspot.com */
#define STORAGE_BUCKET_ID     "fota-62302.appspot.com"

#define NOT_ACKNOWLEDGE       0xAB
#define JUMP_SECCEDDED        0x01
#define ERASE_SUCCEDDED       0x03
#define WRITE_SUCCEDDED       0x01

const char* mqtt_server = "test.mosquitto.org";

/* Command packet in communication with STM32 */
uint8_t packet[100];          

// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
WiFiMulti multi;
#endif

WiFiClient espClient;
PubSubClient client(espClient);

/******************************************************    Setup    **********************************************************/
void setup()
{
    Serial.begin(115200);
    Serial1.begin(115200, SERIAL_8N1);

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    multi.addAP(WIFI_SSID, WIFI_PASSWORD);
    multi.run();
#else
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
#endif

    Serial.print("Connecting to Wi-Fi");
    unsigned long ms = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
        if (millis() - ms > 10000)
            break;
#endif
    }
    Serial.println();
    Serial.println("Connected");

    Serial.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);

    /* Assign the api key (required) */
    config.api_key = API_KEY;

    /* Assign the user sign in credentials */
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    config.wifi.clearAP();
    config.wifi.addAP(WIFI_SSID, WIFI_PASSWORD);
#endif

    /* Assign the callback function for the long running token generation task */
    config.token_status_callback = tokenStatusCallback; 

    // Comment or pass false value when WiFi reconnection will control by your code or third party library e.g. WiFiManager
    Firebase.reconnectNetwork(true);
    fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);
    /* Assign download buffer size in byte */
    config.fcs.download_buffer_size = 2048;
    Firebase.begin(&config, &auth);

    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);

    pinMode(2, OUTPUT);
}

/****************************************************    Loop    ************************************************************/
void loop()
{
    if(!client.connected()) { reconnect(); }
    client.loop();
}


/************************************ The Firebase Storage download callback function ****************************************/
void fcsDownloadCallback(FCS_DownloadStatusInfo info)
{
    if (info.status == firebase_fcs_download_status_init)
    {
        Serial.printf("Downloading file %s (%d) to %s\n", info.remoteFileName.c_str(), info.fileSize, info.localFileName.c_str());
    }
    else if (info.status == firebase_fcs_download_status_download)
    {
        Serial.printf("Downloaded %d%s, Elapsed time %d ms\n", (int)info.progress, "%", info.elapsedTime);
    }
    else if (info.status == firebase_fcs_download_status_complete)
    {
        Serial.println("Download completed\n");
    }
    else if (info.status == firebase_fcs_download_status_error)
    {
        Serial.printf("Download failed, %s\n", info.errorMsg.c_str());

      
       
    }
}


/****************************************************** Reconnect to MQTT *****************************************************/
void reconnect() 
{ 
  while(!client.connected()) 
  {
      Serial.println("Attempting MQTT connection...");

      if(client.connect("ESPClient")) 
      {
          Serial.println("Connected");
          client.subscribe("/FOTA/Boot");
      } 
      else 
      {
          Serial.print("Failed, rc=");
          Serial.print(client.state());
          Serial.println(" try again in 5 seconds");
          delay(5000);
      }
    }
}


/******************************************************** Node-Red Callback ****************************************************/
void callback(char* topic, byte* payload, unsigned int length) 
{ 
    byte recCommand = 0;
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("]: ");
    recCommand = payload[0];
    Serial.println(recCommand);
    
    switch(recCommand)
    {
      case 'G':  Send_CMD_Read_CID();               break;
      case 'R':  Send_CMD_Read_Protection_Level();  break;
      case 'J':  Send_CMD_Jump_To_Application();    break;
      case 'E':  Send_CMD_Erase_Flash();            break;
      case 'U':  Send_CMD_Upload_Application();     break;
    }
}


/******************************************** Communication with bootloader *****************************************************/
void SendCMDPacketToBootloader(uint8_t packetLength)
{
    for(uint8_t dataIndex = 0 ; dataIndex < packetLength ; dataIndex++){
        Serial1.write(packet[dataIndex]);
        Serial.print(packet[dataIndex], HEX);
        Serial.print(" ");
    }
    Serial.println("\n");
}

void ReceiveReplayFromBootloader(uint8_t packetLength)
{
    for(uint8_t dataIndex = 0 ; dataIndex < packetLength ; dataIndex++)
    {
        while(!Serial1.available());
        packet[dataIndex] = Serial1.read();
        if(NOT_ACKNOWLEDGE == packet[0]) break;
    }
}


/***************************************** Bootloader Supported Commands' Functions ********************************************/
void Send_CMD_Read_CID()
{
    char Replay[16] = {0};
    String sReplay = "Chip ID: 0x";
    
    packet[0] = 5;
    packet[1] = 0x10;
    *((uint32_t*)((uint8_t*)packet + 2)) = calculateCRC32((uint8_t*)packet, 2);
    SendCMDPacketToBootloader(6);
    ReceiveReplayFromBootloader(2);

    if(NOT_ACKNOWLEDGE == packet[0]){ sReplay = "NACK"; }
    else{ sReplay += String(packet[1], HEX);   sReplay += String(packet[0], HEX); }

    sReplay.toCharArray(Replay, sReplay.length()+1);
    client.publish("/FOTA/BootReply", Replay, false); 
}

void Send_CMD_Read_Protection_Level()
{
    char Replay[50] = {0};
    String sReplay = "ProtecLvl: ";

    packet[0] = 5;
    packet[1] = 0x11;
    *((uint32_t*)((uint8_t*)packet + 2)) = calculateCRC32((uint8_t*)packet, 2);
    SendCMDPacketToBootloader(6);
    ReceiveReplayFromBootloader(1);

    if(NOT_ACKNOWLEDGE == packet[0]){ sReplay = "NACK"; }
    else{ sReplay += String(packet[0], HEX); }

    sReplay.toCharArray(Replay, sReplay.length()+1);
    client.publish("/FOTA/BootReply", Replay, false); 
}

void Send_CMD_Jump_To_Application()
{
    char Replay[16] = {0};
    String sReplay = "";
    packet[0] = 5;
    packet[1] = 0x12;
    *((uint32_t*)((uint8_t*)packet + 2)) = calculateCRC32((uint8_t*)packet, 2);
    SendCMDPacketToBootloader(6);
    ReceiveReplayFromBootloader(1);

    if(JUMP_SECCEDDED == packet[0]){ sReplay += "Jump Success"; }
    else{ sReplay += "Jump Fail"; }

    sReplay.toCharArray(Replay, sReplay.length()+1);
    client.publish("/FOTA/BootReply", Replay, false); 
}

void Send_CMD_Erase_Flash()
{
    char Replay[50] = {0};
    String sReplay = "";
    packet[0] = 5;
    packet[1] = 0x13;
    *((uint32_t*)((uint8_t*)packet + 2)) = calculateCRC32((uint8_t*)packet, 2);
    SendCMDPacketToBootloader(6);
    ReceiveReplayFromBootloader(1);

    if(ERASE_SUCCEDDED == packet[0]){ sReplay += "Erase Success";  }
    else{ sReplay += "Erase Unsuccess"; }

    sReplay.toCharArray(Replay, sReplay.length()+1);
    client.publish("/FOTA/BootReply", Replay, false); 
}

void Send_CMD_Upload_Application()
{
    uint8_t isFailed = 0;
    if (Firebase.ready())
    {
        Serial.println("\nDownload file...\n");

        if (!Firebase.Storage.download(&fbdo, STORAGE_BUCKET_ID, "application.bin", "/update.bin", mem_storage_type_flash, fcsDownloadCallback))
        {
            Serial.println(fbdo.errorReason());
            return;
        }

        File file = LittleFS.open("/update.bin", "r");
        short addressCounter = 0;

        while (file.available())
        {
            char Replay[32] = {0};
            String sReplay = "";

            digitalWrite(2, !digitalRead(2));  // Toggle LED

            uint8_t ByteCounter = 0;
            packet[0] = 74;                           // Total packet length
            packet[1] = 0x14;                         // Write memory command
            *(int*)(&(packet[2])) = 0x08004000U + (0x40 * addressCounter);  // Write address
            packet[6] = 64;                           // Number of bytes to write

            Serial.printf("Start Address: 0x%X\n", *(int*)(&(packet[2])));

            while (ByteCounter < 64 && file.available())
            {
                packet[7 + ByteCounter] = file.read();
                ByteCounter++;
            }

            *((uint32_t*)((uint8_t*)packet + 71)) = calculateCRC32((uint8_t*)packet, 71);

            SendCMDPacketToBootloader(75);
            ReceiveReplayFromBootloader(1);

            if (WRITE_SUCCEDDED == packet[0])
            {
                sReplay = "Write Success @Block: " + String(addressCounter);
            }
            else
            {
                sReplay = "Write Fail @Block: " + String(addressCounter);
                isFailed = 1;
                break;
            }

            addressCounter++;
            sReplay.toCharArray(Replay, sReplay.length() + 1);
            client.publish("/FOTA/BootReply", Replay, false);
        }

        file.close();

        if (!isFailed)
        {
            client.publish("/FOTA/BootReply", "Upload Complete", false);
        }
        else
        {
            client.publish("/FOTA/BootReply", "Upload Failed", false);
        }

        digitalWrite(2, LOW);  // Ensure LED is off
    }
}
uint32_t calculateCRC32(uint8_t* data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFF;
    for (uint32_t i = 0; i < length; i++)
    {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc = crc >> 1;
        }
    }
    return crc ^ 0xFFFFFFFF;
}
