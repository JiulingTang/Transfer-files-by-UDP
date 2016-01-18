#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include "Client2.h"
#include <sys/stat.h>

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
	else c = Sequence_LENGTH - head + sq+1;
	return (Hhead + c-1) % BUFFER_LENGTH;
}

Client2::Client2()
{
	/*
	Initialize WinSock
	*/
	
	WSADATA wsadata;
	if (WSAStartup(0x0202, &wsadata) != 0)
	{
		cerr << "Starting WSAStartup() error\n" << endl;
		return;
	}

	if (gethostname(localName, MAXHOSTNAMELEN) != 0)
	{
		cerr << "Get the host name error,exit" << endl;
		return;
	}

	cout << "ftp_udp starting at host: " << localName << endl;

	/*
	Initialize Socket
	*/

	if ((serSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		std::cerr << "Socket Creation Error,exit" << endl;
		return;
	}

	//cout << "Type name of ftp server: ";
	//scanf("%s", serverName);
	gethostname(serverName,MAXHOSTNAMELEN);
	hostent* h;
	if ((h = gethostbyname(serverName)) == NULL)
	{
		cout << "Can't Find Server" << endl;
		return;
	}

	/*
	Setting of target server
	*/
	serAddr.sin_family = AF_INET;
	serAddr.sin_addr.s_addr = *((unsigned long *)h->h_addr_list[0]);
	serAddr.sin_port = htons(ROUTER_PORT1);
	cliAddr.sin_family = AF_INET;
	cliAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	cliAddr.sin_port = htons(PEER_PORT1);
	log.open("log.txt");
	if (::bind(serSocket, (struct sockaddr *) &cliAddr, sizeof(cliAddr)) < 0)
	{
		cerr << "Socket Binding Error,exit" << endl;
		closesocket(serSocket);
		exit(1);
	}
}

void Client2::clean()
{
	fd_set readfds;
	/* Initialize and Set the Timer Value */
	struct timeval *tp = new timeval;
	tp->tv_sec = 0;
	tp->tv_usec = TIMEOUT_USEC; // Macro Value 300000
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
		else if (RetVal > 0)	/* There are incoming packets */
		{
			int len = sizeof(serAddr);
			recvfrom(serSocket, rmessage.s, MAXBUFSIZE, 0, (sockaddr *)&serAddr, &len);
				log << "receive " << rmessage.s << " " << *(int*)(rmessage.s + 4) << endl;
			if (strcmp(rmessage.s, "S") == 0)
			{
				memset(smessage.s, '\0', 12);
				strcpy(smessage.s, "A");
				*(int*)(smessage.s + 4) = *(int*)(rmessage.s + 4);
				sendto(serSocket, smessage.s, 12, 0, (sockaddr *)&serAddr, sizeof(serAddr));
				log << "send " << smessage.s << " " << *(int*)(smessage.s + 4) << endl;
			}
		}
		else
			break;
	}
}

void Client2::handShake()
{
	clean();
	cout << "start handshake" << endl;
	log << "start handshake" << endl;
	int len = sizeof(serAddr);
	
	int sequence = rand() % 256;
	int gsequence;
	memset(smessage.s, '\0', 12);
	strcpy(smessage.s, "s");
	*(int*)(smessage.s + 4) = sequence;
	sendto(serSocket, smessage.s, 12, 0, (sockaddr *)&serAddr, len);
	log << "send " << smessage.s << " " << *(int*)(smessage.s + 4) << endl;
	/* File Descriptor to be used with Socket */
	fd_set readfds;
	/* Initialize and Set the Timer Value */
	struct timeval *tp = new timeval;
	tp->tv_sec = 0;
	tp->tv_usec = TIMEOUT_USEC; // Macro Value 300000	

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
		else if (RetVal>0)	/* There are incoming packets */
		{
			
			if (FD_ISSET(serSocket, &readfds))	/* Incoming packet from peer host 1 */
			{
				recvfrom(serSocket, rmessage.s, MAXBUFSIZE, 0, (sockaddr *)&serAddr, &len);
				log << "receive " << rmessage.s << " " << *(int*)(rmessage.s + 4) << endl;
				if (strcmp(rmessage.s, "a") == 0)
				{
					if (*(int*)(rmessage.s + 4) == sequence)
					{
						gsequence = *(int*)(rmessage.s + 12);
					}
					strcpy(smessage.s, "a");
					*(int*)(smessage.s + 4) = gsequence;
					*(int*)(smessage.s + 12) =op;
					if (op==2||op==3)
					strcpy(smessage.s + 20, fileName);
					sendto(serSocket, smessage.s, 60, 0, (sockaddr *)&serAddr, len); 
					log << "send " << smessage.s << " " << *(int*)(smessage.s + 4) << endl;
					cout << "handshake successful" << endl;
					log << "handshake successful" << endl;
					break;
				}
				else if (strcmp(rmessage.s, "S") == 0)
				{
					memset(smessage.s, '\0', 12);
					strcpy(smessage.s, "A");
					*(int*)(smessage.s + 4) = *(int*)(rmessage.s + 4);
					sendto(serSocket, smessage.s, 12, 0, (sockaddr *)&serAddr, len);
					log << "send " << smessage.s << " " << *(int*)(smessage.s + 4) << endl;
				}
			}
		}
		else if (RetVal == 0)
		{
			memset(smessage.s, '\0', 12);
			strcpy(smessage.s, "s");
			*(int*)(smessage.s + 4) = sequence;
			sendto(serSocket, smessage.s, 12, 0, (sockaddr *)&serAddr, len);
			tp->tv_sec = 0;
			log << "resend " << smessage.s << " " << *(int*)(smessage.s + 4) << endl;
			tp->tv_usec = TIMEOUT_USEC;
		}
	}
	
}

