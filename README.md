# ClamHTML-LoopbackServer
ClamHTML - Loopback Server - part 1 of 2

ClamHTML is an application that provides scanning of web resources using free an open source softwares, providing a safer web browsing experience.

The Loopback Server is the first part of this 2 part system. It is a internal proxy that acts as a relay between the web browser, and the clamd antivirus daemon. It opens a internal listener, and waits for the browser to make requests to it. Once it recieves a requests, it's queries the clamd socket interface, waits for the response from clamd, then sends that result back to the browser.

The application is easy to use, there are no settings. It's either running or it's not. An icon will appear in the tasktray if it's running.

To compile:
cd /path/to/project/folder/
mkdir build
cd build/
qmake ../ClamHTML.pro
make

to run:
./ClamHTML
