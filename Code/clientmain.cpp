#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* You will to add includes here */
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
// Enable if you want debugging to be printed, see examble below.
// Alternative, pass

#define DEBUG
#define MAXDATASIZE 1400

// Included to get the support library
#include <calcLib.h>

//一些要求
//1. The reference server (subject to change), is reached via IPv4 13.53.76.30:5000.
//2. When a client connects, then the server sends a string which protocol(s) it supports.
//3. If the client accepts ANY of the protocols, it has to respond with 'OK\n'. If it does not, then it should disconnect.
//4. your client should accept a string as input, the string should be in the <IP>:<PORT> syntax.
int main(int argc, char *argv[])
{

  /*
    Read first input, assumes <ip>:<port> syntax, convert into one string (Desthost) and one integer (port). 
     Atm, works only on dotted notation, i.e. IPv4 and DNS. IPv6 does not work if its using ':'. 
  */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s hostname (%d)\n", argv[0], argc);
    exit(1);
  }
  char delim[] = ":";                      //分割符号:
  char *Desthost = strtok(argv[1], delim); //目的主机号
  char *Destport = strtok(NULL, delim);    //目的端口号
  if(Destport == NULL) return 0;
  // *Desthost now points to a sting holding whatever came before the delimiter, ':'.
  // *Dstport points to whatever string came after the delimiter.

  /* Do magic */
  int port = atoi(Destport);
  printf("----------------------------------------\n");
  printf("Host %s, and port %d.\n", Desthost, port);

  //设置一些套接字属性
  int sockfd;
  struct addrinfo hints, *servinfo, *p;
  int rv;
  int numbytes;
  char buf[MAXDATASIZE];

  //服务器回复的相关字符串
  char refStr[] = "TEXT TCP 1.0";
  char okStr[] = "OK\n";
  char errorStr[] = "ERROR\n";
  char errorToStr[] = "ERROR TO\n";

  //运算符
  char add[] = "add";
  char div[] = "div";
  char mul[] = "mul";
  char sub[] = "sub";
  char fadd[] = "fadd";
  char fdiv[] = "fdiv";
  char fmul[] = "fmul";
  char fsub[] = "fsub";

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;     //表示不确定是IPV4还是IPV6
  hints.ai_socktype = SOCK_STREAM; //字节流类型

  if ((rv = getaddrinfo(Desthost, Destport, &hints, &servinfo)) != 0)
  {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  //socket()
  // loop through all the results and make a socket
  for (p = servinfo; p != NULL; p = p->ai_next)
  {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
    {
      perror("talker: socket");
      continue;
    }

    break;
  }

  if (p == NULL)
  {
    fprintf(stderr, "talker: failed to create socket\n");
    return 2;
  }

  //connect()
  rv = connect(sockfd, p->ai_addr, p->ai_addrlen);
#ifdef DEBUG
  //printf("rv is %d\n",rv);
#endif
  if (rv < 0){
    perror("client: connect fail.\n");
    close(sockfd);
    exit(1);
  }
#ifdef DEBUG
  else
  {
    //获得本地地址信息并打印
    char myAddress[20];

    struct sockaddr_in local_sin;
    socklen_t local_sinlen = sizeof(local_sin);
    getsockname(sockfd, (struct sockaddr *)&local_sin, &local_sinlen);

    inet_ntop(local_sin.sin_family, &local_sin.sin_addr, myAddress, sizeof(myAddress));
    printf("Connected to %s:%s  local %s:%d\n", Desthost, Destport, myAddress, ntohs(local_sin.sin_port));
  }
#endif
  //sleep(200);//用于演示backlog
  //recv()
  if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1)
  {
    perror("recv");
    exit(1);
  }
  buf[numbytes] = '\0'; //设置buff字符串的结束标记
  
  //buf里面存放的是所有的protocol信息
#ifdef DEBUG
  printf("receive:\n%s",buf);