void Client2::run()
{
while (true)
{
	cout << "Please select an operation" << endl;
	cout << "1.LIST" << endl;
	cout << "2.GET" << endl;
	cout << "3.PUT" << endl;
	cout << "4.QUIT" << endl;
	cin >> op;
	if (op == 1)
	{
		getList();
	}
	else if (op == 2)
	{
		getFile();
	}
	else if (op == 3)
	{
		putFile();
	}
	else if (op == 4)
	{
		break;
	}
}
}

void Client2::getFile()
{
	int len = sizeof(serAddr);
	cout << "Please input file name" << endl;
	cin >> fileName;
	char *p = fileName;
	struct _stat stat_buf;
	char path[40];
	strcpy(path, "files\\");
	if (_stat(strcat(path, p), &stat_buf) == 0)
	{
		cout << "file has existed" << endl;
		return;
	}

	handShake();
	hostent* h;
	if ((h = gethostbyname(serverName)) == NULL)
	{
		cout << "Can't Find Server" << endl;
		return;
	}

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
	FILE *f;
	f = fopen(path, "wb+");
	cout << "Getting file , waiting..." << endl;
	boolean fini = false;
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
				if (recvfrom(serSocket, rmessage.s, MAXBUFSIZE, 0, (sockaddr *)&serAddr, &len) == SOCKET_ERROR)
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
				sendto(serSocket, smessage.s, 12, 0, (sockaddr *)&serAddr, len);
				log << "send " << smessage.s << " " << *(int*)(smessage.s + 4) << endl;
				if (isInwindow(head, (head + BUFFER_LENGTH - 1) % Sequence_LENGTH, sequence) && v[sequence] == 0)
				{
					//cout << "sequence" << sequence << " "<<rmessage.s+20 <<endl;
					int p = countPlace(head, bHead, sequence);
					memcpy(buffer[p].s, rmessage.s, MAXBUFSIZE);
					v[sequence] = 1;
					if (sequence == head)
					{
						int i;
						for (i = 0; v[(i+head)%Sequence_LENGTH] == 1 && i < BUFFER_LENGTH; i++)
						{
							int l = *(int *)(buffer[(bHead + i) % BUFFER_LENGTH].s + 12);
							v[(head + i + BUFFER_LENGTH) % Sequence_LENGTH] = 0;
							if (strcmp(buffer[(bHead + i) % BUFFER_LENGTH].s+20, "No such file") == 0)
							{
								cout << "No such file" << endl;
								fini = true;
								break;
							}
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
							if (v[(i + head+1 ) % Sequence_LENGTH] == 0)
								break;
						}
 						head = (head + i+1) % Sequence_LENGTH;
						bHead = (bHead + i+1) % BUFFER_LENGTH;
					}
				}
		
				
			}
		
		}
		else if (RetVal == 0)
		{
			if (!start)
			{
				if (sendto(serSocket, smessage.s, 60, 0, (sockaddr *)&serAddr, len) == SOCKET_ERROR)
					cout << "wrong" << endl;
				log <<"send "<< smessage.s << " " << *(int*)(smessage.s + 4) << endl;
			}
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

void Client2::putFile()
{
	cout << "start put file" << endl;
	cout << "Please input file name" << endl;
	cin >> fileName;
	char *p = fileName;
	int head = 0;
	int nb = 0;
	int bHead = 0;
	ifstream fileToRead;
	boolean v[Sequence_LENGTH];
	memset(v, 1, sizeof(v));
	memset(v, 1, sizeof(v));
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
		cout << "No such file" << endl;
		return;
	}
	handShake();
	FILE* f;
	f = fopen(path, "rb");
	cout << "puting file " << "waiting.." << endl;
	log << "puting file " << "waiting.." << endl;
	boolean start = false;
	int len = sizeof(serAddr);
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
				if (recvfrom(serSocket, rmessage.s, MAXBUFSIZE, 0, (sockaddr *)&serAddr, &len) == SOCKET_ERROR)
					cout << "wrong" << endl;
				log << "receive " << rmessage.s << " " << rmessage.s + 4 << endl;
			}
			if (strcmp(rmessage.s, "A") == 0)
			{
				if (strcmp(rmessage.s + 4, "file exist") == 0)
				{
					cout << "file exist" << endl;
					fileToRead.close();
					fclose(f);
					return;
				}		
				if (strcmp(rmessage.s+4,"ok")==0)
				{ 
					start = true;
				}
				else
				{
					int sequence = *(int*)(rmessage.s + 4);
					if (sequence == head)
					{
						tp->tv_sec = 0;
						tp->tv_usec = TIMEOUT_USEC; // Macro Value 300000
					}
					v[sequence] = 1;
					while (v[head] == 1 && nb > 0)
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
			if (!fini&&start)
				for (int i = 0; nb<BUFFER_LENGTH; i++)
				{
				int sequence = (head + nb) % Sequence_LENGTH;
				v[sequence] = 0;
				int w = (bHead + nb) % BUFFER_LENGTH;
				Msg &m = buffer[w];
				memset(smessage.s, '\0', sizeof(smessage.s));
				strcpy(smessage.s, "S");
				*(int*)(smessage.s + 4) = sequence;
				int elength = fread(smessage.s + 20, sizeof(char), MAXBUFSIZE - 20, f);
				*(int*)(smessage.s + 12) = elength;
				sendto((SOCKET)serSocket, smessage.s, elength + 20, 0, (sockaddr*)&serAddr, sizeof(serAddr));
				log << "send " << smessage.s << " " << *(int*)(smessage.s + 4) << endl;
				if (elength < MAXBUFSIZE - 20)
					fini = true;
				memcpy(m.s, smessage.s, MAXBUFSIZE);
				nb++;
				if (fini)
					break;
				}

			

		}
		else if (RetVal == 0)
		{
			if (!start)
			{
				if (sendto(serSocket, smessage.s, 60, 0, (sockaddr *)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
					cout << "wrong" << endl;
				log << "rsend " << smessage.s << " " << *(int*)(smessage.s + 4) << endl;
			}
			else
			{
				for (int i = 0; i < nb; i++)
				{
					if (v[(head + i) % Sequence_LENGTH] == 0)
					{
						int w = (bHead + i) % BUFFER_LENGTH;
						sendto((SOCKET)serSocket, buffer[w].s, MAXBUFSIZE, 0, (sockaddr*)&serAddr, sizeof(serAddr));
						log << "resend " << buffer[w].s << " " << *(int*)(buffer[w].s + 4) << endl;
					}
				}
			}
			tp->tv_sec = 0;
			tp->tv_usec = TIMEOUT_USEC;
		}
	}
	cout << "end put file" << endl;
	log << "end put file" << endl;
	fileToRead.close();
	fclose(f);
}

void Client2::getList()
{
	int len = sizeof(serAddr);

	handShake();
	
	
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
	cout << "Getting list , waiting..." << endl;
	log << "Getting list , waiting..." << endl;
	boolean fini = false;
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
				if (recvfrom(serSocket, rmessage.s, MAXBUFSIZE, 0, (sockaddr *)&serAddr, &len) == SOCKET_ERROR)
					cout << "wrong" << endl;
				log <<"receive "<< rmessage.s << " " << *(int*)(rmessage.s + 4) << endl;
			}
			if (strcmp(rmessage.s, "S") == 0)
			{
				start = true;
				int sequence = *(int*)(rmessage.s + 4);
				memset(smessage.s, '\0', 12);
				strcpy(smessage.s, "A");
				*(int*)(smessage.s + 4) = sequence;
				sendto(serSocket, smessage.s, 12, 0, (sockaddr *)&serAddr, len);
				log << "send " << smessage.s << " " << *(int*)(smessage.s + 4) << endl;
				if (isInwindow(head, (head + BUFFER_LENGTH - 1) % Sequence_LENGTH, sequence) && v[sequence] == 0)
				{
					
					int p = countPlace(head, bHead, sequence);
					memcpy(buffer[p].s, rmessage.s, MAXBUFSIZE);
					v[sequence] = 1;
					if (sequence == head)
					{
						int i;
						for (i = 0; v[(i + head) % Sequence_LENGTH] == 1 && i < BUFFER_LENGTH; i++)
						{
							v[(head + i + BUFFER_LENGTH) % Sequence_LENGTH] = 0;
							int w = (bHead + i) % BUFFER_LENGTH;
							int l = 12;
							while (strlen(buffer[w].s + l) != 0&&l<MAXBUFSIZE)
							{
								cout << buffer[w].s + l << endl;
								l = l + strlen(buffer[w].s+l)+1;
							}
							if (strlen(buffer[w].s+12)==0)
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

		}
		else if (RetVal == 0)
		{
			if (!start)
			{
				if (sendto(serSocket, smessage.s, 60, 0, (sockaddr *)&serAddr, len) == SOCKET_ERROR)
					cout << "wrong" << endl;
				log <<"resend "<< smessage.s << " " << *(int*)(smessage.s + 4) << endl;
			}
			if (fini)
				break;
			tp->tv_sec = 0;
			tp->tv_usec = TIMEOUT_USEC;
		}
	}
	cout << "end get list" << endl;
	log << "end get list" << endl;
}

Client2::~Client2()
{
	closesocket(serSocket);
	log.close();
}

int main()
{
	Client2 client;
	client.run();
	getchar();
}