## COS 460/540 - Computer Networks
# Project 2: HTTP Server

# Jove Emmons

This project is written in C on Windows.

## How to compile

This uses GCC to compile

To compile:
gcc -c -Wall -g server.c
To link:
gcc -g -LC:\Windows\msys64 server.o -lws2_32 -liphlpapi -o server

## How to run

The executable file will likely be called server.exe. Run it with ".\server.exe"

## My experience with this project

My experience with this was not very good. I wrote it on Linux at first, but faced some issues with getting my code to cooperate with browsers. I also faced issues with memory allocation, and ended up not giving myself enough time to get things figured out.

After the original deadline I decided to try to port my code to windows so that it connecting to a browser would be easier. However, that meant using the Windows version of C, which meant I had to translate my code. I'm not very familiar with what is or isn't standard for C Windows headers, so its possible that my new code won't compile on another machine.