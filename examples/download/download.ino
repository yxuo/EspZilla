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

//? default values: timeout = 10000; retry = 2; verbose = 1; port = 21);
Espzilla ftp(ftp_server, ftp_user, ftp_pass, 1000, 2, 2, 21);

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

    // ?BOTH download functions handle TYPE_A or TYPE_I

    ftp.getSize("/octocat.jpg");
    Serial.println("done");
    return;
    // Download String
    ftp.writeString("buffer_doc", "/Espzilla/file.txt");
    ftp.closeConnection();

    Serial.println("\nDownload string");
    // Routine commands
    Serial.println("\n#1");
    ftp.openConnection();
    ftp.changeWorkDir("/Espzilla");
    ftp.initFile("Type A");
    String response = "";
    //?                         set one command to false v
    ftp.downloadString("file.txt", NULL, response, ftp.TYPE_A, false); // <--
    ftp.closeFile();

    if (ftp.isConnected())
        Serial.println("Command successfull\n");
    else
        Serial.println("Command failed\n");

    Serial.println("The file content is: '" + response + "'");
    ftp.writeString(response.c_str(), "/Espzilla/file_A.txt");

    ftp.closeConnection();

    // one-time command
    Serial.println("\n#2");
    response = "";
    if (ftp.downloadString("/Espzilla/file.txt", NULL, response)) // <--
        Serial.println("Command successfull\n");
    else
        Serial.println("Command failed\n");
    Serial.println("The file content is: '" + response + "'");
    ftp.writeString(response.c_str(), "/Espzilla/file_B.txt");
    //
    Serial.println("\n#3");
    response = "";
    ftp.downloadString("file.txt", "/Espzilla", response); // <--
    if (ftp.isConnected())
        Serial.println("Command successfull\n");
    else
        Serial.println("Command failed\n");
    Serial.println("The file content is: '" + response + "'");
    ftp.writeString(response.c_str(), "/Espzilla/file_C.txt");
    //
    response = "";
    Serial.println("\n#4");
    ftp.downloadString("file.txt", "/Espzilla", response, ftp.TYPE_A, true); // <--
    if (ftp.isConnected())
        Serial.println("Command successfull\n");
    else
        Serial.println("Command failed\n");
    Serial.println("The file content is: '" + response + "'");
    ftp.writeString(response.c_str(), "/Espzilla/file_D.txt");
    //
    ftp.closeConnection();

    // Download data
    ftp.writeData("/Espzilla/octocat.jpg", NULL, octocat_pic, sizeof(octocat_pic));
    ftp.closeConnection();

    Serial.println("\nDownload data");
    { // Routine commands
        Serial.println("\n#1");
        ftp.openConnection();
        ftp.changeWorkDir("/Espzilla");
        size_t file_size = ftp.getSize("octocat.jpg");
        unsigned char *file_data = (unsigned char *)malloc(file_size);
        ftp.initFile("Type I");
        ftp.downloadData("octocat.jpg", NULL, file_data, file_size, NULL, ftp.TYPE_I, false); // <--

        if (ftp.isConnected())
            Serial.println("Command successfull\n");
        else
            Serial.println("Command failed\n");

        ftp.writeData("/Espzilla/octocat_1.jpg", NULL, file_data, file_size);
        free(file_data);
    }
    ftp.closeConnection();

    // One-time command
    {
        Serial.println("\n#2");
        size_t file_size = ftp.getSize("/Espzilla/octocat.jpg");
        unsigned char *file_data = (unsigned char *)malloc(file_size);
        if (ftp.downloadData("/Espzilla/octocat.jpg", NULL, file_data, file_size, handleData)) // <--
            Serial.println("Command successfull\n");
        else
            Serial.println("Command failed\n");
        ftp.writeData("/Espzilla/octocat_2.jpg", NULL, file_data, file_size);
        free(file_data);
    }
    //
    {
        Serial.println("\n#3");
        size_t file_size = ftp.getSize("/Espzilla/octocat.jpg");
        unsigned char *file_data = (unsigned char *)malloc(file_size);
        ftp.downloadData("octocat.jpg", "/Espzilla", file_data, file_size); // <--
        if (ftp.isConnected())
            Serial.println("Command successfull\n");
        else
            Serial.println("Command failed\n");
        ftp.writeData("/Espzilla/octocat_3.jpg", NULL, file_data, file_size);
        free(file_data);
    }
    //
    {
        Serial.println("\n#4");
        size_t file_size = ftp.getSize("/Espzilla/octocat.jpg");
        unsigned char *file_data = (unsigned char *)malloc(file_size);
        ftp.downloadData("octocat.jpg", "/Espzilla", file_data, file_size, NULL, ftp.TYPE_I, true); // <--
        if (ftp.isConnected())
            Serial.println("Command successfull\n");
        else
            Serial.println("Command failed\n");
        ftp.writeData("/Espzilla/octocat_4.jpg", NULL, file_data, file_size);
        free(file_data);
    }
    //
    ftp.closeConnection();
    //*/
    Serial.println("done");
}

// Handling functions make it possible to parse data byte per byte to memory, Serial port or other purpose.
// This is usefull for very large files or for the convenience of not using ram to create  buffer for the whole file.
void handleData(unsigned char data, size_t len, size_t pos)
{
    if (pos == 0)
    {
        Serial.print("starting parse data: len = ");
        Serial.println(len);
    }
    //
    Serial.print("pos = ");
    Serial.print(pos);
    Serial.print(" char = ");
    Serial.println(data);

    if (pos == len - 1)
    {
        Serial.print("parse finished: ");
        Serial.println(data);
    }
}

void loop()
{
}