#include <WiFiClient.h>
#include "ESP32_FTPClient.h"

ESP32_FTPClient::ESP32_FTPClient(char *_serverAdress, char *_userName, char *_passWord, uint16_t _timeout, uint8_t _verbose)
{
  userName = _userName;
  passWord = _passWord;
  serverAdress = _serverAdress;
  timeout = _timeout;
  verbose = _verbose;
}

WiFiClient *ESP32_FTPClient::GetDataClient()
{
  return &dclient;
}

bool ESP32_FTPClient::isConnected(bool _verbose)
{
  if (!_isConnected && _verbose)
  {
    FTPerr("FTP error: ");
    FTPerr(outBuf);
    FTPerr("\n");
  }

  return _isConnected;
}

void ESP32_FTPClient::getLastModifiedTime(const char *fileName, char *result)
{
  char _resp[sizeof(outBuf)];
  FTPdbgn("Send MDTM");
  if (!isConnected())
    return;
  client.print(F("MDTM "));
  client.println(F(fileName));
  getFTPAnswer(_resp);
  _resp[strlen(_resp) - 1] = 0;
  char *ptr_resp = &_resp[4];
  strcpy(result, ptr_resp);
}

size_t ESP32_FTPClient::getSize(const char *fileName)
{
  FTPdbgn("Send TYPE");
  if (!isConnected())
    return 0;
  FTPdbgn("type I");
  client.println(F("type I"));
  getFTPAnswer();

  FTPdbgn("Send SIZE ");
  client.print(F("SIZE "));
  client.println(F(fileName));
  char _resp[sizeof(outBuf)];
  getFTPAnswer(_resp);

  _resp[strlen(_resp) - 1] = 0;
  char *ptr_resp = &_resp[4];
  return atoi(ptr_resp);
}

void ESP32_FTPClient::WriteClientBuffered(WiFiClient *cli, unsigned char *data, int dataLength)
{
  if (!isConnected())
    return;

  size_t clientCount = 0;
  for (int i = 0; i < dataLength; i++)
  {
    clientBuf[clientCount] = data[i];
    //client.write(data[i])
    clientCount++;
    if (clientCount > bufferSize - 1)
    {
      cli->write(clientBuf, bufferSize);
      clientCount = 0;
    }
  }
  if (clientCount > 0)
  {
    cli->write(clientBuf, clientCount);
  }
}

void ESP32_FTPClient::getFTPAnswer(char *result)
{
  char thisByte;
  outCount = 0;

  unsigned long _m = millis();
  while (!client.available() && millis() < _m + timeout)
    delay(1);
  unsigned long _delay = millis() - _m;

  if (!client.available())
  {
    memset(outBuf, 0, sizeof(outBuf));
    strcpy(outBuf, "Offline ");
    return_code = 0;
    _isConnected = false;
    statusAnswer(_delay);
    return;
  }

  while (client.available())
  {
    thisByte = client.read();
    if (outCount < sizeof(outBuf))
    {
      outBuf[outCount] = thisByte;
      outCount++;
      outBuf[outCount] = 0;
    }
  }

  return_code = atoi(outBuf);

  if (outBuf[0] == '4' || outBuf[0] == '5')
  {
    _isConnected = false;
    statusAnswer(_delay);
    return;
  }

  statusAnswer(_delay);

  _isConnected = true;

  if (result != NULL)
  {
    // Deprecated
    // for (int i = 0; i < sizeof(outBuf); i++)
    // {
    //   result[i] = outBuf[i - 0];
    // }
    strcpy(result, outBuf);
  }
}
void ESP32_FTPClient::statusTransfer(unsigned long _delay, size_t _size)
{
  FTPdbg("Transfered ");
  FTPdbg(_size);
  FTPdbg(" bytes in ");
  FTPdbg(_delay);
  FTPdbg(" ms, ");
  FTPdbg((float)_size / _delay * 1000);
  FTPdbgn(" B/s");
}

