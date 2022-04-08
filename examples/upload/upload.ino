/**
 * @file upload.ino
 * @author Raphael Rivas (raphaelrivas@hotmail.com)
 * @brief Feel free to improve it and pull request
 * @version 0.2
 * @date 2022-04-08
 * 
 * @copyright Distributed as-is; no warranty is given.
 * 
 * https://github.com/yxuo/EspZilla
 */

#include "Arduino.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <espzillah>
#include "octocat.h"

char wifi_ssid[] = "SSID";
char wifi_pass[] = "PASSWORD1234";

// DONT include 'ftp.' in ftp_server, it's not necessary and it will not work.
char ftp_server[] = "my_ftp_site.com";
char ftp_user[] = "ftp_user";
char ftp_pass[] = "ftp_pass";

// default values: timeout = 10000; retry = 2; verbose = 1; port = 21);
Espzilla ftp(ftp_server, ftp_user, ftp_pass, 1000, 2, 2);

void setup()
{
    Serial.begin(115200);

    //wifi
    Serial.println("Starting WiFi");
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(wifi_ssid, wifi_pass);
    while (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
        delay(500);
        if (WiFi.waitForConnectResult() != WL_CONNECTED)
        {
            Serial.print(".");
            WiFi.begin(wifi_ssid, wifi_pass);
        }
    }
    Serial.println();
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Upload String
    Serial.println("\nUpload string");
    // Routine commands
    ftp.openConnection();
    ftp.changeWorkDir("/Espzilla");
    ftp.initFile("Type A");
    ftp.newFile("fileA.txt");
    ftp.writeString("teste", NULL, NULL, false); // <--
    ftp.closeFile();

    if (ftp.isConnected())
        Serial.println("Command successfull\n");
    else
        Serial.println("Command failed\n");

    ftp.closeConnection();

    // One-time command
    if (ftp.writeString("buffer_doc", "/Espzilla/fileB.txt"))
        Serial.println("Command successfull\n");
    else
        Serial.println("Command failed\n");
    //
    ftp.writeString("buffer_doc", "fileC.jpeg", "/Espzilla");

    if (ftp.isConnected())
        Serial.println("Command successfull\n");
    else
        Serial.println("Command failed\n");
    //
    ftp.writeString("buffer_doc", "fileD.txt", "/Espzilla", ftp.TYPE_A);

    if (ftp.isConnected())
        Serial.println("Command successfull\n");
    else
        Serial.println("Command failed\n");
    //
    ftp.closeConnection();

    // Upload image
    Serial.println("\nUpload image");
    // Routine commands
    ftp.openConnection();
    ftp.changeWorkDir("/Espzilla");
    ftp.initFile("Type I");
    ftp.newFile("octocat1.jpg");
    ftp.writeData(octocat_pic, sizeof(octocat_pic), NULL, NULL, false); // <--
    ftp.closeFile();
    //
    if (ftp.isConnected())
        Serial.println("Command successfull\n");
    else
        Serial.println("Command failed\n");
    //
    ftp.closeConnection();

    // One-time command
    if (ftp.writeData(octocat_pic, sizeof(octocat_pic), "/Espzilla/octocat2.jpg"))
        Serial.println("Command successfull\n");
    else
        Serial.println("Command failed\n");
    //
    ftp.writeData(octocat_pic, sizeof(octocat_pic), "octocat3.jpg", "/Espzilla");

    if (ftp.isConnected())
        Serial.println("Command successfull\n");
    else
        Serial.println("Command failed\n");
    //
    ftp.writeData(octocat_pic, sizeof(octocat_pic), "octocat4.jpg", "/Espzilla", true, ftp.TYPE_I);

    if (ftp.isConnected())
        Serial.println("Command successfull\n");
    else
        Serial.println("Command failed\n");
    //
    ftp.closeConnection();
}

void loop()
{
}