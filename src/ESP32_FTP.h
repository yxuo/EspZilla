class ESP32_FTP
{
private:

  char *userName;
  char *passWord;
  char *serverAdress;
  uint16_t port;
  bool _isConnected = false;
  unsigned char clientBuf[1500];
  size_t bufferSize = 1500;
  size_t size_transfer = 0;
  uint16_t timeout = 10000;
  uint16_t timeout_transfer = 10000;
  unsigned long delay_transfer = 0;
  WiFiClient *GetDataClient();

  void WriteClientBuffered(WiFiClient *cli, unsigned char *data, int dataLength);
  char outBuf[128] = "Offline";
  unsigned char outCount;
  WiFiClient client;
  WiFiClient dclient;
  uint8_t verbose;

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

  template <typename T>
  void FTPerr(T msg)
  {
    if (verbose >= 1)
      Serial.print(msg);
  }

  void retryConnection(size_t _retry);
  void statusTransfer(unsigned long _delay, size_t _size);
  void statusAnswer(unsigned long _delay);


public:

  uint8_t retry = 1;
  uint16_t return_code;
  typedef enum
  {
    // AUTOMATIC_,
    TYPE_A,
    TYPE_I
  }en_FTP_file_type;

  ESP32_FTP(char *_serverAdress, uint16_t _port, char *_userName, char *_passWord, uint16_t _timeout = 10000, uint8_t _verbose = 1);
  ESP32_FTP(char *_serverAdress, char *_userName, char *_passWord, uint16_t _timeout = 10000, uint8_t _verbose = 1);
  void openConnection();
  void closeConnection();
  void status();
  bool isConnected(bool _verbose = true);
  void newFile(const char *fileName);
  void appendFile(char *fileName);
  void writeData(unsigned char *data, int dataLength, const char *dir = NULL, const char *file = NULL, en_FTP_file_type type = TYPE_I);
  void closeFile();
  void getFTPAnswer(char *result = NULL);
  void getLastModifiedTime(const char *fileName, char *result);
  size_t getSize(const char *fileName);
  void rename(char *from, char *to, char *dir = NULL);
  void writeString(const char *str, const char *dir = NULL, const char *file = NULL, en_FTP_file_type type = TYPE_A);
  void initFile(const char *type);
  void initFile(en_FTP_file_type type);
  void changeWorkDir(const char *dir);
  void deleteFile(const char *file, const char *dir = NULL);
  void makeDir(const char *dir);
  void removeDir(const char *dir);
  void contentList(String *list, size_t length, int offset_start = 0, const char *dir = "");
  void downloadString(const char *filename, String &str, const char *dir = NULL, en_FTP_file_type type = TYPE_A);
  void downloadFile(const char *filename, unsigned char *buf, size_t length, bool printUART = false);
};
