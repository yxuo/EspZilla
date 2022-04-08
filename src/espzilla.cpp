#include <WiFiClient.h>
#include "espzilla.h"

Espzilla::Espzilla(const char *_server_address, const char *_username, const char *_password, uint16_t _timeout, uint8_t _retry, uint8_t _verbose, uint16_t _port)
{
  username = _username;
  password = _password;
  server_address = _server_address;
  timeout = _timeout;
  retry = _retry;
  verbose = _verbose;
  port = _port;
}

WiFiClient *Espzilla::GetDataClient()
{
  return &dclient;
}

bool Espzilla::statusConnected()
{
  if (!is_connected)
  {
    FTPerr("FTP error: ");
    FTPerr(outBuf);
    if (outBuf[strlen(outBuf) - 1] != '\n')
      FTPerr('\r\n');
  }

  return is_connected;
}

/**
 * Returns if the FTP client is connected at the moment.
 *
 * ['TRUE'] : if server dont return any errors the connection is connected.
 * ['FALSE'] : if server retuns any errors [return number 4xx, 5xx] the server drops connection, even if the error is "Folder exists".
 * In order to ignore such cases and return true anyway, use 'isSuccess()'.
 */
bool Espzilla::isConnected()
{
  return is_connected;
}

/**
 * Returns if the FTP operation concluded its purpose or if it was already done, unnecessary anyway.
 *
 * ['TRUE'] : isConnected() = true, error "File exists".
 * ['FALSE'] : isConnected = false;
 */
bool Espzilla::isSuccess()
{
  return is_success;
}

/**
 * Get last modified timestamp in char format. Time format is 'YYYYMMDDhhmmss'
 * (Y year, M month, D month day, h hour, m min, s sec).
 * Returns isConnected(), case the server return no errors and connection doesn't drops.
 *
 * @param result (size: 14 b) -- response from server, timestamp of modified time.
 * @param file file name, relative or absolute address of file.
 * @param dir (one-command only) -- calls changeWorkDir(), change FTP directory. File name or relative dir will be searched based on this current dir. Absolute dir ignores current dir.
 * @param one_cmd [ TRUE ] : uses auto (re)connect and other FTP routine functions.
 *                [ FALSE ] : just download from FTP, behaves like an simple FTP routine.
 */
bool Espzilla::getLastModifiedTime(const char *file, const char *dir, char *result, bool one_cmd)
{
  for (size_t _retry = 0; _retry <= retry; _retry++)
  {
    if (one_cmd)
    {
      if (!is_connected)
        openConnection();
      if (!is_connected)
        goto end;
    }
    else
      _retry = retry;

    { // prevent cross initialization error
      char _resp[sizeof(outBuf)];
      FTPdbg("Send MDTM to ");
      FTPdbgn(file);
      client.print(F("MDTM "));
      client.println(F(file));
      getFTPAnswer(_resp);
      _resp[strlen(_resp) - 1] = 0;
      char *ptr_resp = &_resp[4];
      strncpy(result, ptr_resp, strlen(ptr_resp) - 1);
    }
  end:
    if (is_connected || outBuf[0] == '4' || outBuf[0] == '5')
      _retry = retry;
    else if (_retry < retry)
      statusReconnect(_retry + 1);
  }
  return (is_connected);
}

/**
 * Copy a list of itens inside directory to a list of Strings and to an external function.
 * Item buffer = 512 B. Automatically sets TYPE_TXT, in all cases.
 * Returns the number of items, including current directory '.' and parent directory '..'.
 *
 * @param file relative or absolute directory to list items. [ "" ] : list current working dir.
 * @param handler ( optional ) -- parse to function, for saving big files to memory or send SPI directly.
 * @param list destination of data.
 * @param length list length, maximum number of items to be copied to list.
 * @param offset_start ( default is 0 ) -- nth item to start copying.
 * @param one_cmd [ TRUE ] : uses auto (re)connect and other FTP routine functions.
 *                [ FALSE ] : just download from FTP, behaves like an simple FTP routine.
 */
