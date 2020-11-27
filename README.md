# FTP Client for the ESP32

[![License: MIT](https://img.shields.io/badge/License-MIT-green)](https://github.com/kzasca/ESP32-FTP/blob/master/LICENSE)
[![GitHub last commit](https://img.shields.io/github/last-commit/kzasca/ESP32-FTP.svg?style=social)](https://github.com/kzasca/ESP32-FTP)

This library send and receive any files through FTP (up to 90KB).

It provides features such as connection stability and reability, automatization of all FTP communication, improvement of old commands from the forked library, and some modifications.


## Features

- [ ] One-time commands
- [x] Auto reconnect (for one-time commands)
- [x] contentList (MLSD) with flexible parameters, allowing to list a limited number of result among a large number of files
- [ ] Save contentList (MLSD) to SPIFFS, useful large number of files. Read all content at once from SPIFFS. 
- [x] getLastModifiedTime - MDTM
- [x] Verbose 3 - for debug each FTP response, besides the others
- [x] Debug upload/download time and measured velocity.

## Upload example

* For the uploading example we will use the GitHub Octocat, which binary file is [here](./src/octocat.h):

 <img src="https://github.githubassets.com/images/modules/logos_page/Octocat.png" alt="Octocat" width="50%"> 
 
* Forked from [https://github.com/ldab/ESP32_FTP](https://github.com/ldab/ESP32_FTP)
