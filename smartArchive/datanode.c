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

pbuf_manager* pbuf_manager_ptr;
//p_buf
void init_pbuf_manager(){
	int i;
	if(NULL==(pbuf_manager_ptr=(pbuf_manager*)malloc(sizeof(pbuf_manager)))){
		perror("error malloc in init_pbuf_manager");
		exit(1);
	}
	pbuf_manager_ptr->total = (((double)TOTAL_MEM*1024)/(MEM_BUF_SIZE/1024));
	pbuf_manager_ptr->used = 0;
	if(NULL==(pbuf_manager_ptr->pbuf_usage=
		(int*)malloc((pbuf_manager_ptr->total)*(sizeof(int))))){
		perror("error malloc in init_pbuf_manager");
		exit(1);
	}
	for(i=0;i<pbuf_manager_ptr->total;i++)
		pbuf_manager_ptr->pbuf_usage[i]=0;
	if(NULL==(pbuf_manager_ptr->pbuf_address=
		(p_buf**)malloc((pbuf_manager_ptr->total)*sizeof(p_buf*)))){
		perror("error malloc in init_pbuf_manager");
		exit(1);
	}
	for(i=0;i<pbuf_manager_ptr->total;i++){
		if(NULL==(pbuf_manager_ptr->pbuf_address[i]=
			(p_buf*)malloc(sizeof(p_buf)))){
			perror("error malloc in init_pbuf_manager");
			exit(1);
		}
		(pbuf_manager_ptr->pbuf_address[i])->cur_size = 0;
		(pbuf_manager_ptr->pbuf_address[i])->max_size = MEM_BUF_SIZE;//??????
		(pbuf_manager_ptr->pbuf_address[i])->count = 0;
		(pbuf_manager_ptr->pbuf_address[i])->cur_offset = 0;
	
		INIT_LIST_HEAD(&((pbuf_manager_ptr->pbuf_address[i])->head));
	
		if(0 != pthread_mutex_init(&((pbuf_manager_ptr->pbuf_address[i])->lock),NULL)){
			perror("Pthread_mutex_init error");
			exit(1);
		}

		if(0 != pthread_mutex_init(&((pbuf_manager_ptr->pbuf_address[i])->head_lock),NULL)){
			perror("Pthread_mutex_init error");
			exit(1);
		}

		if(NULL == ((pbuf_manager_ptr->pbuf_address[i])->buf = 
			(char *)malloc(sizeof(char)*((pbuf_manager_ptr->pbuf_address[i])->max_size)))){
			perror("Malloc error");
			exit(1);
		}
	}
	pthread_mutex_init(&(pbuf_manager_ptr->managerMutex),NULL);
	
}

void destory_pbuf_manager(){
	int i;
	for(i=0;i<pbuf_manager_ptr->total;i++){
			
		pthread_mutex_destroy(&((pbuf_manager_ptr->pbuf_address[i])->lock));
		
		pthread_mutex_destroy(&((pbuf_manager_ptr->pbuf_address[i])->head_lock));
		
		free((pbuf_manager_ptr->pbuf_address[i])->buf);
		
		free(pbuf_manager_ptr->pbuf_address[i]);
	
	}

	free(pbuf_manager_ptr->pbuf_address);

	free(pbuf_manager_ptr->pbuf_usage);

	free(pbuf_manager_ptr);

	pthread_mutex_destroy(&(pbuf_manager_ptr->managerMutex));
	
}

int get_pbuf(){
	int i;
	while(pbuf_manager_ptr->used>=pbuf_manager_ptr->total){
		printf("no pbuf,sleep 1 \n");
		sleep(1);
	}
	for(i=0;i<pbuf_manager_ptr->total;i++){
		if(pbuf_manager_ptr->pbuf_usage[i]==0){
			pbuf_manager_ptr->pbuf_usage[i]=1;
			return i;
		}
	}
	printf("pbuf_manager used error\n");
	exit(1);
	return -1;
}

void put_pbuf(int index){
	if((index<0)||(index>=pbuf_manager_ptr->total)){
		printf("error:wrong index\n");
		exit(1);
	}
	if(pbuf_manager_ptr->pbuf_usage[index]==0){
		printf("error:index is idle\n");
		exit(1);
	}
	pbuf_manager_ptr->pbuf_usage[index]=0;
	(pbuf_manager_ptr->used)--;
}

