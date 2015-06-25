#ifndef WYC_DATANODE_H

#define WYC_DATANODE_H
#include"datanode.h"
#include"wyc_socket.h"


#endif

#define IPREFSIZE		(sizeof(struct ifreq))
#define IFCONFBUFSIZE	(IPREFSIZE*3)

//diskfd
int		diskfd[NODE_MAX];
int		localfd;

char 	IP[NODE_MAX][IPLENGYH];
int 	ipSequence;
int 	ipCount;

//malloc && free
void init_wyc();
void free_wyc();

//IP
void getIP();
void getIpSequence();
void sendcoding_strip_str(coding_strip_str* arg);
void recvcoding_strip_str(int socket,coding_strip_str* arg);

//print
void printCodingStripStruct(coding_strip_str* ptr);
void printCodingresult(coding_result* res);


//socket
//int 				smartSocket;    	//smartnode
int					listenSocket;
//int					datanodeSocket[NODE_MAX];
int					usefulFlag[NODE_MAX];
int					actionSocket[CLIENT_MAX];
int					actionSocketCount;




struct sockaddr_in 	client;
//void* doAsServer();
static int doAsClient(int server_seq,int port);


//pthread
pthread_attr_t		detachAttr;
pthread_t 			datanodeListenThread[NODE_MAX];
//pthread_t			codeSubThread;
//pthread_t			codeMainThread;

//link to smartnode
pthread_t		encodingPthread;
pthread_t		recvSmartnodePthread;

//for test
FILE*	diskRateFile;
FILE* 	stripTimeResultFile;

void* datanodeListener();
void* smartnodeListener();

void* smartnodeRecvFrom(void* arg);
void smartnodeSendTo(int socket,coding_result* arg);


void* codeMain(void* arg);
void* codeSub(void* arg);

void codeSubFunction();


void XOR_wyc(char* dest,char* src1,char* src2,int size);



int main(){

	int i;
	pthread_t			dataListenerPthread;
	pthread_t			smartListenerPthread;

//	int iarray[NODE_MAX];
//	for(i=0;i<NODE_MAX;i++)
//		iarray[i]=i;
	
	init_wyc();
	
	//open smart socket
//	if(0!=(pthread_create(&serverSocketPthread,NULL,doAsServer,&(PORT)))){
//		perror("error:pthread_create doAsServer");
//		exit(1);
//	}

//	sleep(2);
//	printf("actionSocketCount:%d,actionSocket[0]:%d\n",
//		actionSocketCount,actionSocket[0]);

	//listen to smartnode 
	if(0!=(pthread_create(&smartListenerPthread,NULL,smartnodeListener,NULL))){
		perror("error:pthread_create smartnodeRecvFrom");
		exit(1);
	}
	//listen to datanode
	if(0!=(pthread_create(&dataListenerPthread,NULL,datanodeListener,NULL))){
		perror("error:pthread_create smartnodeRecvFrom");
		exit(1);
	}

	pthread_join(smartListenerPthread,NULL);
	pthread_join(dataListenerPthread,NULL);
	
	free_wyc();
	return 0;
}

static void get_srvdisk(){
	
		FILE* ffd;		
		int nbytes;
		int ndisk=0;
		
		if(NULL == (ffd = fopen(DISKFILE,"r"))){
			perror("fopen error");
			exit(1);
		}

		while(EOF != (nbytes = fscanf(ffd,"%d",(diskfd+ndisk)))){
//			printf("%d\n",srvdisk[ndisk]);
			ndisk++ ;
			if(ndisk > NODE_MAX){
				printf("Wrong disk number.\n");
				exit(1);
			}
		}
	
		printf("The number of disks:%d\n",ndisk);

		if(fclose(ffd)!=0){
			perror("error:close disk.config");
			exit(1);
		}
}

