#!/bin/bash
echo "int main(){return 0;}" | g++ -o/dev/null -ldb_cxx-4.7 -xc++ - > /dev/null 2>&1
if [ $? == 0 ]; then
  if [ "$1" == "lib" ]; then
    echo -n "-ldb_cxx-4.8"
  else
    echo -n ""
  fi
else
  if [ "$1" == "dep" ]; then
    echo "./dependencies/db-4.8.30.NC/bdb.gyp:bdb"
  fi
fi
