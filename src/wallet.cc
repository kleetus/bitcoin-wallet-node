#include <nan.h>
#include <db_cxx.h>
#include <thread>
#include "streaming-worker.h"
#include "json.hpp"

#define DBNAME "main"

using json = nlohmann::json;

inline bool datFileExists(const std::string& datFile);
inline int deserializeArray(const unsigned char*, const unsigned int, unsigned char*);
int deserializeInt(const char unsigned*, const unsigned int);

const char *derivationMethods[] = { "SHA512", "scrypt" };

class DbFileException: public std::exception {
  public:
    DbFileException(std::string msg) : std::exception(), msg_(msg) {}

    virtual const char* what() const throw() {
      return msg_.c_str();
    }
  private:
    std::string msg_;
};

class Stream: public StreamingWorker {
  public:
    Stream(Callback *data, Callback *complete,
        Callback *error_callback,  v8::Local<v8::Object>& options)
      : StreamingWorker(data, complete, error_callback), filePath_("./wallet.dat") {

      v8::Local<v8::Value> file;
      if (options->IsObject() ) {
        file = options->Get(New<v8::String>("filePath").ToLocalChecked());
        if (file->IsString()) {
          v8::String::Utf8Value s(file);
          filePath_ = std::string(*s);
        }
      } else if (options->IsString()) {
        v8::String::Utf8Value s(options);
        filePath_ = std::string(*s);
      }
      if (!datFileExists(filePath_)) {
        DbFileException dbFileException("Wallet.dat file does not exist.");
        throw dbFileException;
      }
    }

    void Execute (const AsyncProgressWorker::ExecutionProgress& progress) {
      while (!ready()) {
        std::this_thread::sleep_for(chrono::milliseconds(50));
      }
      Dbc *dbc;
      Dbt key, data;
      Db db(NULL, 0);
      int ret;

      memset(&key, 0, sizeof(Dbt));
      memset(&data, 0, sizeof(Dbt));

      if (db.open(NULL, filePath_.c_str(), DBNAME, DB_BTREE, DB_RDONLY, 0)) {
        DbFileException dbFileException("Unable to open the db.");
        throw dbFileException;
      }

      if (db.cursor(NULL, &dbc, 0)) {
        DbFileException dbFileException("Unable to obtain the db cursor");
        throw dbFileException;
      }

      while ((ret = dbc->get(&key, &data, DB_PREV)) == 0 ) {
        const char *keyData = (const char*)key.get_data();

        if (!(keyData[1] == 'm' || keyData[1] == 'c')) {
          continue;
        }

        json obj;
        unsigned char *value = (unsigned char*)malloc(256);

        std::string k(keyData);
        std::string keyType = k.substr(1, 4);

        if (keyType == "mkey") {

          deserializeArray((const unsigned char*)data.get_data(), 50, value);
          obj["salt"] = std::string((char*)value);

          int rounds = deserializeInt((const unsigned char*)data.get_data(), 62);
          obj["rounds"] = rounds;

          int type = deserializeInt((const unsigned char*)data.get_data(), 58);
          obj["derivationMethod"] = std::string(derivationMethods[type]);

          int id = deserializeInt((const unsigned char*)keyData, 5);
          obj["id"] = id;

        } else if (keyType == "ckey") {

          deserializeArray((const unsigned char*)data.get_data(), 1, value);
          obj["cipherText"] = std::string((char*)value);

          deserializeArray((const unsigned char*)key.get_data(), 6, value);
          obj["pubKey"] = std::string((char*)value);

        } else {
          free(value);
          continue;
        }

        free(value);
        Message tosend("data", obj.dump());
        writeToNode(progress, tosend);
        while (paused() && !closed()) {
          std::this_thread::sleep_for(chrono::milliseconds(500));
        }
        if (closed()) goto close;
      }
close:
      dbc->close();
      db.close(0);
    }

  private:
    std::string filePath_;
};

StreamingWorker *create_worker(Callback *data, Callback *complete, Callback *error_callback, v8::Local<v8::Object> &options) {
  return new Stream(data, complete, error_callback, options);
}

inline bool datFileExists(const std::string& datFile) {
  struct stat buffer;
  return (stat (datFile.c_str(), &buffer) == 0);
}

inline int deserializeArray(const unsigned char *input, const unsigned int offset, unsigned char *output) {
  unsigned int i = 0;
  unsigned int maxLen = (uint8_t)input[offset - 1];
  while(i < maxLen) {
    char c[] = { static_cast<char>((input[i + offset] & 0xf0) >> 4), static_cast<char>(input[i + offset] & 0x0f) };
    for (int j = 0; j < 2; j++) {
      if ((int)c[j] < 10) {
        output[i*2+j] = 0x30 | c[j];
      } else {
        output[i*2+j] = 0x60 | (c[j] - 0x09);
      }
    }
    i++;
  }
  output[i * 2] = '\0';
  return (maxLen);
}

//this assumes serialized little endian
int deserializeInt(const char unsigned *input, const unsigned int offset) {
  return (uint32_t)((uint8_t)input[offset] |
    ((uint8_t)input[offset+1] << 8) |
    ((uint8_t)input[offset+2] << 16) |
    ((uint8_t)input[offset+3] << 24));
}

NODE_MODULE(Wallet, StreamWorkerWrapper::Init)