static void get_diskfd(){
	char *localdisk;
	switch (diskfd[ipSequence]){
	case 1:
		localdisk = "/dev/sda";
		break;
	case 2:
		localdisk = "/dev/sdb";
		break;
	default:
		printf("Disk config error\n");
		exit(1);
	}
	localfd = open(localdisk, O_RDWR|O_DIRECT);//
	if(localfd < 0){
		printf("Can't open disk:%s\n", localdisk);
		exit(1);
	}
	printf("Open %s ready!\n",localdisk);
}


void init_wyc(){
	actionSocketCount=0;
//	dataTranslateSocket=0;
//	smartSocket=0;
	if(0!=pthread_attr_init(&detachAttr)){
		perror("error:pthread_attr_init detachAttr");
		exit(1);
	}
	if((pthread_attr_setdetachstate(&detachAttr,PTHREAD_CREATE_DETACHED))!=0){
		perror("error:pthread_attr_setdetachstate detachAttr");
		exit(1);
	}

	if(NULL==(diskRateFile=fopen(DISKRATEFILE,"a"))){
		perror("error:fopen DISKRATEFILE");
		exit(1);
	}

	if(NULL==(stripTimeResultFile=fopen(STRIPRESULTFILE,"wb"))){
		perror("error:fopen SOCKETRATEFILE");
		exit(1);
	}

	getIP();
	getIpSequence();
	
	get_srvdisk();
	get_diskfd();

}

void free_wyc(){

	if(0!=pthread_attr_destroy(&detachAttr)){
		perror("error:pthread_attr_destroy detachAttr");
		exit(1);
	}

	if(close(localfd)!=0){
		perror("error:close localfd");
		exit(1);
	}

	if(fclose(diskRateFile)!=0){
		perror("error fclose diskRateFile");
		exit(1);
	}

	if(fclose(stripTimeResultFile)!=0){
		perror("error fclse stripTimeResultFile");
		exit(1);
	}
}

void* datanodeListener(){

	int datanodelistenSocket;
//	int dataTranslateSocket=0;
	struct sockaddr_in 	client;
	pthread_t	codeSubPthread;
	int 		dataTranslateSocket;
	int* 		socketPtr;

	datanodelistenSocket = wyc_socket_server_create(DATAPORT+ipSequence);
	
	while(1){
		
		dataTranslateSocket = wyc_socket_accept(datanodelistenSocket,&client);
//		printf("client:%s(%d)\n",inet_ntoa(client.sin_addr),dataTranslateSocket);	
//		printf("%p,%p,%p\n",(int*)&codeSubPthread,(int*)&detachAttr,(int*)&dataTranslateSocket);
		if((socketPtr=(int*)malloc(sizeof(int)))==NULL){
			perror("error:malloc");
			exit(1);
		}
		printf("ipSequence:%d dataSocketID:%d\n",ipSequence,dataTranslateSocket);
		*socketPtr=dataTranslateSocket;
		if((pthread_create(&codeSubPthread,&detachAttr,codeSub,(void*)socketPtr))!=0){
			perror("error:pthread_create ");
			exit(1);
		}
		socketPtr=NULL;
		printf("After pthread_create %d\n",dataTranslateSocket);
	}
		
	pthread_exit(NULL);
	
	return ((void*)0);

}

void* codeSubRead(void* arg){
	
	erase_code_sub* codeSubPtr=(erase_code_sub*)arg;

	char* buffer=NULL;
	if((buffer=(char*)malloc(CHUNK_SIZE))==NULL){
		perror("error:malloc Chunk buffer");
		exit(1);
	}

	int lseekRam=rand();
	lseekRam = lseekRam - lseekRam%getpagesize();
	if((lseek(localfd,(off_t)lseekRam,SEEK_SET))==-1){
		perror("error: lseek");
		exit(1);
	}

	while((codeSubPtr->readNum)<(codeSubPtr->toalNum)){
//		memcpy(buffer,codeSubPtr->dataBuffer[codeSubPtr->readNum],CHUNK_SIZE);

		if((read(localfd,codeSubPtr->dataBuffer[codeSubPtr->readNum],CHUNK_SIZE))
			!=CHUNK_SIZE){
			perror("error1:read disk");
			printf("%d,%p\n",localfd,codeSubPtr->dataBuffer[codeSubPtr->readNum]);
			exit(1);
		}
		
		(codeSubPtr->readNum)++;
	//	printf("read:%d(%d)\n",codeSubPtr->readNum,codeSubPtr->toalNum);
	}

	free(buffer);
	
	return ((void*)0);
}

