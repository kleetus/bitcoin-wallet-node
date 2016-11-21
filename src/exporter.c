#include <db.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef _WIN32
extern int getopt(int, char* const*, const char*);
extern char *optarg;
#define snprintf _snprintf
#else
#include <unistd.h>
#endif

typedef struct {
  char *key;
  char type;
  void *data;
} OBJ;

char *derivationMethods[] = { "SHA512", "scrypt" };

const OBJ *objValues[6];
OBJ type, id, ct, slt, dm, rnd, pk;

unsigned char ukeySerPK[429], pkHash[65], cipherText[97], salt[17], pubkey[256], strKey[5], logger[256];
int ret, rounds = 0, nID = 0, ukeynum = 0, keynum = 0, ckeynum = 0, mkeynum = 0, recordsWritten = 0;
short int silent = 0, valuesLen = 0, writeLog = 0, firstRecordWritten = 0;
FILE *outfile;
char *infilepath = NULL, *outfilepath = NULL;
uint32_t flags = DB_RDONLY;
DB *db;
DBC *cur;
DBT key, value;
int keylen;

void printMetaInfo(const unsigned char*);
int usage(void);
int testProvidedFiles(char*);
int deserializeInt(const unsigned char*, const unsigned int);
int deserializeArray(const unsigned char*, const unsigned int, unsigned char*, const unsigned int, const unsigned int, const unsigned int);
int writeRecord(const OBJ* [], const unsigned int, const unsigned char*, FILE*);
char testkey[256];
char testvalue[512];

