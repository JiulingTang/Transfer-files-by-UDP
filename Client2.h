#ifndef Client2_H
#define Client2_H

#pragma comment( linker, "/defaultlib:ws2_32.lib" )

#include <winsock2.h>
#include <fstream>
#include <iostream>
#include <time.h>
#include <list>
#include <stdio.h>

using namespace std;

#define MAXBUFSIZE 2000			//maximum packet size
#define MAXHOSTNAMELEN 256			//maximum length of host name
#define ROUTER_PORT1 7000			//router port number 1
#define ROUTER_PORT2 7001			//router port number 2
#define PEER_PORT1 5000				//port number of peer host 1
#define PEER_PORT2 5001				//port number of peer host 2
#define TIMEOUT_USEC 300000			//time-out value
#define BUFFER_LENGTH 3
#define Sequence_LENGTH 7

typedef struct message
{
	char s[MAXBUFSIZE];
}Msg;

class Client2
{
public:
	Client2();
	~Client2();
	void clean();
	void run();
private:
	Msg rmessage;
	Msg smessage;
	int serSocket;
	struct sockaddr_in serAddr;
	struct sockaddr_in cliAddr;
	char serverName[MAXHOSTNAMELEN];
	char localName[MAXHOSTNAMELEN];
	ofstream logFile;
	Msg buffer[BUFFER_LENGTH];
	int needAck;
	void handShake();
	int op;
	char fileName[40];
	void getList();
	void getFile();
	void putFile();
	ofstream log;
};

#endif 