size_t Espzilla::getSize(const char *file, const char *dir, bool one_cmd)
{
  // size_t resp = 0;
  for (size_t _retry = 0; _retry <= retry; _retry++)
  {
    if (one_cmd)
    {
      if (!is_connected)
        openConnection();
      if (!is_connected)
        goto end;

      if (dir)
      {
        changeWorkDir(dir);
        if (!is_connected)
          goto end;
      }
    }
    else
      _retry = retry;

    { // prevent cross initialization error
      FTPdbgn("Send TYPE");
      FTPdbgn("type I");
      client.println(F("type I"));
      getFTPAnswer();

      FTPdbgn("Send SIZE ");
      client.print(F("SIZE "));
      client.println(F(file));
      char _resp[sizeof(outBuf)];
      getFTPAnswer(_resp);
      if (!is_connected)
        goto end;

      _resp[strlen(_resp) - 1] = 0;
      char *ptr_resp = &_resp[4];
      if (atoi(ptr_resp))
        return atoi(ptr_resp);
    }
  end:
    if (is_connected || outBuf[0] == '4' || outBuf[0] == '5')
      _retry = retry;
    else if (_retry < retry)
      statusReconnect(_retry + 1);
  }
  return 0;
}

void Espzilla::WriteClientBuffered(WiFiClient *cli, unsigned char *data, int dataLength)
{
  if (!is_connected)
    return;

  size_t clientCount = 0;
  for (int i = 0; i < dataLength; i++)
  {
    clientBuf[clientCount] = data[i];
    // client.write(data[i])
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

void Espzilla::getFTPAnswer(char *result)
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
    is_success = false;
    is_connected = false;
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
  if (strstr(outBuf, "\n250 End of list"))
    return_code = 250;

  if (outBuf[0] == '4' || outBuf[0] == '5')
  {
    if (strstr(outBuf, "File exists"))
      is_success = true;
    else
      is_success = false;

    is_connected = false;
    statusConnected();
    return;
  }

  statusAnswer(_delay);

  is_success = true;
  is_connected = true;

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
void Espzilla::statusTransfer(unsigned long _delay, size_t _size)
{
  FTPdbg("Transfered ");
  FTPdbg(_size);
  FTPdbg(" bytes in ");
  FTPdbg(_delay);
  FTPdbg(" ms, ");
  FTPdbg((float)_size / _delay * 1000);
  FTPdbgn(" B/s");
}

void Espzilla::statusAnswer(unsigned long _delay)
{
  if (verbose < 3)
    return;
  char ln[3]{};
  int i, j;
  if (verbose < 4)
    for (i = strlen(outBuf) - 1, j = sizeof(ln) - 2; i >= 0 && j >= 0 && (outBuf[i] == '\r' || outBuf[i] == '\n'); i--, j--)
      ln[j] = outBuf[i], outBuf[i] = 0;

  FTPdbg("FTP answer [");
  FTPdbg(_delay);
  FTPdbg(" ms] '");
  FTPdbg(outBuf);
  FTPdbgn("'");

  if (verbose < 4)
    strcat(outBuf, &ln[j + 1]);
}

/**
 * Creates file then write data into it. If file exists, it will be overwritten.
 *
 * @param data file content to be written.
 * @param length size of data and FTP file.
 * @param file name, sub dir of current work dir or complete address of file.
 * @param dir ( one-command only ) -- calls changeWorkDir(), change current work dir to write file.
 *                                    [ "" ] : server returns error, invalid path.
 * @param type ( one-command only ) -- [ ftp.TYPE_TXT ] : download as ASCII text
 *                                     [ ftp.TYPE_BIN ] : download as binary file (images for example)
 * @param one_cmd [ TRUE ] : uses auto (re)connect and other FTP routine functions.
 *                [ FALSE ] : just write data to a previously created file. Thus, it behaves like a sub command FTP.
 */
bool Espzilla::writeData(const char *file, const char *dir, unsigned char *data, int length, en_transfer_mode transfer, bool one_cmd)
{
  for (size_t _retry = 0; _retry <= retry; _retry++)
  {
    if (one_cmd)
    {
      if (!is_connected)
        openConnection();
      if (!is_connected)
        goto end;

      if (dir)
      {
        changeWorkDir(dir);
        if (!is_connected)
          goto end;
      }

      if (transfer == TYPE_BIN)
        initFile("Type I");
      if (transfer == TYPE_TXT)
        initFile("Type A");
      if (!is_connected)
        goto end;

      if (file)
      {
        newFile(file);
        if (!is_connected)
          goto end;
      }
    }
    else
      _retry = retry;

    { // prevent cross initialization error
      FTPdbgn(F("Write data to file"));
      unsigned long _m = millis();
      WriteClientBuffered(&dclient, &data[0], length);
      statusTransfer(millis() - _m, length);

      if (one_cmd)
        closeFile();
    }
  end:
    if (is_connected || outBuf[0] == '4' || outBuf[0] == '5')
      _retry = retry;
    else if (_retry < retry)
      statusReconnect(_retry + 1);
  }
  if (!is_connected)
    FTPerr("Espzilla: rename(): FAILED\n");
  return (is_connected);
}

/**
 * Creates file then write string into it. If file exists, it will be overwritten.
 *
 * @param str file content to be written.
 * @param file name, sub dir of current work dir or complete address of file.
 * @param dir ( one-command only ) -- calls changeWorkDir(), change current work dir to write file.
 *                                    [ "" ] : server returns error, invalid path.
 * @param one_cmd [ TRUE ] : uses auto (re)connect and other FTP routine functions.
 *                [ FALSE ] : just write string to a previously created file. Thus, it behaves like a sub command FTP.
 * @param type ( one-command only ) -- [ ftp.TYPE_TXT ] : download as ASCII text
 *                                     [ ftp.TYPE_BIN ] : download as binary file (images for example)
 */
bool Espzilla::writeString(const char *file, const char *dir, const char *str, en_transfer_mode transfer, bool one_cmd)
{
  for (size_t _retry = 0; _retry <= retry; _retry++)
  {
    if (one_cmd)
    {
      if (!is_connected)
        openConnection();
      if (!is_connected)
        goto end;

      if (dir)
      {
        changeWorkDir(dir);
        if (!is_connected)
          goto end;
      }

      if (transfer == TYPE_BIN)
        initFile("Type I");
      if (transfer == TYPE_TXT)
        initFile("Type A");
      if (!is_connected)
        goto end;

      if (file)
      {
        newFile(file);
        if (!is_connected)
          goto end;
      }
    }
    else
      _retry = retry;

    { // prevent cross initialization error
      FTPdbgn(F("Write string to file"));
      unsigned long _m = millis();
      GetDataClient()->print(str);
      statusTransfer(millis() - _m, strlen(str));

      if (one_cmd)
        closeFile();
    }
  end:
    if (is_connected || outBuf[0] == '4' || outBuf[0] == '5')
      _retry = retry;
    else if (_retry < retry)
      statusReconnect(_retry + 1);
  }
  return (is_success);
}

void Espzilla::closeFile()
{
  FTPdbgn(F("Close File"));
  dclient.stop();
  getFTPAnswer();
}

void Espzilla::closeConnection()
{
  client.println(F("QUIT"));
  client.stop();
  FTPdbgn(F("Connection closed"));
  getFTPAnswer();
}

void Espzilla::openConnection()
{
  FTPdbg(F("Connecting to: "));
  FTPdbgn(server_address);
  if (client.connect(server_address, 21, timeout))
    FTPdbgn(F("Command connected"));
  getFTPAnswer();
  if (!is_connected)
    return;

  FTPdbgn("Send USER");
  client.print(F("USER "));
  client.println(F(username));
  getFTPAnswer();
  if (!is_connected)
    return;

  FTPdbgn("Send PASSWORD");
  client.print(F("PASS "));
  client.println(F(password));
  getFTPAnswer();
  if (!is_connected)
    return;

  FTPdbgn("Send SYST");
  client.println(F("SYST"));
  getFTPAnswer();
  if (!is_connected)
    return;
}

void Espzilla::statusReconnect(size_t _retry)
{
  FTPerr("Retry connection ");
  FTPerr(_retry);
  FTPerr("/");
  FTPerr(retry);
  FTPerr("\r\n");
}

void Espzilla::status()
{
  FTPdbgn("Send STAT");
  client.print(F("STAT"));
  getFTPAnswer();
}

/**
 * Rename or copy a file.
 * @param from Absolute file address, origin.
 * @param to Absolute file address, destination.
 * @param one_cmd [ TRUE ] : uses auto (re)connect and other FTP routine functions.
 *                [ FALSE ] : behaves like an simple FTP routine.
 */
bool Espzilla::rename(char *from, char *to, bool one_cmd)
{
  for (size_t _retry = 0; _retry <= retry; _retry++)
  {
    if (one_cmd)
    {
      if (!is_connected)
        openConnection();
      if (!is_connected)
        goto end;

      // if (dir)
      // {
      //   changeWorkDir(dir);
      //   if (!is_connected)
      //     goto end;
      // }
    }

    FTPdbgn("Send RNFR");
    client.print(F("RNFR "));
    client.println(F(from));
    getFTPAnswer();
    if (!is_connected)
      goto end;

    FTPdbgn("Send RNTO");
    client.print(F("RNTO "));
    client.println(F(to));
    getFTPAnswer();
  end:
    if (is_connected || outBuf[0] == '4' || outBuf[0] == '5')
      _retry = retry;
    else if (_retry < retry)
      statusReconnect(_retry + 1);
  }
  if (!is_connected)
    FTPerr("Espzilla: rename(): FAILED\n");
  return (is_connected);
}

bool Espzilla::siteCopy(char *from, char *to, bool one_cmd)
{
  for (size_t _retry = 0; _retry <= retry; _retry++)
  {
    if (one_cmd)
    {
      if (!is_connected)
        openConnection();
      if (!is_connected)
        goto end;

      // if (dir)
      // {
      //   changeWorkDir(dir);
      //   if (!is_connected)
      //     goto end;
      // }
    }

    FTPdbgn("Send CPFR");
    client.print(F("CPFR "));
    client.println(F(from));
    getFTPAnswer();
    if (!is_connected)
      goto end;

    FTPdbgn("Send CPTO ");
    client.print(F("CPTO "));
    client.println(F(to));
    getFTPAnswer();
  end:
    if (is_connected || outBuf[0] == '4' || outBuf[0] == '5')
      _retry = retry;
    else if (_retry < retry)
      statusReconnect(_retry + 1);
  }
  return (is_connected);
}

void Espzilla::newFile(const char *fileName)
{
  FTPdbgn("Send STOR");
  client.print(F("STOR "));
  client.println(F(fileName));
  getFTPAnswer();
  if (!is_connected)
    return;
}

void Espzilla::initFile(en_transfer_mode transfer)
{
  if (transfer == TYPE_BIN)
    initFile("Type I");
  if (transfer == TYPE_TXT)
    initFile("Type A");
}
void Espzilla::initFile(const char *type)
{
  FTPdbgn("Send TYPE");
  FTPdbgn(type);
  client.println(F(type));
  getFTPAnswer();
  if (!is_connected)
    return;

  FTPdbgn("Send PASV");
  client.println(F("PASV"));
  getFTPAnswer();
  if (!is_connected)
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

  hiPort = hiPort | loPort;
  FTPdbg(F("Data port: "));
  FTPdbgn(hiPort);
  if (dclient.connect(server_address, hiPort, timeout))
  {
    FTPdbgn(F("Data connection established"));
  }
}

void Espzilla::appendFile(char *fileName)
{
  FTPdbgn("Send APPE");
  client.print(F("APPE "));
  client.println(F(fileName));
  getFTPAnswer();
}

void Espzilla::changeWorkDir(const char *dir)
{
  FTPdbg(F("Send CWD to "));
  FTPdbgn(dir);
  client.print(F("CWD "));
  client.println(F(dir));
  getFTPAnswer();
}

bool Espzilla::deleteFile(const char *file, const char *dir, bool one_cmd)
{
  for (size_t _retry = 0; _retry <= retry; _retry++)
  {
    if (one_cmd)
    {
      if (!is_connected)
        openConnection();
      if (!is_connected)
        goto end;

      if (dir)
      {
        changeWorkDir(dir);
        if (!is_connected)
          goto end;
      }
    }
    FTPdbgn("Send DELE");
    client.print("DELE ");
    client.println(file);
    getFTPAnswer();

  end:
    if (is_connected || outBuf[0] == '4' || outBuf[0] == '5')
      _retry = retry;
    else if (_retry < retry)
      statusReconnect(_retry + 1);
  }
  return (is_connected);
}

void Espzilla::testCommand(char *cmd)
{
  FTPdbg("Send ");
  FTPdbgn(cmd);
  client.println(cmd);
  getFTPAnswer();
}

/**
 * Create folders through FTP.
 * @return 'isSuccess()'.
 * <true> error 'Folder exists',  isConnected() = true.
 * <false> isConnected() = false, internet failed, invalid command, server failed.
 *
 * @param dir dir or sub dir of current work dir or complete address of file.
 *                 (inserting the complete address doesnt change work dir)
 * @param work_dir ( one-command only ) -- directory above dir.
 * @param force ( one-command only ) -- if necessary create all subfolders.
 * @param one_cmd <true> uses auto (re)connect and other FTP routine functions.
 *                <false> just download from FTP, behaves like an simple FTP routine.
 */
bool Espzilla::makeDir(const char *dir, const char *work_dir, bool force, bool one_cmd)
{
  for (size_t _retry = 0; _retry <= retry; _retry++)
  {
    if (one_cmd)
    {
      if (!is_connected)
        openConnection();
      if (!is_connected)
        goto end;

      if (work_dir)
      {
        changeWorkDir(work_dir);
        if (!is_connected)
          goto end;
      }
    }

    FTPdbgn("Send MKD");
    client.print(F("MKD "));
    client.println(F(dir));
    getFTPAnswer();

    // Force makedir
    if (force && strstr(outBuf, "No such file or directory"))
    {
      // copy whole dir address
      size_t len_dir_new = 1;
      if (work_dir)
        len_dir_new += strlen(work_dir);
      len_dir_new += strlen(dir);
      char dir_new[len_dir_new]{};
      if (work_dir)
        strcpy(dir_new, work_dir);
      strcat(dir_new, dir);

      char *ptr_dir = strstr(dir_new + 1, "/");
      if (ptr_dir)
      {
        // Number of dirs available
        uint8_t sub_dir, pos_sub_dir; //, ini = 0;
        // if(dir_new[0] == '/')ini = 1;

        for (sub_dir = 0; ptr_dir; sub_dir++)
          ptr_dir = strstr(ptr_dir + 1, "/");

        // erase last and makedir
        for (pos_sub_dir = sub_dir; pos_sub_dir; pos_sub_dir--)
        {
          ptr_dir = strstr(dir_new + 1, "/");
          for (size_t i = 1; i < pos_sub_dir; i++)
            ptr_dir = strstr(ptr_dir + 1, "/");
          ptr_dir[0] = 0;
          makeDir(dir_new, false);
          if (is_connected)
            break;
          else if (!strstr(outBuf, "No such file or directory"))
            goto end;
        }
        // restore / and makedir
        for (pos_sub_dir; pos_sub_dir <= sub_dir; pos_sub_dir++)
        {
          dir_new[strlen(dir_new)] = '/';
          makeDir(dir_new, false);
          if (!is_connected)
            goto end;
        }
      }
    }
  end:
    if (is_success || outBuf[0] == '4' || outBuf[0] == '5')
      _retry = retry;
    else if (_retry < retry)
      statusReconnect(_retry + 1);
  }
  return (is_success);
}

/**
 * Remove folder of FTP server.
 * Returns 'isConnected()'.
 *
 * @param dir file name, relative or absolute address of file.
 * @param work_dir (one-command only) -- calls changeWorkDir(), change FTP directory.
 * File name or relative dir will be searched based on this current dir. Absolute dir ignores current dir.
 * @param one_cmd [ TRUE ] : uses auto (re)connect and other FTP routine functions.
 *                [ NULL ] : just download from FTP, behaves like an simple FTP routine.
 */
bool Espzilla::removeDir(const char *dir, const char *work_dir, bool force, bool one_cmd)
{
  for (size_t _retry = 0; _retry <= retry; _retry++)
  {
    uint8_t item_num = 0;
    if (one_cmd)
    {
      if (force)
      {
        item_num = getList(dir, work_dir, ITEM_LIST, NULL, NULL, 0, 3);
        if (!is_connected)
          goto end;
        if (item_num > 2)
          goto force_del;
      }
      else
      {
        if (!is_connected)
          openConnection();
        if (!is_connected)
          goto end;

        if (work_dir)
        {
          changeWorkDir(work_dir);
          if (!is_connected)
            goto end;
        }
      }
    }
    else
      _retry = retry;

    { // prevent cross initialization error
      FTPdbgn("Send RMD");
      client.print(F("RMD "));
      client.println(F(dir));
      getFTPAnswer();

      // force delete
      if (strstr(outBuf, "Directory not empty"))
      {
      force_del:
        int sub_folder = 0;
        String adr = dir;
        if (work_dir)
          adr = work_dir + adr;
        if (adr[adr.length() - 1] == '/')
          adr[adr.length() - 1] = 0;
      I:
        String list[3]{};
        item_num = getList(adr.c_str(), NULL, ITEM_LIST, list, NULL, 0, 3);
        if (!is_connected)
          goto end;
        for (uint8_t n = 0; n < 3; n++)
          list[n][list[n].length() - 1] = 0;
        if (item_num == 1)
        {
          deleteFile(list[0].substring(list[0].indexOf(' ', 15) + 33).c_str(), adr.c_str());
          if (!is_connected)
            goto end;
          goto I;
        }
        if (item_num == 2)
        {
          removeDir(adr.c_str(), NULL, false);
          if (!is_connected || !sub_folder)
            goto end;
          if (sub_folder)
          {
            adr[adr.lastIndexOf('/')] = 0;
            sub_folder--;
            goto I;
          }
        }
        if (item_num > 2)
        {
          if (atoi(list[2].substring(10, 14).c_str()) == 1)
          {
            deleteFile(list[2].substring(list[2].indexOf(' ', 15) + 33).c_str(), adr.c_str());
            if (!is_connected)
              goto end;
            goto I;
          }
          else
          {
            adr = adr + '/' + list[2].substring(list[2].indexOf(' ', 15) + 33).c_str();
            sub_folder++;
            goto I;
          }
        }
      }
    }
  end:
    if (is_connected || outBuf[0] == '4' || outBuf[0] == '5')
      _retry = retry;
    else if (_retry < retry)
      statusReconnect(_retry + 1);
  }
  return (is_connected);
}

/**
 * Copy a list of itens inside directory to a list of Strings and to an external function.
 * Item buffer = 512 B. Automatically sets TYPE_TXT, in all cases.
 * It doesnt owrk with documents, for such, use
 *
 * @param dir relative or absolute directory to list items. [ "" ] : list current working dir.
 * @param handler ( optional ) -- parse to function, for saving big files to memory or send SPI directly.
 * @param list destination of data.
 * @param length list length, maximum number of items to be copied to list.
 * @param offset_start ( default is 0 ) -- nth item to start copying.
 * @param one_cmd [ TRUE ] : uses auto (re)connect and other FTP routine functions.
 *                [ FALSE ] : just download from FTP, behaves like an simple FTP routine.
 * @return copied items number
 */
size_t Espzilla::getList(const char *obj, const char *work_dir, en_info_type info, String *list, handlerInfo handler, int offset_start, size_t length, bool one_cmd)
{
  size_t pos = 0;
  for (size_t _retry = 0; _retry <= retry; _retry++)
  {
    if (one_cmd)
    {
      if (!is_connected)
        openConnection();
      if (!is_connected)
        goto end;

      if (work_dir)
      {
        changeWorkDir(work_dir);
        if (!is_connected)
          goto end;
      }
    }
    else
      _retry = retry;

    { // prevent cross initialization error
      initFile("Type A");
      if (!is_connected)
        goto end;

      char _cmd[6] = "LIST ";
      if (info == DIR_LIST)
        strcpy(_cmd, "MLSD ");
      if (info == DIR_INFO)
        strcpy(_cmd, "MLST ");
      if (info == NAME_LIST)
        strcpy(_cmd, "NLST ");

      FTPdbg("Send ");
      FTPdbg(_cmd);
      FTPdbg("to ");
      FTPdbg(obj);
      if (work_dir)
      {
        FTPdbg(" in ");
        FTPdbg(work_dir);
      }
      FTPdbgn();

      client.print(F(_cmd));
      client.println(F(obj));
      getFTPAnswer();
      if (!is_connected)
        goto end;

      if (info == DIR_INFO)
      {
        char *ptr_info = strstr(outBuf, "\n250 End of list");
        if (ptr_info)
        {
          ptr_info[0] = 0;
          if (list && length)
            list[0] = outBuf;
          if (handler)
            handler(outBuf, 0);
          if (verbose > 2)
            FTPdbgn(outBuf);

          ptr_info[0] = '\n';
          pos++;
        }
      }
      else
      {
        unsigned long _m = millis();
        while (!dclient.available() && millis() < _m + timeout)
          delay(1);

        _m = millis();
        pos = 0;
        size_t total = 0;
        // char content[512]{};
        String content{};
        while (dclient.available())
        {
          if (pos >= offset_start)
          {
            // size_t len = dclient.readBytesUntil('\n', content, 512);
            content = dclient.readStringUntil('\n');
            size_t len = content.length();
            total += len;
            // content[len] = 0;
            bool parsed = false;

            if (list && length && pos - offset_start < length)
              list[pos - offset_start] = content, parsed = true;
            if (handler && (!length || pos - offset_start < length))
              handler(content, pos - offset_start), parsed = true;
            if (verbose > 2 && (!length || pos - offset_start < length))
              FTPdbg(content), parsed = true;

            if (!parsed)
              goto parse_end;
          }
          else
            total += dclient.readStringUntil('\n').length();
          // total += dclient.readBytesUntil('\n', content, 512);
          pos++;
        }
      parse_end:
        if (handler)
          handler("", pos);
        statusTransfer(millis() - _m, total);
        if (one_cmd)
          closeFile();
      }
    }
  end:
    if (is_connected || outBuf[0] == '4' || outBuf[0] == '5')
      _retry = retry;
    else if (_retry < retry)
      statusReconnect(_retry + 1);
  }
  return (pos);
}

/**
 * Download and write a FTP file (text and byte) to String, recommended for manipulate text in most of cases.
 * Maximum size of String (ESP32 is 4000 B). No support to parsing to function, for such, use 'downloadData()'.
 * Returns 'isConnected()'.
 *
 * @param filename name, sub dir of current work dir or complete address of file.
 *                 (inserting here the complete address doesnt change work dir)
 * @param str destination of data.
 * @param one_cmd [ TRUE ] : uses auto (re)connect and other FTP routine functions.
 *                [ FALSE ] : just download from FTP, behaves like an simple FTP routine.
 * @param type ( one-command only ) -- [ ftp.TYPE_TXT ] : download as ASCII text.
 *                                     [ ftp.TYPE_BIN ] : download as binary file (images for example).
 * @param dir ( one-command only ) -- calls changeWorkDir(), change FTP directory along download.
 *                                    [ "" ] : server returns error, invalid path.
 */
bool Espzilla::downloadString(const char *file, const char *dir, String &str, en_transfer_mode transfer, bool one_cmd)
{
  for (size_t _retry = 0; _retry <= retry; _retry++)
  {
    if (one_cmd)
    {
      if (!is_connected)
        openConnection();
      if (!is_connected)
        goto end;

      if (dir)
      {
        changeWorkDir(dir);
        if (!is_connected)
          goto end;
      }

      if (transfer == TYPE_BIN)
        initFile("Type I");
      if (transfer == TYPE_TXT)
        initFile("Type A");
      if (!is_connected)
        goto end;
    }
    else
      _retry = retry;

    { // prevent cross initialization error
      FTPdbgn("Send RETR");
      client.print(F("RETR "));
      client.println(F(file));
      char _resp[sizeof(outBuf)];
      getFTPAnswer(_resp);
      if (!is_connected)
        goto end;

      unsigned long _m = millis();
      while (!GetDataClient()->available() && millis() < _m + timeout)
        delay(1);

      _m = millis();
      if (str)
        while (GetDataClient()->available())
          str += GetDataClient()->readString();
      GetDataClient()->stop();
      getFTPAnswer();
    }
  end:
    if (is_connected || outBuf[0] == '4' || outBuf[0] == '5')
      _retry = retry;
    else if (_retry < retry)
      statusReconnect(_retry + 1), str.clear();
  }
  return (is_success);
}

/**
 * Download and write a FTP file (text and byte) to variable.
 * Recommended for manipulate images or fixed-size variables (char, byte, etc).
 * Returns 'isConnected()'.
 *
 * @param filename name, sub dir of current work dir or complete address of file.
 * Inserting here the complete address doesnt change work dir, and ignores it.
 * @param data ( optional ) -- destination of data. if 'data' and 'handler' are NULL the function returns false at first.
 * @param handler ( optional ) -- parse to function, for saving big files to memory or send SPI directly
 * @param length size of data and FTP file
 * @param one_cmd [ TRUE ] : uses auto (re)connect and other FTP routine functions
 *                [ FALSE ] : just download from FTP, behaves like an simple FTP routine
 * @param type ( one-command only ) -- [ ftp.TYPE_TXT ] : download as ASCII text
 *                                     [ ftp.TYPE_BIN ] : download as binary file (images for example)
 * @param dir ( one-command only ) -- calls changeWorkDir(), change FTP directory along download.
 *                                    [ "" ] : server returns error, invalid path.
 */
bool Espzilla::downloadData(const char *file, const char *dir, unsigned char *data, size_t length, msgHandlerDownloadData handler, en_transfer_mode transfer, bool one_cmd)
{
  if (!length || (!data && !handler))
  {
    FTPerr("FTP err downloadData: destination with no length / data and handler are set to NULL, nothing to parse, aborted.\n");

    FTPerr("file: ");
    if (file)
      FTPerr(file);
    else
      FTPerr("NULL");
    FTPerr("\n");

    FTPerr("dir: ");
    if (dir)
      FTPerr(dir);
    else
      FTPerr("NULL");
    FTPerr("\n");

    FTPerr("data: ");
    if (data)
      FTPerr("OK");
    else
      FTPerr("NULL");
    FTPerr("\n");

    FTPerr("length: ");
    if (length)
      FTPerr(length);
    else
      FTPerr("NULL");
    FTPerr("\n");

    is_success = false;
    return false;
  }

  for (size_t _retry = 0; _retry <= retry; _retry++)
  {
    bool transfered = false;
    if (one_cmd)
    {
      if (!is_connected)
        openConnection();
      if (!is_connected)
        goto end;

      if (dir)
      {
        changeWorkDir(dir);
        if (!is_connected)
          goto end;
      }
      if (transfer == TYPE_BIN)
        initFile("Type I");
      if (transfer == TYPE_TXT)
        initFile("Type A");
      if (!is_connected)
        goto end;
    }
    else
      _retry = retry;

    { // prevent cross initialization error
      FTPdbgn("Send RETR");
      client.print(F("RETR "));
      client.println(F(file));
      if (!is_connected)
        goto end;

      char _resp[sizeof(outBuf)];
      getFTPAnswer(_resp);

      char _buf[2];

      unsigned long _m = millis();
      while (!dclient.available() && millis() < _m + timeout)
        delay(1);

      size_t pos = 0;
      if (dclient.available())
      {
        for (pos = 0; pos < length; pos++)
        {
          dclient.readBytes(_buf, 1);
          if (data)
            data[pos] = _buf[0];
          if (handler)
            handler(_buf[0], length, pos);
        }
        statusTransfer(millis() - _m, pos);

        // ? case there is new bytes after end of file
        while (dclient.available())
        {
          dclient.readBytes(_buf, 1);
          // Serial.println(_buf[0]);
          pos++;
        }
        if (pos != length)
        {
          FTPdbg("Expected: ");
          FTPdbg(length);
          FTPdbgn(" bytes");
        }
        if (pos > length)
        {
          FTPdbg("Total: ");
          FTPdbg(pos);
          FTPdbgn(" bytes");
        }
      }
      if (pos >= length)
        transfered = true;
      dclient.stop();
    }
  end:
    if ((transfered && is_connected) || outBuf[0] == '4' || outBuf[0] == '5')
      _retry = retry;
    else if (_retry < retry)
      statusReconnect(_retry + 1);
  }
  return (is_success);
}