#endif
  bool isFind = false;
  char *tokStr = strtok(buf, "\n"); 
  while(tokStr != nullptr){
    if (strcmp(tokStr, refStr) == 0)
    {//如果收到的协议可以接受
      isFind = true;
      break;
    }
    tokStr = strtok(NULL, "\n");
  }
  if (isFind)
  {//如果收到的协议可以接受
    if (send(sockfd, okStr, strlen(okStr), 0) == -1)
    {
      perror("sendto fail");
      exit(1);
    }
    else{
      printf("send OK about protocol\n");
    }
  }
  else
  {//如果收到的协议不能接受
    perror("protocol wrong");
    exit(1);
  }

  char myResult[18];

  while (1)
  {
    memset(&buf, 0, sizeof buf);
    if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1)
    {//接收assignment
      perror("recv");
      exit(1);
    }
    if (strcmp(buf, errorToStr) == 0)
    { //接收到提示超时的消息
      printf("receive %smeans timeout\n", buf);
      break;
    }
    else if (strcmp(buf, okStr) == 0)
    { //收到了ok的消息
      char *okStr2 = strtok(okStr, "\n");
      char *myResult2 = strtok(myResult, "\n");
      printf("%s (myresult=%s)\n", okStr2, myResult2);
      break;
    }
    else if (strcmp(buf, errorStr) == 0)
    { //收到了错误的消息
      char *errorStr2 = strtok(errorStr, "\n");
      char *myResult2 = strtok(myResult, "\n");
      printf("%s (myresult=%s)\n", errorStr2, myResult2);
      break;
    }
    else if (numbytes > 4)
    { //收到了assignment
      //sleep(6);//用于验证超时
      printf("ASSIGNMENT: %s", buf);
      char *operatorSign = strtok(buf, " ");
      char *numberStr1 = strtok(NULL, " ");
      char *numberStr2 = strtok(NULL, "\n");
      //整数情况
      if (strcmp(operatorSign, add) == 0)
      {//加法
        int result;
        int number1 = atoi(numberStr1);
        int number2 = atoi(numberStr2);
        result = number1 + number2;
#ifdef DEBUG
        printf("Calculated the result to %d\n", result);
#endif
        snprintf(myResult, sizeof(myResult), "%d\n", result);
      }
      else if (strcmp(operatorSign, div) == 0)
      {//除法
        int result;
        int number1 = atoi(numberStr1);
        int number2 = atoi(numberStr2);
        result = number1 / number2;
#ifdef DEBUG
        printf("Calculated the result to %d\n", result);
#endif
        snprintf(myResult, sizeof(myResult), "%d\n", result);
      }
      else if (strcmp(operatorSign, mul) == 0)
      {//乘法
        int result;
        int number1 = atoi(numberStr1);
        int number2 = atoi(numberStr2);
        result = number1 * number2;
#ifdef DEBUG
        printf("Calculated the result to %d\n", result);
#endif
        snprintf(myResult, sizeof(myResult), "%d\n", result);
      }
      else if (strcmp(operatorSign, sub) == 0)
      {//减法
        int result;
        int number1 = atoi(numberStr1);
        int number2 = atoi(numberStr2);
        result = number1 - number2;
#ifdef DEBUG
        printf("Calculated the result to %d\n", result);
#endif
        snprintf(myResult, sizeof(myResult), "%d\n", result);
      }
      //浮点数情况,fadd/fdiv/fmul/fsub
      else if (strcmp(operatorSign, fadd) == 0)
      {//浮点数加法
        float result;
        float number1 = atof(numberStr1);
        float number2 = atof(numberStr2);
        result = number1 + number2;
#ifdef DEBUG
        printf("Calculated the result to %f\n", result);
#endif
        snprintf(myResult, sizeof(myResult), "%8.8g\n", result);
      }
      else if (strcmp(operatorSign, fdiv) == 0)
      {//浮点数除法
        float result;
        float number1 = atof(numberStr1);
        float number2 = atof(numberStr2);
        result = number1 / number2;
#ifdef DEBUG
        printf("Calculated the result to %f\n", result);
#endif
        snprintf(myResult, sizeof(myResult), "%8.8g\n", result);
      }
      else if (strcmp(operatorSign, fmul) == 0)
      {//浮点数乘法
        float result;
        float number1 = atof(numberStr1);
        float number2 = atof(numberStr2);
        result = number1 * number2;
#ifdef DEBUG
        printf("Calculated the result to %f\n", result);
#endif
        snprintf(myResult, sizeof(myResult), "%8.8g\n", result);
      }
      else if (strcmp(operatorSign, fsub) == 0)
      {//浮点数减法
        float result;
        float number1 = atof(numberStr1);
        float number2 = atof(numberStr2);
        result = number1 - number2;
#ifdef DEBUG
        printf("Calculated the result to %f\n", result);
#endif
        snprintf(myResult, sizeof(myResult), "%8.8g\n", result);
      }
      else
      {
        perror("wrong operator");
        exit(1);
      }
      //发送结果
      if ((numbytes = send(sockfd, myResult, strlen(myResult), 0)) == -1)
      {
        perror("send:\n");
        exit(1);
      }
    }
    else
    {
      break;
    }
  }
  //close()
  close(sockfd);
}
