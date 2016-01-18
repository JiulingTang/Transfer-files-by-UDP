#define _CRT_SECURE_NO_WARNINGS
#include "Server2.h"

#include <iostream>
#include <windows.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstdio>
#include <fstream>
#include <process.h>
#include <mutex>
#include <io.h>

using namespace std;

boolean isInwindow(int head, int tail, int sq)
{
	if (tail >= head)
	{
		if (sq >= head&&sq <= tail)
			return true;
		else
			return false;
	}
	else
	{
		if (sq >= head || sq <= tail)
			return true;
		else return false;
	}
}

int countPlace(int head, int Hhead, int sq)
{
	int c;
	if (sq >= head)
		c = sq - head + 1;
	else c = Sequence_LENGTH - head + sq + 1;
	return (Hhead + c - 1) % BUFFER_LENGTH;
}
Server2::Server2() 
{
	WSADATA wsadata;
	if (WSAStartup(0x0202, &wsadata) != 0)
	{
		cerr << "Starting WSAStartup() error\n" << endl;
		exit(1);
	}

	serverName[MAXHOSTNAMELEN];
	if (gethostname(serverName,MAXHOSTNAMELEN) != 0)
	{
		cerr << "Get the host name error,exit" << endl;
		exit(1);
	}

	cout << "ftpd_udp starting at host: " << serverName << endl;


	if ((serSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		std::cerr << "Socket Creation Error,exit" << endl;
		exit(1);
	}

	serAddr.sin_family = AF_INET;
	serAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serAddr.sin_port = htons(PEER_PORT2);

	if (::bind(serSocket, (struct sockaddr *) &serAddr, sizeof(serAddr)) < 0)
	{
		cerr << "Socket Binding Error,exit" << endl;
		closesocket(serSocket);
		exit(1);
	}
	log.open("log.txt");
}



int  Server2::handShake()
{
	Msg rmessage;
	Msg smessage;
	int sequence;
	int len = sizeof(cliAddr);
	cout << "start handshake" << endl;
	log << "start handshake" << endl;
	while(1)
	{
		recvfrom(serSocket, rmessage.s, MAXBUFSIZE, 0, (sockaddr *)&cliAddr, &len);
		log << "receive " << rmessage.s << " " << *(int*)(rmessage.s + 4) << endl;
		if (strcmp(rmessage.s, "s") == 0)
		{
			int sq = *(int*)(rmessage.s + 4);
			memset(smessage.s + 4, '\0', 8);
			strcpy(smessage.s, "a");
			*(int*)(smessage.s + 4) = sq;
			srand((int)time(NULL));
			sequence = rand() % 256;
			memset(smessage.s + 12, '\0', 8);
			*(int*)(smessage.s + 12) = sequence;
			sendto(serSocket, smessage.s, 20, 0, (sockaddr *)&cliAddr, len);
			log << "send " << smessage.s << " " << *(int*)(smessage.s + 4) << endl;
			break;
		}
		else if (strcmp(rmessage.s, "S") == 0)
		{
			memset(smessage.s, '\0', 12);
			strcpy(smessage.s, "A");
			*(int*)(smessage.s + 4) = *(int*)(rmessage.s + 4);
			sendto(serSocket, smessage.s, 12, 0, (sockaddr *)&cliAddr, len);
			log << "send " << smessage.s << " " << *(int*)(smessage.s + 4) << endl;
		}

	}
	while (true)
	{
		recvfrom(serSocket, rmessage.s, MAXBUFSIZE, 0, (sockaddr *)&cliAddr, &len);
		log << "receive " << rmessage.s << " " << *(int*)(rmessage.s + 4) << endl;
		if (strcmp(rmessage.s, "s") == 0)
		{
			sendto(serSocket, smessage.s, 20, 0, (sockaddr *)&cliAddr, len);
			log << "send " << smessage.s << " " << *(int*)(smessage.s + 4) << endl;
		}
		else if (strcmp(rmessage.s, "a")==0)
		{
			int sq = *(int*)(rmessage.s + 4);
			if (sq == sequence)
			{
				
				int op;
				op = *(int*)(rmessage.s + 12);
				if (op == 2 || op == 3)
					strcpy(fileName, rmessage.s + 20);
				cout << "handshake successful" << endl;
				log << "handshake successful" << endl;
				return op;
			}
		}
	
	}
}

void Server2::run()
{
	while (true)
	{
		int op = handShake();
		if (op == 1)
			sendList();
		else if (op == 2)
			sendFile();
		else if (op == 3)
			getFile();
	}
}

void Server2::sendFile()
{
	cout << "start send file" << endl;
	log << "start send file" << endl;
	int head = 0;
	int nb = 0;
	int bHead = 0;
	ifstream fileToRead;
	boolean v[Sequence_LENGTH];
	memset(v, 1, sizeof(v));
	boolean nofile = false;
	boolean fini = false;
	fd_set readfds;
	struct timeval *tp = new timeval;
	tp->tv_sec = 0;
	tp->tv_usec = TIMEOUT_USEC; // Macro Value 300000
	char path[40] = "files\\"; 
	strcat(path, fileName);
	fileToRead.open(path, ios::binary | ios::in | ios::out);
	struct _stat stat_buf;
	if (_stat(path, &stat_buf) != 0)
	{	
		nofile = true;
	}
	FILE* f;
	if (!nofile)
	{
		f = fopen(path, "rb");
		cout << "Sending file " << "waiting.." << endl;
		log << "Sending file " << "waiting.." << endl;
	}
	while (1)
	{
		if (!fini)
		for (int i = 0; nb<BUFFER_LENGTH; i++)
		{
				int sequence = (head + nb) % Sequence_LENGTH;
				v[sequence] = 0;
				int w = (bHead + nb ) % BUFFER_LENGTH;
				Msg &m = buffer[w];
				memset(smessage.s, '\0',sizeof(smessage.s));
				strcpy(smessage.s, "S");
				*(int*)(smessage.s + 4) = sequence;
				if (nofile)
				{
					strcpy(smessage.s+20, "No such file");
					fini = true;
					sendto((SOCKET)serSocket, smessage.s, 60, 0, (sockaddr*)&cliAddr, sizeof(cliAddr));
					log << "send " << smessage.s << " " << *(int*)(smessage.s + 4) << endl;
				}
				else
				{
					int elength = fread(smessage.s + 20, sizeof(char), MAXBUFSIZE - 20, f);
					*(int*)(smessage.s + 12) = elength;
					sendto((SOCKET)serSocket, smessage.s, elength + 20, 0, (sockaddr*)&cliAddr, sizeof(cliAddr));
					log << "send " << smessage.s << " " << *(int*)(smessage.s + 4) << endl;
					if (elength < MAXBUFSIZE - 20)
						fini = true;
				}
				memcpy(m.s, smessage.s,MAXBUFSIZE); 
				nb++;
				if (fini)
					break;
		}

		int RetVal;
		FD_ZERO(&readfds);
		FD_SET(serSocket, &readfds);
		if ((RetVal = select(1, &readfds, NULL, NULL, tp)) == SOCKET_ERROR)	/* Select call waits for the specified time */
		{
			cerr << "Timer error!" << endl;
			return;
		}
		if (RetVal > 0)
		{
			int len=sizeof(cliAddr);
			recvfrom(serSocket, rmessage.s, 40, 0, (sockaddr *)&cliAddr, &len);
			log << "receive " << rmessage.s << " " << *(int*)(rmessage.s + 4) << endl;
			if (strcmp(rmessage.s, "A") == 0)
			{
				int sequence = *(int*)(rmessage.s + 4); 
				if (sequence == head)
				{
					tp->tv_sec = 0;
					tp->tv_usec = TIMEOUT_USEC; // Macro Value 300000
				}
				v[sequence] = 1;
				while (v[head] == 1&&nb>0)
				{
					head = (head + 1) % Sequence_LENGTH;
					bHead = (bHead + 1) % BUFFER_LENGTH;
					nb--;
				}
				if (fini&&nb == 0)
				{
					break;
				}

			}
		}
		else if (RetVal == 0)
		{
			for (int i = 0; i < nb; i++)
			{
				if (v[(head + i) % Sequence_LENGTH] == 0)
				{
					int w = (bHead + i) % BUFFER_LENGTH;
					sendto((SOCKET)serSocket, buffer[w].s, MAXBUFSIZE, 0, (sockaddr*)&cliAddr, sizeof(cliAddr));
					log << "resend " << buffer[w].s << " " << *(int*)(buffer[w].s + 4) << endl;
				}
				
			}
			
			tp->tv_sec = 0;
			tp->tv_usec = TIMEOUT_USEC; // Macro Value 300000
		}

	}
	if (f!=NULL)
	fclose(f);
	fileToRead.close();
	cout << "end send file" << endl;
	log << "end send file" << endl;

}

void Server2::getFile()
{
	cout << "start get file" << endl;
	log << "start get file" << endl;
	boolean exist = false;
	struct _stat stat_buf;
	char path[40];
	strcpy(path, "files\\");
	char *p = fileName;
	if (_stat(strcat(path, p), &stat_buf) == 0)
	{
		exist = true;
	}
	FILE *f;
	f = fopen(path, "wb+");
	fd_set readfds;
	/* Initialize and Set the Timer Value */
	struct timeval *tp = new timeval;
	tp->tv_sec = 0;
	tp->tv_usec = TIMEOUT_USEC; // Macro Value 300000
	int head = 0;
	int bHead = 0;
	boolean start = false;
	int v[Sequence_LENGTH];
	memset(v, 0, sizeof(v));
	boolean fini = false;
	int len = sizeof(serAddr);
	strcpy(smessage.s, "A");
	if (!exist)
		strcpy(smessage.s + 4, "ok");
	else
	{
		strcpy(smessage.s + 4, "file exist");
		fini = true;
	}
	if (sendto(serSocket, smessage.s, 60, 0, (sockaddr *)&cliAddr, sizeof(cliAddr)) == SOCKET_ERROR)
		cout << "wrong" << endl;
	while (1)
	{
		int RetVal;
		FD_ZERO(&readfds);
		FD_SET(serSocket, &readfds);
		if ((RetVal = select(1, &readfds, NULL, NULL, tp)) == SOCKET_ERROR)	/* Select call waits for the specified time */
		{
			cerr << "Timer error!" << endl;
			return;
		}
		else if (RetVal > 0)
		{
			if (FD_ISSET(serSocket, &readfds))
			{
				if (recvfrom(serSocket, rmessage.s, MAXBUFSIZE, 0, (sockaddr *)&cliAddr, &len) == SOCKET_ERROR)
					cout << "wrong" << endl;
				log << "receive " << rmessage.s << " " << *(int*)(rmessage.s + 4) << endl;
			}
			if (strcmp(rmessage.s, "S") == 0)
			{
				start = true;
				int sequence = *(int*)(rmessage.s + 4);
				memset(smessage.s, '\0', 12);
				strcpy(smessage.s, "A");
				*(int*)(smessage.s + 4) = sequence;
				sendto(serSocket, smessage.s, 12, 0, (sockaddr *)&cliAddr, len);
				log << "send " << smessage.s << " " << *(int*)(smessage.s + 4) << endl;
				if (isInwindow(head, (head + BUFFER_LENGTH - 1) % Sequence_LENGTH, sequence) && v[sequence] == 0)
				{
					//cout << "sequence" << sequence << " " << rmessage.s + 20 << endl;
					int p = countPlace(head, bHead, sequence);
					memcpy(buffer[p].s, rmessage.s, MAXBUFSIZE);
					v[sequence] = 1;
					if (sequence == head)
					{
						int i;
						for (i = 0; v[(i + head) % Sequence_LENGTH] == 1 && i < BUFFER_LENGTH; i++)
						{
							int l = *(int *)(buffer[(bHead + i) % BUFFER_LENGTH].s + 12);
							v[(head + i + BUFFER_LENGTH) % Sequence_LENGTH] = 0;
				
							fwrite(buffer[(bHead + i) % BUFFER_LENGTH].s + 20, sizeof(char), l, f);
							if (l < MAXBUFSIZE - 20)
							{
								fini = true;
								tp->tv_sec = 0;
								tp->tv_usec = 0;
								break;
							}
							if (i == BUFFER_LENGTH - 1)
								break;
							if (v[(i + head + 1) % Sequence_LENGTH] == 0)
								break;
						}
						head = (head + i + 1) % Sequence_LENGTH;
						bHead = (bHead + i + 1) % BUFFER_LENGTH;
					}
				}


			}
			else if (strcmp(rmessage.s,"a")==0)
			{
				if (sendto(serSocket, smessage.s, 60, 0, (sockaddr *)&cliAddr, sizeof(cliAddr)) == SOCKET_ERROR)
					cout << "wrong" << endl;
				log << "rsend " << smessage.s << " " << *(int*)(smessage.s + 4) << endl;
			}

		}
		else if (RetVal == 0)
		{
		
			if (fini)
				break;
			tp->tv_sec = 0;
			tp->tv_usec = TIMEOUT_USEC;
		}
	}
	fflush(f);
	fclose(f);
	cout << "end get file" << endl;
	log << "end get file" << endl;
}

void Server2::sendList()
{
	cout << "start send list" << endl;
	log << "start send list" << endl;
	int head = 0;
	int nb = 0;
	int bHead = 0;
	ifstream fileToRead;
	boolean v[Sequence_LENGTH];
	memset(v, 0, sizeof(v));
	boolean nofile = false;
	boolean fini = false;
	fd_set readfds;
	struct timeval *tp = new timeval;
	tp->tv_sec = 0;
	tp->tv_usec = TIMEOUT_USEC; // Macro Value 300000
	_finddata_t finfo;
	long hfile = _findfirst("files\\*.*", &finfo);
	int c = 0;
	while (1)
	{
		    if (!fini)
			for (int i = 0; nb<BUFFER_LENGTH; i++)
				if (!fini)
			{
			int sequence = (head + nb) % Sequence_LENGTH;
			v[sequence] = 0;
			int w = (bHead + nb) % BUFFER_LENGTH;
			Msg &m = buffer[w];
			memset(smessage.s, '\0', sizeof(smessage.s));
			strcpy(smessage.s, "S");
			*(int*)(smessage.s + 4) = sequence;
			int l = 12;
			if (c == 1)
			{
				strcpy(smessage.s + l, finfo.name);
				l = l + strlen(finfo.name)+1;
			}
			c = 0;
			while (_findnext(hfile, &finfo) != -1)
			{
				
				if (l + strlen(finfo.name)+1 > MAXBUFSIZE)
				{
					c = 1;
					break;
				}
				if (strcmp(".", finfo.name) == 0 || strcmp("..", finfo.name) == 0)
				{
					continue;
				}
				
				
				strcpy(smessage.s + l, finfo.name);
				l = l + strlen(finfo.name)+1;
			}
			sendto((SOCKET)serSocket, smessage.s, MAXBUFSIZE, 0, (sockaddr*)&cliAddr, sizeof(cliAddr));
			log << "send " << smessage.s << " " << *(int*)(smessage.s + 4) << endl;
			if (strlen(smessage.s + 12) == 0)
				fini = true;
			
			memcpy(m.s, smessage.s, MAXBUFSIZE);
			nb++;
			if (fini)
				break;
			}

		int RetVal;
		FD_ZERO(&readfds);
		FD_SET(serSocket, &readfds);
		if ((RetVal = select(1, &readfds, NULL, NULL, tp)) == SOCKET_ERROR)	/* Select call waits for the specified time */
		{
			cerr << "Timer error!" << endl;
			return;
		}
		if (RetVal > 0)
		{
			int len = sizeof(cliAddr);
			recvfrom(serSocket, rmessage.s, 40, 0, (sockaddr *)&cliAddr, &len);
			log << "receive " << rmessage.s << " " << *(int*)(rmessage.s + 4) << endl;
			if (strcmp(rmessage.s, "A") == 0)
			{
				int sequence = *(int*)(rmessage.s + 4);
				if (sequence == head)
				{
					tp->tv_sec = 0;
					tp->tv_usec = TIMEOUT_USEC; // Macro Value 300000
				}
				v[sequence] = 1;
				while (v[head] == 1 && nb>0)
				{
					head = (head + 1) % Sequence_LENGTH;
					bHead = (bHead + 1) % BUFFER_LENGTH;
					nb--;
				}
				if (fini&&nb == 0)
				{
					break;
				}

			}
		}
		else if (RetVal == 0)
		{
			sendto((SOCKET)serSocket, buffer[bHead].s, MAXBUFSIZE, 0, (sockaddr*)&cliAddr, sizeof(cliAddr));
			log << "resend " << buffer[bHead].s << " " << *(int*)(buffer[bHead].s + 4) << endl;
			tp->tv_sec = 0;
			tp->tv_usec = TIMEOUT_USEC; // Macro Value 300000
		}

	}
	cout << "end send list" << endl;
	log << "end send list" << endl;
}

Server2::~Server2()
{
}

int main()
{
	Server2 server;
	server.run();
	getchar();
}













