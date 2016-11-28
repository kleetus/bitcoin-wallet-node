#!/bin/bash
echo "int main(){return 1;}" | g++ -o/dev/null -ldb_cxx-4.8 -xc++ - >/dev/null 2>&1
if [ $? == 0 ]; then
  echo "\"lib\""
else
  echo "\"dep\""
fi