void* codeSubSend(void* arg){

	erase_code_sub* codeSubPtr=(erase_code_sub*)arg;
	int shouldByte=CHUNK_SIZE,nowBytes,nBytes;
	while((codeSubPtr->sendNum)<(codeSubPtr->toalNum)){
		
		if((codeSubPtr->sendNum)<(codeSubPtr->readNum)){
			
//			printf("send:%d(%d)(socket:%d)\n",codeSubPtr->sendNum,
//							codeSubPtr->toalNum,codeSubPtr->socket);
			nowBytes=0;
			while(nowBytes<shouldByte){
				if((nBytes=write(codeSubPtr->socket,
					codeSubPtr->dataBuffer[codeSubPtr->sendNum]+nowBytes,
					shouldByte-nowBytes))==-1){
					perror("error:write datanode ChunkData");
					exit(1);
				}
				nowBytes += nBytes;
			}
			
			(codeSubPtr->sendNum)++;
			
		}else{
			sleep(1);
		}
	}

	return ((void*)0);
}

void* codeSub(void* arg){
//without memory cache
	int i;

	data_request* codeRequestPtr=NULL;

	int socket=*(int*)arg;
	*(int*)arg=0;

	char* chunk_buf;
	int shouldByte=CHUNK_SIZE,nowBytes,nBytes;

	if((codeRequestPtr=(data_request*)malloc(sizeof(data_request)))==NULL){
		perror("error:malloc data_request");
		exit(1);
	}
//	if((chunk_buf=(char*)malloc(CHUNK_SIZE))==NULL){
//		perror("error:malloc chunk_buf");
//		exit(1);
//	}
	if(0!=posix_memalign((void **)&chunk_buf,getpagesize(),CHUNK_SIZE)){
		perror("error posix_memalign CHUNKSIZE");
		exit(1);
	}

	if((read(socket,codeRequestPtr,sizeof(data_request)))!=sizeof(data_request)){
		perror("error:read data_request");
		exit(1);
	}
	printf("data_request:%d,%d\n",codeRequestPtr->blockID,codeRequestPtr->codingNode);

	int lseekRam=rand();
	lseekRam = lseekRam - lseekRam%getpagesize();
	if((lseek(localfd,(off_t)lseekRam,SEEK_SET))==-1){
		perror("error: lseek");
		exit(1);
	}
	
	for(i=0;i<CHUNK_NUM;i++){
		if((read(localfd,chunk_buf,CHUNK_SIZE))!=CHUNK_SIZE){
			perror("error2:read disk");
			exit(1);
		}
		nowBytes=0;
		while(nowBytes<shouldByte){
			if((nBytes=write(socket,chunk_buf+nowBytes,shouldByte-nowBytes))==-1){
				perror("error:write datanode ChunkData");
				exit(1);
			}
			nowBytes += nBytes;
		}
	}

	printf("done!\n");
	
	free(codeRequestPtr);
	free(chunk_buf);


	close(socket);
	free(arg);

	return ((void*)0);
}