void ESP32_FTPClient::statusAnswer(unsigned long _delay)
{
  if (verbose < 3)
    return;
  FTPdbg("FTP answer [");
  FTPdbg(_delay);
  FTPdbg(" ms] '");
  char out_dbg[strlen(outBuf)]{};
  strncpy(out_dbg, outBuf, strlen(outBuf) - 1);
  FTPdbg(out_dbg);
  FTPdbgn("'");
}

void ESP32_FTPClient::writeData(unsigned char *data, int dataLength, const char *dir, const char *file, en_FTP_file_type type)
{
  for (size_t _retry = 0; _retry <= retry; _retry++)
  {
    if (dir)
    {
      if (!_isConnected)
        openConnection();
      if (outBuf[0] == '4' || outBuf[0] == '5')
        _retry = retry;
      if (!_isConnected)
        goto end;

      if (file && dir)
      {
        changeWorkDir(dir);
        if (outBuf[0] == '4' || outBuf[0] == '5')
          _retry = retry;
        if (!_isConnected)
          goto end;
      }

      if (type == TYPE_I)
        initFile("Type I");
      if (type == TYPE_A)
        initFile("Type A");

      if (outBuf[0] == '4' || outBuf[0] == '5')
        _retry = retry;
      if (!_isConnected)
        goto end;

      if (file && dir)
        newFile(file);
      else if (dir)
        newFile(dir);
      if (outBuf[0] == '4' || outBuf[0] == '5')
        _retry = retry;
      if (!_isConnected)
        goto end;
    }
    else
      _retry = retry;

    { // prevent cross initialization error
      FTPdbgn(F("Write data to file"));
      unsigned long _m = millis();
      WriteClientBuffered(&dclient, &data[0], dataLength);
      statusTransfer(millis() - _m, dataLength);
      if (dir)
      {
        closeFile();
        if (outBuf[0] == '4' || outBuf[0] == '5')
          _retry = retry;
      }
    }
  end:
    if (_retry < retry && !_isConnected)
      retryConnection(_retry);
  }
  if (dir)
  {
    closeFile();
    closeConnection();
  }
}

void ESP32_FTPClient::writeString(const char *str, const char *dir, const char *file, en_FTP_file_type type)
{
  for (size_t _retry = 0; _retry <= retry; _retry++)
  {
    if (dir)
    {
      if (!_isConnected)
        openConnection();
      if (outBuf[0] == '4' || outBuf[0] == '5')
        _retry = retry;
      if (!_isConnected)
        goto end;

      if (file && dir)
      {
        changeWorkDir(dir);
        if (outBuf[0] == '4' || outBuf[0] == '5')
          _retry = retry;
        if (!_isConnected)
          goto end;
      }

      if (type == TYPE_I)
        initFile("Type I");
      if (type == TYPE_A)
        initFile("Type A");

      if (outBuf[0] == '4' || outBuf[0] == '5')
        _retry = retry;
      if (!_isConnected)
        goto end;

      if (file && dir)
        newFile(file);
      else if (dir)
        newFile(dir);
      if (outBuf[0] == '4' || outBuf[0] == '5')
        _retry = retry;
      if (!_isConnected)
        goto end;
    }
    else
      _retry = retry;

    { // prevent cross initialization error
      FTPdbgn(F("Write string to file"));
      unsigned long _m = millis();
      GetDataClient()->print(str);
      statusTransfer(millis() - _m, strlen(str));

      if (dir)
        closeFile();
    }
  end:
    if (_retry < retry && !_isConnected)
      retryConnection(_retry);
  }
  if (dir)
    closeFile();
}

void ESP32_FTPClient::closeFile()
{
  FTPdbgn(F("Close File"));
  dclient.stop();

  if (!isConnected())
    return;

  getFTPAnswer();
}

void ESP32_FTPClient::closeConnection()
{
  client.println(F("QUIT"));
  client.stop();
  FTPdbgn(F("Connection closed"));
  getFTPAnswer();
}

void ESP32_FTPClient::openConnection()
{
  FTPdbg(F("Connecting to: "));
  FTPdbgn(serverAdress);
  if (client.connect(serverAdress, 21, timeout))
  { // 21 = FTP server
    FTPdbgn(F("Command connected"));
  }
  getFTPAnswer();

  FTPdbgn("Send USER");
  client.print(F("USER "));
  client.println(F(userName));
  getFTPAnswer();

  FTPdbgn("Send PASSWORD");
  client.print(F("PASS "));
  client.println(F(passWord));
  getFTPAnswer();

  FTPdbgn("Send SYST");
  client.println(F("SYST"));
  getFTPAnswer();
}