int main(int argc, char *argv[]) {

  char ch;
  while ((ch = getopt(argc, argv, "d:o:h:s:")) != EOF) {
    switch (ch) {
      case 'd':
        infilepath = optarg;
        break;
      case 'o':
        outfilepath = optarg;
        break;
      case 'h':
        return (usage());
      case 's':
        silent = 1;
        break;
      default:
        return (usage());
    }
  }

  if(!testProvidedFiles(infilepath)) {
    fprintf(stderr, "Provided input wallet file or output wallet file location is not accessible or writeable.\n");
    return 1;
  }

  ret = db_create(&db, NULL, 0);

  ret = db->open(db, NULL, infilepath, "main", DB_BTREE, flags, 0);
  if (ret != 0) {
    fprintf(stderr, "Unable to open the database file: %s", infilepath);
    return (ret);
  }

  db->cursor(db, NULL, &cur, 0);

  //we zero out the key and value structs memset
  //there is a "size" and a "value", the size should be an int and the value should a void*
  memset(&key, 0, sizeof(DBT));
  memset(&value, 0, sizeof(DBT));

  if (outfilepath != NULL) {
    outfile = fopen(outfilepath, "r+");
    if (outfile != NULL) {
      fclose(outfile);
      char ans;
      printf("The file: \"%s\" exists. Append/Overwrite/Quit? (A/o/q): ", outfilepath);
      scanf("%c", &ans);
      if (ans == 'o') {
        outfile = fopen(outfilepath, "w");
      } else if (ans == 'a' || ans == 'A') {
        printf("OK, appending to: \"%s\"\n", outfilepath);
        outfile = fopen(outfilepath, "a");
      } else {
        fprintf(stdout, "Chose not to overwrite output file, exiting.");
        return (0);
      }
    } //the file doesn't exist or we had an error trying to open it
    else {
      outfile = fopen(outfilepath, "w");
      if (outfile == NULL) {
        fprintf(stderr, "Could not write to output file, exiting.");
        return 1;
      }
    }
  } else {
    outfile = stdout;
  }

  printMetaInfo((const unsigned char*)"Searching the database for master keys and private keys...");

  fprintf(outfile, "[\n  ");
  /*
  note: the DBT objects for the key and the value are re-used. These DBT structures are
  auto initialized into the data segment of the binary. The smallest value for the data buffer is the
  page size for the database, 4K? This should be plenty large enough for all the data values that
  we are interested in. If there isn't enough room in the buffer, then the DB_BUFFER_SMALL error will
  be returned.
  */

  while ((ret = cur->get(cur, &key, &value, DB_NEXT)) == 0) {

    if (ret == DB_BUFFER_SMALL) {
      printMetaInfo((const unsigned char*)"[Warning] tried to retrieve a data value, but DB_DBT_USERMEM was too small.");
    }

    keynum++;

    deserializeArray((const unsigned char*)key.data, 1, strKey, 0, 0, 1);

    if (strcmp((const char*)strKey, "mkey") == 0) {
      /* serialization format is:

        key: [length][key name][id]
        example: [4][mkey][0]

                  0       1..48       49   50..57   58..61     62..65      66
        value: [length][cipherText][length][salt][der method][der rounds][length][der params]
        example: [48][0..47][8][0..7][0][25000][0][0]

      */
      valuesLen = 6;

      type.type = 's';
      type.key = "type";
      type.data = "master";
      objValues[0] = &type;

      nID = deserializeInt((const unsigned char*)key.data, 5);
      id.key = "id";
      id.type = 'i';
      id.data = &nID;
      objValues[1] = &id;

      deserializeArray((const unsigned char*)value.data, 1, cipherText, 48, 1, 1);
      ct.key = "cipherText";
      ct.type = 's';
      ct.data = cipherText;
      objValues[2] = &ct;

      deserializeArray((const unsigned char*)value.data, 50, salt, 8, 1, 1);
      slt.key = "salt";
      slt.type = 's';
      slt.data = salt;
      objValues[3] = &slt;

      int derInt = deserializeInt((const unsigned char*)value.data, 58);
      dm.key = "derivationMethod";
      dm.type = 's';
      dm.data = derivationMethods[derInt];
      objValues[4] = &dm;

      rounds = deserializeInt((const unsigned char*)value.data, 62);
      rnd.key = "rounds";
      rnd.type = 'i';
      rnd.data = &rounds;
      objValues[5] = &rnd;
      mkeynum++;
      writeLog = 1;

    } else if (strcmp((const char*)strKey, "ckey") == 0) {
      /*
         serialization format:
         key: [length][ckey][length][compressed pub key]
         example: [4][ckey][33][0..32]
         value: [length][cipherText]
         example: [48][0..47] <- 48 byts of data representing the 32 bytes plain text private key
      */
      ckeynum++;
      valuesLen = 3;

      type.type = 's';
      type.key = "type";
      type.data = "encrypted private key";
      objValues[0] = &type;

      deserializeArray((const unsigned char*)value.data, 1, cipherText, 48, 1, 1);
      ct.type = 's';
      ct.key = "cipherText";
      ct.data = cipherText;
      objValues[1] = &ct;

      deserializeArray((const unsigned char*)key.data, 6, pubkey, 0, 1, 1);
      pk.type = 's';
      pk.key = "pubKey";
      pk.data = pubkey;
      objValues[2] = &pk;
      writeLog = 1;

    } else if (strcmp((const char*)strKey, "key") == 0) { //this is an unencrypted key, the key is just "key" + compressed pub key
      /*
         serialization format:
         key: [length][key][length][compressed pub key]
         example: [3][key][33][0..32]
         value: [length][openssl evp serialized private key in der format][sha256 hash of private key + pub key]
         example: [214][0..213][0..31] <-- 247 bytes total
      */
      ukeynum++;
      valuesLen = 3;

      type.type = 's';
      type.key = "type";
      type.data = "unencrypted private key";
      objValues[0] = &type;

      deserializeArray((const unsigned char*)value.data, 1, ukeySerPK, 0, 1, 1);
      ct.type = 's';
      ct.key = "OpenSSL EVP DER-encoded serialized private key";
      ct.data = ukeySerPK;
      objValues[1] = &ct;

      deserializeArray((const unsigned char*)value.data, 215, pkHash, 32, 1, 1);
      pk.type = 's';
      pk.key = "sha256 hash of compressed public key and private key";
      pk.data = pkHash;
      objValues[2] = &pk;
      writeLog = 1;

    }

    if (writeLog) {
      if (firstRecordWritten) {
        writeRecord(objValues, valuesLen, (const unsigned char*)",\n  ", outfile);
      } else {
        writeRecord(objValues, valuesLen, (const unsigned char*)"", outfile);
        firstRecordWritten = 1;
      }
      recordsWritten++;
    }
    writeLog = 0;

  } //end while loop

  fprintf(outfile, "\n]\n");

  printMetaInfo((const unsigned char*)"Completed processing the input database, proceeding to close out descriptors...");
  if (outfilepath != NULL) {
    printMetaInfo((const unsigned char*)"Closing the output file.");
    fclose(outfile);
  }

  sprintf((char*)logger, "Number of master keys found: %d.", mkeynum);
  printMetaInfo(logger);

  sprintf((char*)logger, "Number of encrypted keys found: %d.", ckeynum);
  printMetaInfo(logger);

  sprintf((char*)logger, "Number of unencrypted keys found: %d.", ukeynum);
  printMetaInfo(logger);

  if (cur != NULL) {
    printMetaInfo((const unsigned char*)"Closing the database cursor.");
    cur->close(cur);
  }

  if (db != NULL) {
    printMetaInfo((const unsigned char*)"Closing the database itself.");
    db->close(db, 0);
  }

  printMetaInfo((const unsigned char*)"Completed all tasks.");
  return (0);
}