void* codeSub2(void* arg){
//with memory cache

//	printf("first line in codeSub\n");
	int i;

	data_request* codeRequestPtr=NULL;
	pthread_t	readPthread,sendPthread;

	erase_code_sub* codeSubPtr;

	int socket=*(int*)arg;
	*(int*)arg=0;

//	printf("In codeSub(socket:%d)\n",socket);

	if((codeSubPtr=(erase_code_sub*)malloc(sizeof(erase_code_sub)))==NULL){
		perror("error:malloc codeSubPtr");
		exit(1);
	}
	for(i=0;i<CHUNK_NUM;i++)
		if(0!=posix_memalign((void**)&(codeSubPtr->dataBuffer[i]),getpagesize(),CHUNK_SIZE)){
			perror("error posix_memalign CHUNKSIZE");
			exit(1);
		}

	if((codeRequestPtr=(data_request*)malloc(sizeof(data_request)))==NULL){
		perror("error:malloc data_request");
		exit(1);
	}
//	printf("@@@@@\n");
	if((read(socket,codeRequestPtr,sizeof(data_request)))!=sizeof(data_request)){
		perror("error:read data_request");
		exit(1);
	}
	printf("data_request:%d,%d\n",codeRequestPtr->blockID,codeRequestPtr->codingNode);

	codeSubPtr->blockID=codeRequestPtr->blockID;
	codeSubPtr->codingNode=codeRequestPtr->codingNode;
	codeSubPtr->readNum=0;
	codeSubPtr->sendNum=0;
	codeSubPtr->toalNum=CHUNK_NUM;
	codeSubPtr->socket=socket;

	if((pthread_create(&readPthread,NULL,codeSubRead,codeSubPtr))!=0){
		perror("error:pthread_create codeSubRead");
		exit(1);
	}

	if((pthread_create(&sendPthread,NULL,codeSubSend,codeSubPtr))!=0){
		perror("error:pthread_create codeSubSend");
		exit(1);
	}

	pthread_join(readPthread,NULL);
	pthread_join(sendPthread,NULL);

	printf("done!\n");

	for(i=0;i<CHUNK_NUM;i++)
		free(codeSubPtr->dataBuffer[i]);
	
	free(codeRequestPtr);
	free(codeSubPtr);

	close(socket);

	return ((void*)0);
}

//void* datanodeListen(void* arg){

//	pthread_t	codeSubPthread;

//	int datanode = *(int*)arg;
//	erase_code_sub* codesubPtr=NULL;
//	data_request* requestPtr=NULL;

//	if((requestPtr=(data_request*)malloc(sizeof(data_request)))==NULL){
//		printf("error:malloc data_request\n");
//		exit(1);
//	}

//	while(1){
//		
//		if((codesubPtr=(erase_code_sub*)malloc(sizeof(erase_code_sub)))==NULL){
//			perror("error:malloc erase_code_sub");
//			exit(1);
//		}

//		if((read(datanodeSocket[datanode],requestPtr,sizeof(data_request)))!=sizeof(data_request)){
//			perror("error:read data_request");
//			exit(1);
//		}

//		codesubPtr->blockID=requestPtr->blockID;
//		codesubPtr->codingNode=requestPtr->codingNode;
//		codesubPtr->readNum=0;
//		codesubPtr->sendNum=0;
//		codesubPtr->toalNum=CHUNK_NUM;

//		if((pthread_create(&codeSubPthread,&detachAttr,codeSub,codesubPtr))!=0){
//			perror("error:pthread_create codeSub");
//			exit(1);
//		}

//		codesubPtr=NULL;
//	}
//	
//}

void XOR_wyc(char* dest,char* src1,char* src2,int size){
	int i;
	
	long *ptr=(long*)dest;
	long *ptr1=(long*)src1;
	long *ptr2=(long*)src2;

	for(i=0;i<size;i += sizeof(long)){
		*ptr=(*ptr1)^(*ptr2);
		ptr++;
		ptr1++;
		ptr2++;
	}

	for(i=i-8;i<size;i++){
		*(dest+i)=(*(src1+i))^(*(src2+i));
	}
}