void updata_in_pbuf(p_buf *p_cur_buf,char *databuf,size_t size,off_t offset){

	list_head	head;
	off_t		newcuroffset;
	head.data_off = offset;

	while((p_cur_buf->cur_size + sizeof(head) + size) > p_cur_buf->max_size){
		printf("updata_in_pbuf:p_buf full,wait 0.1 seconds\n");
		usleep(100000);
	}

	newcuroffset = p_cur_buf->cur_offset + sizeof(head) + size;
	while((newcuroffset > p_cur_buf->max_size) && \
		((p_cur_buf->max_size-p_cur_buf->cur_offset) + \
		p_cur_buf->cur_size + sizeof(head) + size >p_cur_buf->max_size)){
		printf("Litter buffer fragmentary:wait 0.1 seconds\n");
		usleep(100000);
	}

	
	pthread_mutex_lock(&(p_cur_buf->lock));
	
	(p_cur_buf->count)++;
	
	if(newcuroffset > p_cur_buf->max_size){
		newcuroffset = sizeof(head) + size;
		p_cur_buf->cur_offset = 0;
	}

	head.pbuf_off = p_cur_buf->cur_offset;
	
	p_cur_buf->cur_size = p_cur_buf->cur_size + sizeof(head) + size;
	pthread_mutex_unlock(&(p_cur_buf->lock));

	memcpy((p_cur_buf->buf+p_cur_buf->cur_offset),&(head),sizeof(head));

	pthread_mutex_lock(&(p_cur_buf->head_lock));
	list_add_tail(((list_head *)(p_cur_buf->buf+p_cur_buf->cur_offset)),&(p_cur_buf->head));
	pthread_mutex_unlock(&(p_cur_buf->head_lock));

	memcpy((p_cur_buf->buf+p_cur_buf->cur_offset+sizeof(head)),databuf,size);

	pthread_mutex_lock(&(p_cur_buf->lock));
	p_cur_buf->cur_offset = newcuroffset;
	pthread_mutex_unlock(&(p_cur_buf->lock));
}

off_t updata_out_pbuf(p_buf *p_cur_buf,char *databuf){

	list_head	head;

	while(p_cur_buf->count <= 0){
		printf("Updata_out_pbuf:no data to out,wait 0.1 seconds.\n");
		usleep(100000);
	}
	
	memcpy(&head,p_cur_buf->buf+(((p_cur_buf->head).next)->pbuf_off),sizeof(head));
	memcpy(databuf,p_cur_buf->buf+(((p_cur_buf->head).next)->pbuf_off)+sizeof(head),CHUNK_SIZE);

	pthread_mutex_lock(&(p_cur_buf->head_lock));		
	list_del(p_cur_buf->head.next);
	pthread_mutex_unlock(&(p_cur_buf->head_lock));
		
	pthread_mutex_lock(&(p_cur_buf->lock));	
	p_cur_buf->cur_size = p_cur_buf->cur_size - sizeof(head) - CHUNK_SIZE;
	(p_cur_buf->count)--;
	pthread_mutex_unlock(&(p_cur_buf->lock));

	return (head.data_off);
}


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

	init_pbuf_manager();

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

	destory_pbuf_manager();
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

void codeSub_read_dire(int socket){

	int i;
	char* chunk_buf;
	int shouldByte=CHUNK_SIZE,nowBytes,nBytes;

	if(0!=posix_memalign((void **)&chunk_buf,getpagesize(),CHUNK_SIZE)){
		perror("error posix_memalign CHUNKSIZE");
		exit(1);
	}
	
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
	free(chunk_buf);
}

void* readWithCache(void* cache_index_ptr){
	int i;
	char* chunk_buf;
	int cache_index=*(int*)cache_index_ptr;

	if(0!=posix_memalign((void **)&chunk_buf,getpagesize(),CHUNK_SIZE)){
		perror("error posix_memalign CHUNKSIZE");
		exit(1);
	}
	
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
		printf("(done %d) in pbuf number:%d\n",i,cache_index);
		updata_in_pbuf(pbuf_manager_ptr->pbuf_address[cache_index],
			chunk_buf,CHUNK_SIZE,i*CHUNK_SIZE);
	}
	free(chunk_buf);
}

