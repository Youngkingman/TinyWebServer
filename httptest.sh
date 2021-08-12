#!/bin/bash
HOST="127.0.0.1"
PORT="8080"
FILE_ENTRY="./index.html"

g++ test/http_test.cpp -o ./bin/http_test  --std=c++11
./bin/http_test ${HOST} ${PORT} ${FILE_ENTRY}