void* codeMain(void* arg){
	int i,j;
//	char*	buffer=NULL;
	int	codeMainSocket[K_MAX];
	struct timeval start,end;
	struct timeval codeStart,codeEnd;
	double	readTimeTotal=0,socketTimetotal=0,writeTimetotal=0;
	double 	timecost;

	coding_strip_str* encodeArg=(coding_strip_str*)arg;
	char* parityBuffer;

	erase_code_main codeMainStr;

	gettimeofday(&codeStart,NULL);
	
	for(i=0;i<K_MAX;i++)
		if(0!=posix_memalign((void**)&(codeMainStr.dataBuffer[i]),getpagesize(),CHUNK_SIZE)){
			perror("error posix_memalign CHUNKSIZE");
			exit(1);
		}
	//buffer all the parity block
	if(0!=posix_memalign((void**)&(parityBuffer),getpagesize(),encodeArg->parity_blocks_num*BLOCK_SIZE)){
		perror("error posix_memalign CHUNKSIZE");
		exit(1);
	}
//	for(i=0;i<R_MAX;i++)
//		if(0!=posix_memalign((void**)&(codeMainStr.checkBuffer[i]),getpagesize(),CHUNK_SIZE)){
//			perror("error posix_memalign CHUNKSIZE");
//			exit(1);
//		}
	
	data_request dataRequest;
	

//	if((buffer=(char*)malloc(CHUNK_SIZE))==NULL){
//		perror("error:malloc buffer");
//		exit(1);
//	}

//	printf("****111111\n");
	
	for(i=0;i<encodeArg->data_blocks_num;i++){
		if(encodeArg->data_node_arr[i]==ipSequence){
			codeMainSocket[i]=0;
		}else{
			codeMainSocket[i]=doAsClient(encodeArg->data_node_arr[i],DATAPORT);
		}
	}
	for(i=0;i<encodeArg->data_blocks_num;i++){
		printf("%d socket:%d\n",i,codeMainSocket[i]);
	}
	
//	printf("****222222\n");

	dataRequest.codingNode=ipSequence;
	for(i=0;i<encodeArg->data_blocks_num;i++)
		if(encodeArg->data_node_arr[i]!=ipSequence){
			dataRequest.blockID=encodeArg->first_blockID+i;
			if((write(codeMainSocket[i],&dataRequest,sizeof(data_request)))!=sizeof(data_request)){
				perror("error:write dataRequest");
				exit(1);
			}
			printf("%d,data_request:%d,%d\n",i,dataRequest.blockID,dataRequest.codingNode);
		}

//	printf("****333333\n");

	codeMainStr.totoalNum=CHUNK_NUM;
	codeMainStr.curNum=0;
	
	int shouldBytes=CHUNK_SIZE,nowBytes,nBytes;
	int lseekRam=rand();
	lseekRam = lseekRam - lseekRam%getpagesize();
	if((lseek(localfd,(off_t)lseekRam,SEEK_SET))==-1){
		perror("error: lseek");
		exit(1);
	}
	
	while((codeMainStr.curNum)<(codeMainStr.totoalNum)){
		
		//data from
		for(i=0;i<encodeArg->data_blocks_num;i++){
//			printf("data from:%d(socket:%d)\n",encodeArg->data_node_arr[i],codeMainSocket[i]);
			if(encodeArg->data_node_arr[i]==ipSequence){
			//local
//				memcpy(codeMainStr.dataBuffer[i],buffer,CHUNK_SIZE);
				lseekRam=rand();
				lseekRam = lseekRam - lseekRam%getpagesize();
				gettimeofday(&start,NULL);
				if((lseek(localfd,(off_t)lseekRam,SEEK_SET))==-1){
					perror("error: lseek");
					exit(1);
				}

				if((read(localfd,codeMainStr.dataBuffer[i],CHUNK_SIZE))
					!=CHUNK_SIZE){
					perror("error3:read disk chunk");
					exit(1);
				}
				gettimeofday(&end,NULL);
				timecost = ((end.tv_sec-start.tv_sec)+(end.tv_usec-start.tv_usec)*0.000001);
				readTimeTotal += timecost;
				if(0>fprintf(diskRateFile,"disk read,stripID(%d) rate(%lfMBPS)\n",encodeArg->stripID,
					((double)CHUNK_SIZE/1024/1024)/timecost)){
					perror("error:fprintf diskRateFile");
					exit(1);
				}
			}else{
			//socket
				nowBytes=0;
//				if(0>fprintf(stripTimeResultFile,"nowBytes:%d,shouldBytes:%d\n",nowBytes,shouldBytes)){
//					perror("error:fprintf diskRateFile");
//					exit(1);
//				}
				gettimeofday(&start,NULL);
				while(nowBytes<shouldBytes){
					if((nBytes=read(codeMainSocket[i],codeMainStr.dataBuffer[i]+nowBytes,
						shouldBytes-nowBytes))==-1){
						perror("error:read datanode socket data");
						exit(1);
					}
					nowBytes += nBytes;
//					if(0>fprintf(stripTimeResultFile,"nowBytes:%d,shouldBytes:%d,nBytes:%d\n",nowBytes,shouldBytes,nBytes)){
//						perror("error:fprintf diskRateFile");
//						exit(1);
//					}
				}
				gettimeofday(&end,NULL);
				timecost = ((end.tv_sec-start.tv_sec)+(end.tv_usec-start.tv_usec)*0.000001);
				socketTimetotal += timecost;
//				if(0>fprintf(stripTimeResultFile,"socket:%d,start:%ld:%ld    end:%ld:%ld\n",codeMainSocket[i],
//					(long)start.tv_sec,(long)start.tv_usec,
//					(long)end.tv_sec,(long)end.tv_usec)){
//					perror("error:fprintf diskRateFile");
//					exit(1);
//				}
//				if(0>fprintf(stripTimeResultFile,"socket read,stripID(%d) rate(%lfMBPS:%lf,%lf)\n",encodeArg->stripID,
//					((double)CHUNK_SIZE/1024/1024)/((end.tv_sec-start.tv_sec)+(end.tv_usec-start.tv_usec)*0.000001),
//					((double)CHUNK_SIZE/1024/1024),((end.tv_sec-start.tv_sec)+(end.tv_usec-start.tv_usec)*0.000001))){
//					perror("error:fprintf diskRateFile");
//					exit(1);
//				}
			}
		}
		//data deal
//		for(i=0;i<encodeArg->parity_blocks_num;i++)
//			XOR_wyc(codeMainStr.checkBuffer[i],codeMainStr.dataBuffer[0],
//			codeMainStr.dataBuffer[1],CHUNK_SIZE);
		//data to
		for(i=0;i<encodeArg->parity_blocks_num;i++){
			memcpy(parityBuffer+i*BLOCK_SIZE+codeMainStr.curNum*CHUNK_SIZE,
				codeMainStr.dataBuffer[0],CHUNK_SIZE);
//			lseekRam=rand();
//			lseekRam = lseekRam - lseekRam%getpagesize();
			if(0>fprintf(diskRateFile,"disk write,stripID(%d) rate(%lfMBPS)\n",encodeArg->stripID,
				((double)CHUNK_SIZE/1024/1024)/((end.tv_sec-start.tv_sec)+(end.tv_usec-start.tv_usec)*0.000001))){
				perror("error:fprintf diskRateFile");
				exit(1);
			}
		}
		(codeMainStr.curNum)++;
//		printf("codeMain:%d(%d)\n",codeMainStr.curNum,codeMainStr.totoalNum);
	}

		gettimeofday(&start,NULL);
		lseekRam=rand();
		lseekRam = lseekRam - lseekRam%getpagesize();
		if((lseek(localfd,(off_t)lseekRam,SEEK_SET))==-1){
			perror("error: lseek");
			exit(1);
		}
		for(i=0;i<(encodeArg->parity_blocks_num*CHUNK_NUM);i++)
			if((write(localfd,parityBuffer+i*CHUNK_SIZE,CHUNK_SIZE))
				!=CHUNK_SIZE){
				perror("error:write disk chunk");
				exit(1);
			}
		gettimeofday(&end,NULL);
		writeTimetotal = ((end.tv_sec-start.tv_sec)+(end.tv_usec-start.tv_usec)*0.000001);

	for(i=0;i<encodeArg->data_blocks_num;i++){
		if(encodeArg->data_node_arr[i]!=ipSequence){
			close(codeMainSocket[i]);
		}
	}

	for(i=0;i<K_MAX;i++)
		free(codeMainStr.dataBuffer[i]);
//	for(i=0;i<R_MAX;i++)
//		free(codeMainStr.checkBuffer[i]);
	
	free(parityBuffer);

	gettimeofday(&codeEnd,NULL);
	printf("codeTime:stripID(%d) totaltime(%lf)[read(%lf),socket(%lf),write(%lf)]\n",
		encodeArg->stripID,(codeEnd.tv_sec-codeStart.tv_sec)+(codeEnd.tv_usec-codeStart.tv_usec)*0.000001,
		readTimeTotal,socketTimetotal,writeTimetotal);
	if(0>fprintf(stripTimeResultFile,"%d,%d,%lf,%lf,%lf,%lf\n",
		encodeArg->stripID,ipSequence,(codeEnd.tv_sec-codeStart.tv_sec)+(codeEnd.tv_usec-codeStart.tv_usec)*0.000001,
		readTimeTotal,socketTimetotal,writeTimetotal)){
		perror("error:fprintf diskRateFile");
		exit(1);
	}
	if(0!=fflush(stripTimeResultFile)){
		perror("error:fflush");
		exit(1);
	}

	return ((void*)0);
}