void codeSub_read_withCache(int socket){
	//get cache
	int cache_index;
	pthread_mutex_lock(&(pbuf_manager_ptr->managerMutex));
	cache_index=get_pbuf();
	pthread_mutex_unlock(&(pbuf_manager_ptr->managerMutex));

	//pthread
	pthread_t	read_pthread;
	if(0!=pthread_create(&read_pthread,&detachAttr,readWithCache,&cache_index)){
		perror("error pthead_create in codeSub_read_withCache");
		exit(1);
	}
	//send data
	int shouldBytes=CHUNK_SIZE,nowBytes,nBytes;
	char* buf;
	int i;
	if(NULL==(buf=(char*)malloc(CHUNK_SIZE))){
		perror("error malloc in codeSub_read_withCache");
		exit(1);
	}
	for(i=0;i<CHUNK_NUM;i++){
		printf("(done count:%d)out pbuf number:%d\n",i,cache_index);
		updata_out_pbuf(pbuf_manager_ptr->pbuf_address[cache_index],buf);
		nowBytes=0;
		while(nowBytes<shouldBytes){
			if(0>(nBytes=write(socket,buf+nowBytes,shouldBytes-nowBytes))){
				perror("error write in codeSub_read_withCache");
				exit(1);
			}else{
				nowBytes += nBytes;
			}
		}
	}
	//put cache
	pthread_mutex_lock(&(pbuf_manager_ptr->managerMutex));
	put_pbuf(cache_index);
	pthread_mutex_unlock(&(pbuf_manager_ptr->managerMutex));
	//done
	printf("done!\n");
}

