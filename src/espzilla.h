#include <functional>
#include <stdarg.h>
#include <HardwareSerial.h>

typedef std::function<void(unsigned char data, size_t len, size_t pos)> msgHandlerDownloadData;
// typedef std::function<void(const char *data, size_t len, size_t pos)> handlerInfo;
typedef std::function<void(String str, size_t pos)> handlerInfo;

class Espzilla
{
private:
  const char *username;
  const char *password;
  const char *server_address;
  uint16_t port;
  bool is_connected = false;
  bool is_success = false;
  unsigned char clientBuf[1500];
  size_t bufferSize = 1500;
  uint16_t timeout = 10000;
  unsigned long delay_transfer = 0;
  WiFiClient *GetDataClient();

  void WriteClientBuffered(WiFiClient *cli, unsigned char *data, int dataLength);
  char outBuf[128] = "Offline";
  unsigned char outCount;
  WiFiClient client;
  WiFiClient dclient;
  uint8_t verbose;
  uint8_t recursive_subfolder;
  uint8_t recursive_fn_pos;
  char recursive_folder[128]{};
  bool recursive_abort = false;

  template <typename T>
  void FTPdbg(T msg)
  {
    if (verbose >= 2)
      Serial.print(msg);
  }

  template <typename T>
  void FTPdbgn(T msg)
  {
    if (verbose >= 2)
      Serial.println(msg);
  }

  void FTPdbgn()
  {
    if (verbose >= 2)
      Serial.println();
  }

  template <typename T>
  void FTPerr(const T msg)
  {
    if (verbose >= 1)
      Serial.print(msg);
  }

  void statusReconnect(size_t _retry);
  void statusTransfer(unsigned long _delay, size_t _size);
  void statusAnswer(unsigned long _delay);
  bool statusConnected();

public:
  uint8_t retry;
  uint16_t return_code;
  typedef enum
  {
    TYPE_TXT,
    TYPE_BIN
  } en_transfer_mode;

  typedef enum
  {
    TYPE_FILE,
    TYPE_DIR,
    DETECT_ITEM
  } en_item_type;

  typedef enum
  {
    DIR_LIST,
    DIR_INFO,
    ITEM_LIST,
    NAME_LIST
  } en_info_type;

  Espzilla(const char *_server_address, const char *_username, const char *_password, uint16_t _timeout = 10000, uint8_t _retry = 2, uint8_t _verbose = 1, uint16_t _port = 21);

  // routine commands
  void openConnection();
  void closeConnection();
  void status();
  void testCommand(char *cmd);
  void changeWorkDir(const char *dir);
  bool isConnected();
  bool isSuccess();
  void initFile(const char *type);
  void initFile(en_transfer_mode transfer);
  void newFile(const char *fileName);
  void appendFile(char *fileName);
  void closeFile();
  void getFTPAnswer(char *result = NULL);

  // one command
  size_t getSize(const char *file, const char *dir = NULL, bool one_cmd = true);
  size_t getList(const char *item, const char *work_dir, en_info_type info, String *list = NULL, handlerInfo handler = NULL, int offset_start = 0, size_t length = NULL, bool one_cmd = true);
  bool getLastModifiedTime(const char *file, const char *dir, char *result, bool one_cmd = true);
  bool rename(char *from, char *to, bool one_cmd = true);
  bool writeString(const char *file, const char *dir, const char *str, en_transfer_mode transfer = TYPE_TXT, bool one_cmd = true);
  bool writeData(const char *file, const char *dir, unsigned char *data, int length, en_transfer_mode transfer = TYPE_BIN, bool one_cmd = true);
  bool deleteFile(const char *file, const char *dir = NULL, bool one_cmd = true);
  bool makeDir(const char *dir, const char *work_dir = NULL, bool force = true, bool one_cmd = true);
  bool removeDir(const char *dir, const char *work_dir = NULL, bool force = true, bool one_cmd = true);
  bool downloadString(const char *file, const char *dir, String &str, en_transfer_mode transfer = TYPE_TXT, bool one_cmd = true);
  bool downloadData(const char *file, const char *dir, unsigned char *data, size_t length, msgHandlerDownloadData handler = NULL, en_transfer_mode transfer = TYPE_BIN, bool one_cmd = true);

  // test
  bool siteCopy(char *from, char *to, bool one_cmd = true);

  void tst()
  {
    int repetir = 0;
  // denovo_0:
  denovo:
    if (!is_connected)
      openConnection();
    initFile("Type A");
    FTPdbgn("Send LIST  ");
    client.print(F("LIST "));
    client.println(F("Espzilla"));
    getFTPAnswer();
    statusConnected();
    unsigned long _m = millis();
    while (!dclient.available() && millis() < _m + timeout)
      delay(1);
    int pos = 0;
    String content{};
    while (dclient.available())
    {
      // size_t len = dclient.readBytesUntil('\n', content, 512);
      // size_t len = dclient.readBytesUntil('\n', content, 512);
      content = dclient.readStringUntil('\n');
      // content[len] = 0;
      if (pos < 2)
        // recursiveDeleteFiles(content.c_str(), content.length(), pos);
      pos++;
    }
    closeFile();
  }
};
