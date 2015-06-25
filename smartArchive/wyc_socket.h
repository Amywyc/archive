#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#include <sys/ioctl.h>
//#include<linux/if.h>

/*
#include "common.h"
#include"wyc_socket.h"
*/

#define BACKLOG 256
#define IFCLEN  10240
#define MAXSLEEP 256



int wyc_socket_server_create(int port){
	
	struct sockaddr_in server;
	
	int listenfd;
	if(-1 == (listenfd = socket(AF_INET,SOCK_STREAM,0))){
		perror("Socket error.");
		exit(1);
	}

	int opt = SO_REUSEADDR;
	setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

	socklen_t optlen;
	optlen = sizeof(int);
	/*set SO_RCVBUF*/
	int rcvbuflen = 512000;
	if(0 != setsockopt(listenfd,SOL_SOCKET,SO_RCVBUF,&rcvbuflen,optlen)){
		printf("Setsockopt error.\n");
		exit(1);
	}
	/*set SO_SNDBUF*/
	int sndbuflen = 512000;
	if(0 != setsockopt(listenfd,SOL_SOCKET,SO_SNDBUF,&sndbuflen,optlen)){
		printf("Setsockopt error.\n");
		exit(1);
	}	

	bzero(&server,sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	if (-1 == bind(listenfd,(struct sockaddr *)&server,sizeof(struct sockaddr))){
		perror("Bind error.");
		exit(1);
	}
	if(-1 == listen(listenfd,BACKLOG)){
		perror("Listen error.");
		exit(1);
	}
	printf("server:waiting,port:%d\n",port);
	return listenfd;
}

int wyc_socket_accept(int listenfd,void *arg){
	struct sockaddr_in *client;
	client = (struct sockaddr_in *)arg;

	int confd;
	socklen_t sin_size;
	sin_size = sizeof(struct sockaddr_in);
	
	if(-1 == (confd = accept(listenfd,(struct sockaddr *)client,&sin_size))){
		perror("Accept error.");
		exit(1);
	}
	
	return confd;
}

int wyc_socket_client_create(){

	int fd;
	if(-1 == (fd=socket(AF_INET,SOCK_STREAM,0))){
		perror("Socket error.");
		exit(1);
	}
	return fd;
}

void wyc_socket_connect(char *ip,int fd,int port){
	struct hostent *he;
	struct sockaddr_in server;

//	printf("client:connetct start!port:%d\n",port);
//	printf("connect to:%s(%d)\n",ip,(int)strlen(ip));
	
	if(NULL == (he=gethostbyname(ip))){
		printf("Gethostbyname error.\n");
		exit(1);
	}
	
	bzero(&server,sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr = *((struct in_addr *)he->h_addr);

	socklen_t optlen;
	optlen = sizeof(int);
	/*set SO_RCVBUF*/
	int rcvbuflen = 512000;
	if(0 != setsockopt(fd,SOL_SOCKET,SO_RCVBUF,&rcvbuflen,optlen)){
		printf("Setsockopt error.\n");
		exit(1);
	}
	/*set SO_SNDBUF*/
	int sndbuflen = 512000;
	if(0 != setsockopt(fd,SOL_SOCKET,SO_SNDBUF,&sndbuflen,optlen)){
		printf("Setsockopt error.\n");
		exit(1);
	}	

	int nsec,connflag=-1;
	for(nsec=1;nsec <= MAXSLEEP;nsec <<= 1){
		if(0 == (connflag=connect(fd,(struct sockaddr *)&server,sizeof(struct sockaddr)))){
			goto jcon;
		}
		//perror("connect mid error");
		if(nsec <= MAXSLEEP/2)
			sleep(nsec);
	}
jcon:	
	if(0 != connflag){
		printf("Connect error(%s).\n",ip);
		perror("connect error");
		pthread_exit(NULL);
	}
}


void wyc_socket_close(int fd){
	close(fd);
}

void get_localip(){
	
		int i,num, sockfd;
		struct ifconf conf;
		struct ifreq * ifr;
		char buf[IFCLEN], *ip;
	
		sockfd = socket(AF_INET, SOCK_DGRAM, 0);
		conf.ifc_len = IFCLEN;
		conf.ifc_buf = buf;
	
		ioctl(sockfd, SIOCGIFCONF, &conf);
	
		num = conf.ifc_len /sizeof(struct ifreq);
		ifr = conf.ifc_req;
		
		for(i = 0; i < num; i++){
			struct sockaddr_in *sin = (struct sockaddr_in *)(&ifr->ifr_addr);
			ioctl(sockfd, SIOCGIFFLAGS, ifr);
	
			if(((ifr->ifr_flags & IFF_LOOPBACK) == 0) &&(ifr->ifr_flags & IFF_UP)){
				ip = inet_ntoa(sin->sin_addr);
				printf("Local IP:%s\n",ip);
/*
				for(j = 0; j < svcnum; j++){
					if(strcmp(ip, ipp[j]) == 0){//查找相应的IP地址
						seq = j;
						goto OUT;
					}
				}
*/
			}
			ifr++;
		}
	
//	OUT:
/*
		if(seq == -1){
			printf("Your ip is not in server list, if you wanna join it, please reverse server.conf\n");
			return -1;
		}
		else{
			printf("Server: %d\n", seq);
			return seq;
		}
*/
	
}
/*
send:
int shouldsend,realsend;
shouldsend = size;
realsend = 0;
printf("Shouldsend:%d\n",DATABUFSIZ);
while(realsend<shouldsend){
	if(-1 == (nbyte=send(sockfd,buf+realsend,shouldsend-realsend,0))){
		perror("Send error");
		exit(1);
	}
	realsend += nbyte;
}
printf("Realsend:%d\n",realsend);

receive:
int shouldrecv,realrecv;
shouldrecv = size;
realrecv = 0;
printf("Shouldrecv:%d\n",shouldrecv);
while(realrecv < shouldrecv){
	if(-1 == (nbyte=recv(sockfd,buf+realrecv,shouldrecv-realrecv,0))){
		perror("Recv error");
		exit(1);
	}else if(0 == nbyte){
		printf("Recv end.\n");
	}
	rearecv+=nbytes;
}
printf("Realrecv:%d\n",realrecv);
*/

/*send check data*/

