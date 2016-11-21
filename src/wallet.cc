#include <v8.h>
#include <node.h>
#include <nan.h>
#include <uv.h>
#include <db_cxx.h>

using namespace v8;
using namespace node;
using v8::FunctionTemplate;
using v8::Local;
using Nan::Set;

int deserializeArray(const unsigned char*, const unsigned int, unsigned char*, unsigned int, const unsigned int, const unsigned int);
NAN_METHOD(getMasterKey);

class BDB: public Nan::ObjectWrap {
  public:
    static NAN_MODULE_INIT(Init);

  private:
    explicit BDB();
    ~BDB();

    static NAN_METHOD(New);
    static Nan::Persistent<v8::Function> constructor;
};

NAN_METHOD(getMasterKey) {
  String::Utf8Value fileDat(info[0]);
  const char* dbName = "main";
  Dbc *dbc;
  Dbt key, data;
  Db db(NULL, 0);
  memset(&key, 0, sizeof(Dbt));
  memset(&data, 0, sizeof(Dbt));
  int ret = db.open(NULL, *fileDat, dbName, DB_BTREE, DB_RDONLY, 0);
  ret = db.cursor(NULL, &dbc, 0);
  while ((ret = dbc->get(&key, &data, DB_NEXT)) == 0 ) {
    int rSize = (int)*((char*)key.get_data());
    if (rSize != 4) continue;
    unsigned char *keyname = (unsigned char*)malloc(rSize+1);
    deserializeArray((const unsigned char*)key.get_data(), 1, keyname, 0, 0, 1);
    if (strcmp((const char*)keyname, "mkey") == 0) {
      Local<v8::Object> mkey = Nan::New<v8::Object>("salt",  
    }
    free(keyname);
  }
  Local<v8::String> teststr = Nan::New<v8::String>("test").ToLocalChecked();
  info.GetReturnValue().Set(teststr);
}

int deserializeArray(const unsigned char *input, const unsigned int offset, unsigned char *output, unsigned int maxLen, const unsigned int outputHex, const unsigned int nullTerminate) {
  unsigned int i = 0;
  if (!maxLen) {
    maxLen = (uint8_t)input[offset - 1];
  }
  while(i < maxLen) {
    if (outputHex) {
      char c[] = { ((input[i + offset] & 0xf0) >> 4), (input[i + offset] & 0x0f) };
      for (int j = 0; j < 2; j++) {
        if ((int)c[j] < 10) {
          output[i*2+j] = 0x30 | c[j];
        } else {
          output[i*2+j] = 0x60 | (c[j] - 0x09);
        }
      }
    } else {
      output[i] = input[i + offset];
    }
    i++;
  }
  if (nullTerminate) {
    output[i * (outputHex ? 2 : 1)] = '\0';
  }
  return (maxLen);
}

Nan::Persistent<v8::Function> BDB::constructor;

NAN_MODULE_INIT(BDB::Init) {
  Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("BDB").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(target, Nan::New("BDB").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

BDB::~BDB() {
}

NAN_METHOD(BDB::New) {
  BDB *obj = new BDB();
  obj->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

NAN_MODULE_INIT(InitAll) {
  Nan::Set(target, Nan::New("getMasterKey").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(getMasterKey)).ToLocalChecked());
  BDB::Init(target);
}

NODE_MODULE(Wallet, InitAll)