void smartnodeSendTo(int socket,coding_result* arg){

	printf("datanode: send to smartnode(socket:%d)\n",socket);
	if(write(socket,arg,sizeof(coding_result))!=sizeof(coding_result)){
		perror("error:write coding_result");
		exit(1);
	}
	
	printCodingresult(arg);
}

void recvcoding_strip_str(int socket,coding_strip_str* arg){
	printf("readBefore\n");
	if(read(socket,arg,sizeof(coding_strip_str))!=sizeof(coding_strip_str)){
		perror("error:read coding_strip_str");
		exit(1);
	}
	printf("readAfter\n");
	printCodingStripStruct(arg);
}

void* smartnodeRecvFrom(void* arg){

	int socket=*(int*)arg;
	*(int*)arg=0;
	
	pthread_t	encodePthread;
	coding_strip_str* codingStripStr=NULL;
	coding_result encodeResult;

//	socket=smartnodeDataSocket;
	
//	printf("smartnodeRecvFrom start!socket(%d)\n",socket);
	
	if((codingStripStr=(coding_strip_str*)malloc(sizeof(coding_strip_str)))==NULL){
		printf("error:malloc coding_strip_str datanode\n");
		exit(1);
	}
	printf("datanode waiting:encode strip\n");
	recvcoding_strip_str(socket,codingStripStr);
	if((pthread_create(&encodePthread,NULL,codeMain,codingStripStr))!=0){
		perror("error:pthread_create encoding");
		exit(1);
	}

	pthread_join(encodePthread,NULL);

	encodeResult.stripID=codingStripStr->stripID;
	encodeResult.done=1;

	smartnodeSendTo(socket,&encodeResult);
	free(codingStripStr);
	close(socket);
	free(arg);

	return ((void*)0);
}