int usage(void) {
  fprintf(stderr, "\n\n");
  fprintf(stderr, "exporter [-d absolute path to database] [-o absolute path to output json file] [-test] [-silent]");
  fprintf(stderr, "\n\n");
  fprintf(stderr, "example usage: exporter -d /tmp/wallet.dat -o /tmp/wallet.json -silent");
  fprintf(stderr, "\n");
  fprintf(stderr, "example usage: exporter -d /tmp/wallet.dat > /tmp/wallet.json");
  fprintf(stderr, "\n\n");
  fprintf(stderr, "To run test suite: exporter -test");
  fprintf(stderr, "\n\n");
  fprintf(stderr, "To run without extraneous messaging (only actual key objects): exporter -silent -d /tmp/wallet.dat");
  fprintf(stderr, "\n\n");
  return (0);
}

int testProvidedFiles(char *file) {
  if (file == NULL) {
    return (0);
  }
  FILE *in;
  in = fopen(file, "r");
  if (in == NULL) {
    return (0);
  }
  fclose(in);
  return (1);
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

//this assumes serialized little endian
int deserializeInt(const char unsigned *input, const unsigned int start) {
  return (uint32_t)((uint8_t)input[start] |
      ((uint8_t)input[start+1] << 8) |
      ((uint8_t)input[start+2] << 16) |
      ((uint8_t)input[start+3] << 24));
}

int writeRecord(const OBJ *values[], const unsigned int len, const unsigned char *prefix, FILE *file) {
  fprintf(file, "%s{\n  ", prefix);
  for(int i = 0; i < len; i++) {
    char *key = values[i]->key;
    void *value = values[i]->data;
    char *linep = (i > 0 ? ",\n  " : "");
    switch (values[i]->type) {
      case 's':
        fprintf(file, "%s  \"%s\": \"%s\"", linep, key, (char*)value);
        break;
      case 'i':
        fprintf(file, "%s  \"%s\": %d", linep, key, (*(int*)value));
        break;
      default:
        return (-1);
        break;
    }
  }
  fprintf(file, "\n  }");
  return (0);
}

void printMetaInfo(const unsigned char *message) {
  if (!silent) {
    fprintf(stdout, "\n%s\n", message);
  }
}