void ESP32_FTPClient::retryConnection(size_t _retry)
{
  FTPdbg("Retry connection ");
  FTPdbg(_retry);
  FTPdbg("/");
  FTPdbgn(_retry);
}

void ESP32_FTPClient::status()
{
  FTPdbgn("Send STAT");
  client.print(F("STAT"));
  getFTPAnswer();
}

void ESP32_FTPClient::rename(char *from, char *to, char *dir)
{
  for (size_t _retry = 0; _retry <= retry || !_isConnected; _retry++)
  {
    if (!_isConnected)
      openConnection();
    if (!_isConnected)
      goto end;

    if (dir)
    {
      changeWorkDir(dir);
      if (!_isConnected)
        goto end;
    }

    FTPdbgn("Send RNFR");
    client.print(F("RNFR "));
    client.println(F(from));
    getFTPAnswer();
    if (!_isConnected)
      goto end;

    FTPdbgn("Send RNTO");
    client.print(F("RNTO "));
    client.println(F(to));
    getFTPAnswer();
  end:
    if (_retry < retry && !_isConnected)
      retryConnection(_retry);
  }
}

void ESP32_FTPClient::newFile(const char *fileName)
{
  FTPdbgn("Send STOR");
  client.print(F("STOR "));
  client.println(F(fileName));
  getFTPAnswer();
  if (!isConnected())
    return;
}

void ESP32_FTPClient::initFile(en_FTP_file_type type)
{
  if (type == TYPE_I)
    initFile("Type I");
  if (type == TYPE_A)
    initFile("Type A");
}
void ESP32_FTPClient::initFile(const char *type)
{
  FTPdbgn("Send TYPE");
  FTPdbgn(type);
  client.println(F(type));
  getFTPAnswer();
  if (!isConnected())
    return;

  FTPdbgn("Send PASV");
  client.println(F("PASV"));
  getFTPAnswer();
  if (!isConnected())
    return;

  char *tStr = strtok(outBuf, "(,");
  int array_pasv[6];
  for (int i = 0; i < 6; i++)
  {
    tStr = strtok(NULL, "(,");
    if (tStr == NULL)
    {
      FTPdbgn(F("Bad PASV Answer"));
      closeConnection();
      return;
    }
    array_pasv[i] = atoi(tStr);
  }
  unsigned int hiPort, loPort;
  hiPort = array_pasv[4] << 8;
  loPort = array_pasv[5] & 255;

  FTPdbg(F("Data port: "));
  hiPort = hiPort | loPort;
  FTPdbgn(hiPort);
  if (dclient.connect(serverAdress, hiPort, timeout))
  {
    FTPdbgn(F("Data connection established"));
  }
}

void ESP32_FTPClient::appendFile(char *fileName)
{
  FTPdbgn("Send APPE");
  if (!isConnected())
    return;
  client.print(F("APPE "));
  client.println(F(fileName));
  getFTPAnswer();
}

void ESP32_FTPClient::changeWorkDir(const char *dir)
{
  FTPdbg("Send CWD to ");
  FTPdbgn(dir);
  client.print(F("CWD "));
  client.println(F(dir));
  getFTPAnswer();
}

void ESP32_FTPClient::deleteFile(const char *file, const char *dir)
{
  for (size_t _retry = 0; _retry <= retry; _retry++)
  {
    if (!_isConnected)
      openConnection();
    if (outBuf[0] == '4' || outBuf[0] == '5')
      _retry = retry;
    if (!_isConnected)
      goto end;

    if (dir)
    {
      changeWorkDir(dir);
      if (outBuf[0] == '4' || outBuf[0] == '5')
        _retry = retry;
      if (!_isConnected)
        goto end;
    }

    FTPdbgn("Send DELE");
    if (!isConnected())
      return;
    client.print(F("DELE "));
    client.println(F(file));
    getFTPAnswer();

  end:
    if (_retry < retry && !_isConnected)
      retryConnection(_retry);
  }
}