void getIP(){
	FILE* ipfl;
	int i;
	ipCount=0;
	if(NULL==(ipfl=fopen(IPFILE,"r"))){
		perror("error:fopen IPFILE");
		exit(1);
	}
	while(NULL!=(fgets(IP[ipCount],IPLENGYH,ipfl))){
		IP[ipCount][strlen(IP[ipCount])-1]='\0';
		ipCount++;
		if(ipCount>=NODE_MAX)
			break;
	}
//	for(i=0;i<ipCount;i++)
//		printf("%d ip(%d):%s\n",i,(int)strlen(IP[i]),IP[i]);
//	printf("\n");
	if(fclose(ipfl)!=0){
		perror("error:fclose IPFILE");
		exit(1);
	}
}

void getIpSequence(){
	int i,j,num, sockfd;
	struct ifconf conf;
	struct ifreq * ifr;
	char buf[IFCONFBUFSIZE], *ip;

	ipSequence=-1;
	
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	conf.ifc_len = IFCONFBUFSIZE;
	conf.ifc_buf = buf;
	
	ioctl(sockfd, SIOCGIFCONF, &conf);
	
	num = conf.ifc_len /sizeof(struct ifreq);
	ifr = conf.ifc_req;
		
	for(i = 0; i < num; i++){
		struct sockaddr_in *sin = (struct sockaddr_in *)(&ifr->ifr_addr);
		ioctl(sockfd, SIOCGIFFLAGS, ifr);
	
		if(((ifr->ifr_flags & IFF_LOOPBACK) == 0) &&(ifr->ifr_flags & IFF_UP)){
			ip = inet_ntoa(sin->sin_addr);
			for(j=0;j<ipCount;j++){
				if(0 == strcmp(ip,IP[j])){
				ipSequence = j;
				goto out;
				}
			}	
		}
		ifr++;
	}
out:	
	if(-1 == ipSequence){
		printf("You server Ip is not in ip.config!\n");
		exit(1);
	}
	printf("%d: %s.\n",ipSequence,ip);
}

