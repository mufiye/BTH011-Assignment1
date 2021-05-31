#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* You will to add includes here */
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <math.h>

// Included to get the support library
#include <calcLib.h>

#define DEBUG
#define BACKLOG 3 //队列的最大容量,对应的真实队列长度为5

using namespace std;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

//要求
//1. The server should accept IP:port the first command-line argument.
//2. The server should handle ONE client at a time.
//3. Five (5) clients should be able to queue for service, the sixth should be rejected.
//4. The server should only support protocol "TEXT TCP 1.0".
//5. If a client takes longer than 5s to complete any task (send a reply), the server should terminate (send 'ERROR TO\n' to the client), close the connection.
int main(int argc, char *argv[])
{
	if (argc != 2)
	{
      printf("you should press in the port number\n");
	  exit(1);
	}
	char *Destport = argv[1];
	int port = atoi(Destport);
#ifdef DEBUG
	printf("the port is %d.\n",port);
#endif

	int sockfd, new_fd; //一个监听的端口，一个用于连接的端口
	struct sockaddr_in serverAddr;
	//int rv;

	serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	//socket()
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	//bind()
    if(bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
	{
       perror("bind fail\n");
	   exit(1);
	}
    
	printf("server: waiting for connections...\n");
	char msg[1500];
	timeval timeout; //定时变量
	fd_set rfd;		 //读描述符
	int nRet;		 //select返回值
	int maxfdp;
	int numbytes;
	int state;
	int inValue1, inValue2, inResult;
	float fValue1, fValue2, fResult;

	//listen()
	if (listen(sockfd, BACKLOG) == -1)
	{
		perror("listen\n");
		exit(1);
	}

	while (1)
	{
		socklen_t sin_size;
		struct sockaddr_in connector_addr;
		sin_size = sizeof(connector_addr);
		//accept()
		new_fd = accept(sockfd, (struct sockaddr *)&connector_addr, &sin_size);
		if (new_fd == -1)
		{
			perror("accept fail\n");
			exit(1);
		}
#ifdef DEBUG
        //展示连接的客户端的地址和端口号
        int show_port = ntohs(connector_addr.sin_port);
		char show_addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET,&connector_addr.sin_addr, show_addr, sizeof(show_addr));
		printf("----------------------------------------\n");
        printf("accept the %s:%d\n",show_addr,show_port);
		printf("the new created socket is %d\n",new_fd);
#endif
		memset(&msg, 0, sizeof(msg));
		sprintf(msg, "TEXT TCP 1.1\nTEXT TCP 1.0\n\n");
		//发送初始化报文告知允许的协议
		if (send(new_fd, &msg, strlen(msg), 0) == -1)
		{
			perror("send TEXT TCP 1.0 fail\n");
			exit(1);
		}
		else{
			printf("send:\n%s",msg);
		}
		fValue1 = 0, fValue2 = 0, fResult = 0;
		inValue1 = 0, inValue2 = 0, inResult = 0;
		//sleep(5);
		while (1)
		{
			//设置超时
			timeout.tv_sec = 5;//验证backlog时需要将其值设置得大一些
			timeout.tv_usec = 0;
			FD_ZERO(&rfd);		  // 在使用之前总是要清空
			FD_SET(new_fd, &rfd); // 把sockfd放入要测试的描述符中
			maxfdp = new_fd + 1;
			nRet = select(maxfdp, &rfd, NULL, NULL, &timeout); //检测是否有套接口是否可读
			if (nRet == -1)
			{
				perror("select function error\n");
				exit(1);
			}
			else if (nRet == 0)
			{ //如果超时
#ifdef DEBUG
				printf("socket%d is timeout\n", new_fd);
#endif
				memset(&msg, 0, sizeof(msg));
				sprintf(msg, "ERROR TO\n");
				if (send(new_fd, &msg, strlen(msg), 0) == -1)
				{
					perror("send error\n");
					exit(1);
				}
#ifdef DEBUG
				printf("close socket %d for timeout\n", new_fd);
#endif
				close(new_fd);
				break;
			}
			else
			{ //没有超时
				memset(&msg, 0, sizeof(msg));
				if ((numbytes = recv(new_fd, msg, sizeof(msg), 0)) == -1)
				{
					perror("receive fail\n");
					exit(1);
				}
				if (strcmp(msg, "OK\n") == 0)
				{ //如果收到的消息是OK
				    printf("receive OK about protocol\n");
					char *operateType = randomType();
					if (operateType[0] == 'f')
					{ //浮点数操作
						state = 2;
						fValue1 = randomFloat();
						fValue2 = randomFloat();
						char str1[20];
						char str2[20];
						sprintf(str1, "%8.8g", fValue1);
						sprintf(str2, "%8.8g", fValue2);
						fValue1 = atof(str1);
						fValue2 = atof(str2);
						if (strcmp(operateType, "fadd") == 0)
						{
							fResult = fValue1 + fValue2;
						}
						else if (strcmp(operateType, "fsub") == 0)
						{
							fResult = fValue1 - fValue2;
						}
						else if (strcmp(operateType, "fmul") == 0)
						{
							fResult = fValue1 * fValue2;
						}
						else
						{
							while (fValue2 == 0.0)
							{
								fValue2 = randomFloat();
							}
							fResult = fValue1 / fValue2;
						}
						char str3[20];
						sprintf(str3, "%8.8g", fResult);
						fResult = atof(str3);
						memset(&msg, 0, sizeof(msg));
						sprintf(msg, "%s %8.8g %8.8g\n", operateType, fValue1, fValue2);//将assignment写入报文中
					}
					else
					{ //整数操作
						state = 1;
						inValue1 = randomInt();
						inValue1 = randomInt();
						if (strcmp(operateType, "add") == 0)
						{
							inResult = inValue1 + inValue2;
						}
						else if (strcmp(operateType, "sub") == 0)
						{
							inResult = inValue1 - inValue2;
						}
						else if (strcmp(operateType, "mul") == 0)
						{
							inResult = inValue1 * inValue2;
						}
						else
						{
							while (inValue2 == 0)
							{
								inValue2 = randomInt();
							}
							inResult = inValue1 / inValue2;
						}
						memset(&msg, 0, sizeof(msg));
						sprintf(msg, "%s %d %d\n", operateType, inValue1, inValue2);//将assignment写入报文中
					}
//send()
#ifdef DEBUG
					printf("send: %s", msg);
#endif
					if (send(new_fd, &msg, strlen(msg), 0) == -1)
					{
						perror("send int error");
						exit(1);
					}
				}
				else
				{ //如果收到消息是一个result
#ifdef DEBUG
					printf("received result is: %s", msg);
#endif
					bool isRight;
					if (state == 1) //整数运算
					{
						int num = atoi(msg);
						if (abs(num - inResult) < 0.0001)
							isRight = true;
						else
							isRight = false;
					}
					else //浮点数运算
					{
						double num = atof(msg);
						if (fabs(num - fResult) < 0.0001)
							isRight = true;
						else
							isRight = false;
					}
					if (isRight)
					{//如果结果正确
#ifdef DEBUG
						printf("result is right\n");
#endif
						memset(&msg, 0, sizeof(msg));
						sprintf(msg, "OK\n");
					}
					else
					{//如果结果错误
#ifdef DEBUG
						printf("result is error\n");
#endif
						memset(&msg, 0, sizeof(msg));
						sprintf(msg, "ERROR\n");
					}
					if (send(new_fd, &msg, strlen(msg), 0) == -1)
					{//发送结果报文
						perror("send Message error\n");
						exit(1);
					}
#ifdef DEBUG
					printf("close socket %d\n", new_fd);
#endif
					close(new_fd);
					break;
				}
			}
		}
	}
	//close()
	close(sockfd);
	printf("the sockfd has been closed\n");
	return 0;
}
