#!/bin/bash

HOST="127.0.0.1"
PORT="8080"
FILE_ENTRY="./index.html"

# g++ server.cpp -o ./bin/server  --std=c++11 
# ./bin/server ${HOST} ${PORT} ${FILE_ENTRY}

g++ server_sendfile.cpp -o ./bin/server_sendfile  --std=c++11 
./bin/server_sendfile ${HOST} ${PORT} ${FILE_ENTRY}