void* smartnodeListener(){

	int listenSocket;
	int* socketPos;
	int smartnodeDataSocket=0;
	struct sockaddr_in 	client;
	pthread_t	codePthread;
	
	int port=PORT;

	//open smartnode listen socket PORT+sequence
	listenSocket = wyc_socket_server_create(port+ipSequence);
	printf("Datanode's listenSocket:%d\n",listenSocket);
	
	while(1){
		smartnodeDataSocket = wyc_socket_accept(listenSocket,&client);
		printf("ipSequence:%d smartSocketID:%d\n",ipSequence,smartnodeDataSocket);
		if((socketPos=(int*)malloc(sizeof(int)))==NULL){
			perror("error:malloc");
			exit(1);
		}
		*socketPos=smartnodeDataSocket;
		if((pthread_create(&codePthread,&detachAttr,smartnodeRecvFrom,(void*)socketPos))!=0){
			perror("error:pthread_create smartnodeRecvFrom");
			exit(1);
		}
		socketPos=NULL;
	}
	
	pthread_exit(NULL);

	return ((void*)0);
}

static int doAsClient(int server_seq,int port){
	
	int client_socket=-1;

	client_socket= wyc_socket_client_create();

	printf("Do_as_client(IP):%s\n",IP[server_seq]);
	wyc_socket_connect(IP[server_seq],client_socket,port+server_seq);
	printf("Do_as_client:(socket):%d\n",client_socket);

	return client_socket;

}

void printCodingStripStruct(coding_strip_str* ptr){
	int i;
	printf("coding_strip_str:\n");
	printf("stripID:%d,first_blockID:%d,data_blocks_num:%d,parity_blocks_num:%d,coding_node:%d\n",
		ptr->stripID,ptr->first_blockID,ptr->data_blocks_num,ptr->parity_blocks_num,ptr->coding_node);
	printf("data node array:");
	for(i=0;i<ptr->data_blocks_num;i++)
		printf("%d ",ptr->data_node_arr[i]);
	printf("\n");
}

void printCodingresult(coding_result* res){
	printf("coding_result:");
	printf("stripID:%d,done:%d\n",res->stripID,res->done);
}
