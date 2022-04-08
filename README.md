# FTP Client for the ESP32

[![License: MIT](https://img.shields.io/badge/License-MIT-green)](https://github.com/yxuo/EspZilla/blob/master/LICENSE)
[![GitHub last commit](https://img.shields.io/github/last-commit/yxuo/EspZilla.svg?style=social)](https://github.com/yxuo/EspZilla)

This library send and receive any files through FTP (up to 90KB).

## Features

Some content and basic functions are from the [ancestor project](https://github.com/ldab/ESP32_FTPClient), the features covered are from this library. 

### Automatization of all FTP commands

The forked libraries I tested (`2022/04/08`) uses 4, 5 functions to do one simple FTP task.

This project automated these little steps inside big final functions, like:sss

* Upload/download binary;

* Upload/download text;

* Create folder;
  
  and so on...

### Connection stability and reliability:

Automating functions let the library take more control of what is happaning, why the connection fails.

Also it can retry to do the task if connection fails.

In the end, the library will try the best possible to execute the task, if it fails you will be assured that the problem is a server or connection error.

## Todo

- [x] One-time commands
- [x] Auto reconnect (for one-time commands)
- [x] contentList (MLSD) with flexible parameters, allowing to list a limited number of result among a large number of files
- [x] Save contentList (MLSD) recursively. 
- [x] getLastModifiedTime - MDTM
- [x] Verbose 3 - for debug each FTP response, besides the others
- [x] Debug upload/download time and measured velocity
- [x] Force create dir (recursively)
- [ ] Detailed documentation

## Examples

You will find examples using both the legacy routine-commands and one-command, so if you are coming from the other Esp32-FTP libraries, you will have time to adapt and test.

### Upload example

* For the uploading example we will use the GitHub Octocat, which binary file is [here](./src/octocat.h):
  
  <img src="https://github.githubassets.com/images/modules/logos_page/Octocat.png" alt="Octocat" width="50%"> 

---

* Forked from [https://github.com/ldab/ESP32_FTP](https://github.com/ldab/ESP32_FTPClient)