void ESP32_FTPClient::makeDir(const char *dir)
{
  FTPdbgn("Send MKD");
  if (!isConnected())
    return;
  client.print(F("MKD "));
  client.println(F(dir));
  getFTPAnswer();
}

void ESP32_FTPClient::removeDir(const char *dir)
{
  FTPdbgn("Send RMD");
  if (!isConnected())
    return;
  client.print(F("RMD "));
  client.println(F(dir));
  getFTPAnswer();
}

void ESP32_FTPClient::contentList(String *list, size_t length, int offset_start, const char *dir)
{
  char _resp[sizeof(outBuf)];
  uint16_t _b = 0;

  FTPdbgn("Send MLSD");
  if (!isConnected())
    return;
  client.print(F("MLSD"));
  client.println(F(dir));
  getFTPAnswer(_resp);

  // Convert char array to string to manipulate and find response size
  // each server reports it differently, TODO = FEAT
  //String resp_string = _resp;
  //resp_string.substring(resp_string.lastIndexOf('matches')-9);
  //FTPdbgn(resp_string);

  unsigned long _m = millis();
  while (!dclient.available() && millis() < _m + timeout)
    delay(1);

  while (dclient.available())
  {
    if (_b >= offset_start)
    {
      if (_b - offset_start < length)
        list[_b - offset_start] = dclient.readStringUntil('\n');
      else
        break;
    }
    else
      dclient.readStringUntil('\n');
    _b++;
  }
}

void ESP32_FTPClient::downloadString(const char *filename, String &str, const char *dir, en_FTP_file_type type)
{
  for (size_t _retry = 0; _retry <= retry; _retry++)
  {
    if (dir)
    {
      if (!_isConnected)
        openConnection();
      if (outBuf[0] == '4' || outBuf[0] == '5')
        _retry = retry;
      if (!_isConnected)
        goto end;

      changeWorkDir(dir);
      if (outBuf[0] == '4' || outBuf[0] == '5')
        _retry = retry;
      if (!_isConnected)
        goto end;

      if (type == TYPE_I)
        initFile("Type I");
      if (type == TYPE_A)
        initFile("Type A");
      if (outBuf[0] == '4' || outBuf[0] == '5')
        _retry = retry;
      if (!_isConnected)
        goto end;
    }
    else
      _retry = retry;

    { // prevent cross initialization error
      FTPdbgn("Send RETR");
      client.print(F("RETR "));
      client.println(F(filename));
      char _resp[sizeof(outBuf)];
      getFTPAnswer(_resp);
      if (outBuf[0] == '4' || outBuf[0] == '5')
        _retry = retry;
      if (!_isConnected)
        goto end;

      unsigned long _m = millis();
      while (!GetDataClient()->available() && millis() < _m + timeout)
        delay(1);
      _m = millis();
      while (GetDataClient()->available())
        str += GetDataClient()->readString();
      statusTransfer(millis() - _m, strlen(str.c_str()));

      if (dir)
      {
        closeFile();
        if (outBuf[0] == '4' || outBuf[0] == '5')
          _retry = retry;
      }
    }
  end:
    if (_retry < retry && !_isConnected)
      retryConnection(_retry);
  }
  if (dir)
  {
    closeFile();
    closeConnection();
  }
}

void ESP32_FTPClient::downloadFile(const char *filename, unsigned char *buf, size_t length, bool printUART)
{
  FTPdbgn("Send RETR");
  if (!isConnected())
    return;
  client.print(F("RETR "));
  client.println(F(filename));

  char _resp[sizeof(outBuf)];
  getFTPAnswer(_resp);

  char _buf[2];

  unsigned long _m = millis();
  while (!dclient.available() && millis() < _m + timeout)
    delay(1);

  while (dclient.available())
  {
    if (!printUART)
      dclient.readBytes(buf, length);

    else
    {
      for (size_t _b = 0; _b < length; _b++)
      {
        dclient.readBytes(_buf, 1),
            Serial.print(_buf[0], HEX);
      }
    }
  }
}