void* codeSub(void* arg){
//without memory cache

	data_request* codeRequestPtr=NULL;

	int socket=*(int*)arg;
//	*(int*)arg=0;

	if((codeRequestPtr=(data_request*)malloc(sizeof(data_request)))==NULL){
		perror("error:malloc data_request");
		exit(1);
	}

	if((read(socket,codeRequestPtr,sizeof(data_request)))!=sizeof(data_request)){
		perror("error:read data_request");
		exit(1);
	}
	printf("data_request:%d,%d\n",codeRequestPtr->blockID,codeRequestPtr->codingNode);

	if(codeRequestPtr->codeWay!=2){
		codeSub_read_dire(socket);
	}else{
		codeSub_read_withCache(socket);
	}
	

	free(codeRequestPtr);
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

void* parchInputPthead(void* arg){

	parch_pthread_arg* parchArgPtr=(parch_pthread_arg*)arg;
	int  	node_index=parchArgPtr->node_index;
	int 	pbuf_index=parchArgPtr->pbuf_index;
	int		count=0;
	int	 	shouldGet=CHUNK_SIZE;
	int  	nbytes;
	int	 	realGet;
	data_request localDataRequest;
	char*	buf_chunk;
	if(0!=posix_memalign((void**)&(buf_chunk),getpagesize(),CHUNK_SIZE)){
		perror("error malloc in parchInputPthread");
		exit(1);
	}

	int fd;
	if(node_index==ipSequence){
	//local
		fd=localfd;
	}else{
	//remote
		fd=doAsClient(node_index,DATAPORT);
		localDataRequest.blockID=0;
		localDataRequest.codingNode=ipSequence;
		localDataRequest.codeWay=2;
		if((write(fd,&localDataRequest,sizeof(data_request)))!=sizeof(data_request)){
			perror("error:write dataRequest");
			exit(1);
		}
	}

	while(count<CHUNK_NUM){
		
		realGet=0;
		while(realGet<shouldGet){
			if((nbytes=read(fd,buf_chunk+realGet,shouldGet-realGet))<0){
				perror("error read in parchInputThread");
				exit(1);
			}else{
				realGet += nbytes;
			}
		}
		printf("(node %d,count %d)in pbuf number:%d\n",node_index,count,pbuf_index);
		updata_in_pbuf(pbuf_manager_ptr->pbuf_address[pbuf_index],buf_chunk,CHUNK_SIZE,count*CHUNK_SIZE);
		count++ ;
	}

	free(buf_chunk);

	return ((void*)0);
}

void* codeMainPArch(void* arg){
	int i;
	int* numDetailPtr;
	int		count=0;
	coding_strip_str* encodeArg=(coding_strip_str*)arg;
	int k_rs=encodeArg->data_blocks_num;
	int r_rs=encodeArg->parity_blocks_num;
	//get pbuf
	int* pbufArr;
	if(NULL==(pbufArr=(int*)malloc(encodeArg->data_blocks_num*sizeof(int)))){
		perror("error malloc in codeMainPArch");
		exit(1);
	}
	pthread_mutex_lock(&(pbuf_manager_ptr->managerMutex));
	for(i=0;i<encodeArg->data_blocks_num;i++){
		pbufArr[i]=get_pbuf();
	}
	pthread_mutex_unlock(&(pbuf_manager_ptr->managerMutex));
	
	//thread
	parch_pthread_arg* pthreadArgArr;
	if(NULL==(pthreadArgArr=(parch_pthread_arg*)malloc
		(encodeArg->data_blocks_num*sizeof(parch_pthread_arg)))){
		perror("error malloc");
		exit(1);
	}
	pthread_t parchPthread;
	for(i=0;i<encodeArg->data_blocks_num;i++){
		pthreadArgArr[i].node_index = encodeArg->data_node_arr[i];
		pthreadArgArr[i].pbuf_index = pbufArr[i];
		if(0!=(pthread_create(&parchPthread,&detachAttr,parchInputPthead,(void*)(pthreadArgArr+i)))){
			perror("error:pthread_create smartnodeRecvFrom");
			exit(1);
		}
	}
	//calculate
	char** bufArr;
	if(NULL==(bufArr=(char**)malloc((k_rs+r_rs)*(sizeof(char*))))){
		perror("error malloc in codeMainPArch");
		exit(1);
	}
	for(i=0;i<(k_rs+r_rs);i++){
		if(NULL==(bufArr[i]=(char*)malloc(CHUNK_SIZE*sizeof(char)))){
			perror("error malloc in codeMainPArch");
			exit(1);
		}
	}

	while(count<CHUNK_NUM){
	//get data without calculate and write
		for(i=0;i<k_rs;i++){
			printf("(done count %d)out pbuf number:%d\n",count,pbufArr[i]);
			updata_out_pbuf(pbuf_manager_ptr->pbuf_address[pbufArr[i]],bufArr[i]);
		}
		count++ ;
	}

	//put cache
	pthread_mutex_lock(&(pbuf_manager_ptr->managerMutex));
	for(i=0;i<encodeArg->data_blocks_num;i++){
		put_pbuf(pbufArr[i]);
	}
	pthread_mutex_unlock(&(pbuf_manager_ptr->managerMutex));

	for(i=0;i<(k_rs+r_rs);i++)
		free(bufArr[i]);
	free(bufArr);
	free(pthreadArgArr);
	free(pbufArr);
	return ((void*)0) ;
}

void* codeMain(void* arg){
	int i,j;
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
	
	data_request dataRequest;
	
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
			if(encodeArg->data_node_arr[i]==ipSequence){
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
				gettimeofday(&start,NULL);
				while(nowBytes<shouldBytes){
					if((nBytes=read(codeMainSocket[i],codeMainStr.dataBuffer[i]+nowBytes,
						shouldBytes-nowBytes))==-1){
						perror("error:read datanode socket data");
						exit(1);
					}
					nowBytes += nBytes;
				}
				gettimeofday(&end,NULL);
				timecost = ((end.tv_sec-start.tv_sec)+(end.tv_usec-start.tv_usec)*0.000001);
				socketTimetotal += timecost;

			}
		}

		for(i=0;i<encodeArg->parity_blocks_num;i++){
			memcpy(parityBuffer+i*BLOCK_SIZE+codeMainStr.curNum*CHUNK_SIZE,
				codeMainStr.dataBuffer[0],CHUNK_SIZE);
			if(0>fprintf(diskRateFile,"disk write,stripID(%d) rate(%lfMBPS)\n",encodeArg->stripID,
				((double)CHUNK_SIZE/1024/1024)/((end.tv_sec-start.tv_sec)+(end.tv_usec-start.tv_usec)*0.000001))){
				perror("error:fprintf diskRateFile");
				exit(1);
			}
		}
		(codeMainStr.curNum)++;
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
	if(codingStripStr->codeWay==2){
		
		if((pthread_create(&encodePthread,NULL,codeMainPArch,codingStripStr))!=0){
				perror("error:pthread_create encoding");
				exit(1);
		}

	}else{
	
		if((pthread_create(&encodePthread,NULL,codeMain,codingStripStr))!=0){
				perror("error:pthread_create encoding");
				exit(1);
		}
		
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
	int times=5;
	int ret;
again:
	client_socket= wyc_socket_client_create();

//	printf("Do_as_client(IP):%s\n",IP[server_seq]);
	ret=wyc_socket_connect(IP[server_seq],client_socket,port+server_seq);
//	printf("Do_as_client:(socket):%d\n",client_socket);

	if((ret!=0)&&(times>0)){
		wyc_socket_close(client_socket);
		times-- ;
		printf("sleep 1 for connect refused\n");
		sleep(1);
		goto again;
	}

	if(ret!=0){
		printf("connect error end\n");
		exit(1);
	}


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
