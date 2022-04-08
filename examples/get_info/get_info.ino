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
#include <SPIFFS.h>

char wifi_ssid[] = "SSID";
char wifi_pass[] = "PASSWORD1234";

// DONT include 'ftp.' in ftp_server, it's not necessary and it will not work.
char ftp_server[] = "my_ftp_site.com";
char ftp_user[] = "ftp_user";
char ftp_pass[] = "ftp_pass";

//? default values: timeout = 10000; retry = 2; verbose = 1; port = 21);
Espzilla ftp(ftp_server, ftp_user, ftp_pass, 1000, 2, 2, 21);
File temp;

void setup()
{
    Serial.begin(115200);
    SPIFFS.begin();

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

    Serial.print("ini");
    SPIFFS.remove("/temp.txt");
    temp = SPIFFS.open("/temp.txt", FILE_WRITE);
    ftp.getList("Espzilla", NULL, ftp.NAME_LIST, NULL, handleInfo);
    temp.close();
    // // read saved
    File file = SPIFFS.open("/temp.txt", FILE_READ);
    Serial.println("file: ");
    Serial.println(file.size());
    Serial.println(file.readString());
    // int br = 0;
    // for (int i = 0; i < file.size(); i++)
    // {
    //     Serial.print(file.peek());
    //     file.seek(i);
    //     br += 1;
    // }

    Serial.print("fim");
    return;

    //* content list
    // ? get list of files inside folder
    Serial.println("\nIni");

    {
        Serial.println("\n# getList DIR_INFO");
        String strs[2]{};
        ftp.getList("Espzilla", NULL, ftp.DIR_INFO, strs, handleInfo, 0, 2);
        Serial.printf("string: <%s>\n", strs[0].c_str());
    }
    {
        Serial.println("\n#2 getList DIR_LIST");
        String strs[5]{"str ini"};
        ftp.getList("Espzilla", NULL, ftp.DIR_LIST, strs, handleInfo, 5, 10);
        Serial.printf("string: <%s>\n", strs[0].c_str());
    }
    {
        Serial.println("\n#3 getList ITEM_LIST");
        String strs[12]{"str ini"};
        ftp.getList("Espzilla", NULL, ftp.ITEM_LIST, strs, handleInfo, 0, 12);
        Serial.printf("string: <%s>\n", strs[0].c_str());
    }
    {
        Serial.println("\n#4 getList NAME_LIST");
        String strs[12]{"str ini"};
        ftp.getList("Espzilla", NULL, ftp.NAME_LIST, strs, handleInfo, 0, 12);
        Serial.printf("string: <%s>\n", strs[0].c_str());
    }
    //* last modify time
    Serial.println("\nlast modify time");

    char resp[16]{};
    if (ftp.getLastModifiedTime("/Espzilla/file.txt", NULL, resp)) // <-
        Serial.printf("modify: %s\n", resp);
    else
        Serial.println("fail");

    //* size of file
    Serial.println("\nsize of file");

    size_t f_size = 0;
    f_size = ftp.getSize("/Espzilla/file.txt"); // <-
    if (ftp.isConnected())
        Serial.printf("size: %i\n", f_size);
    else
        Serial.println("fail");
    //*/
    Serial.println("done");
}

void loop()
{
}

// Handling functions make it possible to parse data byte per byte to memory, Serial port or other purpose.
// This is usefull for very large files or for the convenience of not using ram to create  buffer for the whole file.
void handleInfo(String str, size_t pos)
{
    // Serial.println("spr");
    if (str.length())
    {
        if (pos == 0)
        {
            // Serial.println("starting parse data");
        }
        //
        // Serial.print("pos = ");
        // Serial.print(pos);
        // Serial.print(" char = '");
        // Serial.print(str);
        // Serial.println("'");
        // temp.seek(EOF);
        temp.print(str);
    }
    else if (pos > 0)
    {
        // Serial.println("parse finished");
    }
}

void handleContent(const char *data, size_t len, size_t pos)
{
    // Serial.println("spr");
    if (data)
    {
        if (pos == 0)
        {
            Serial.println("starting parse data");
        }
        //
        Serial.print("pos = ");
        Serial.print(pos);
        Serial.print(" char = '");
        Serial.print(data);
        Serial.println("'");
    }
    else if (pos > 0)
    {
        Serial.println("parse finished");
    }
}
