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
Espzilla ftp(ftp_server, ftp_user, ftp_pass, 1000, 2, 3, 21);

void mult(String &tois)
{
    //
}

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

    // ftp.getSize("/Espzilla/.in.void.txt.");
    // ftp.getList("/Espzilla", NULL, ftp.ITEM_LIST);
    size_t siz = 2147483648;
    Serial.printf("siz <%d>\n", siz);
    Serial.println(sizeof(siz));
    Serial.println("Done");
    return;
      
    // make dir
    Serial.println("\n Make dir");
    if (ftp.makeDir("/Espzilla/temp/folder")) // <-
    {
        Serial.println("Make dir successfull\n");
        ftp.writeString("/Espzilla/temp/file.txt", NULL, "content");
        ftp.writeString("/Espzilla/temp/folder/file.txt", NULL, "content");
    }
    else
        Serial.println("Make dir failed\n");

    // rename/move file
    Serial.println("\n rename/move file");
    if (ftp.rename("/Espzilla/temp/file.txt", "/Espzilla/temp/file_renamed.txt")) // <-
        Serial.println("Rename file successfull\n");
    else
        Serial.println("Rename dir failed\n");

    // TODO copy file
    // there is no knew working FTP command for copy a file..

    // remove dir
    Serial.println("\n remove dir");
    if (ftp.removeDir("/Espzilla/temp")) // <-
        Serial.println("Remove dir successfull\n");
    else
        Serial.println("Remove dir failed\n");

    // ftp.removeDir("/Espzilla/nova");
    Serial.println("done");
}

void loop()
{
}

// Handling functions make it possible to parse data byte per byte to memory, Serial port or other purpose.
// This is usefull for very large files or for the convenience of not using ram to create  buffer for the whole file.
void handleContent(char *data, size_t len, size_t pos)
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
        Serial.print(" char = ");
        Serial.println(data);
    }
    else if (pos > 0)
    {
        Serial.println("parse finished");
    }
}
