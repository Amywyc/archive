#include<limits.h>

#ifndef WYC_SMARTCONTROL_H

#define WYC_SMARTCONTROL_H
#include"smartControl.h"
#include"wyc_socket.h"

#define	STRIPSIZE	((BLOCKID_MAX)/(K_MIN))

#endif

int		encodingNodeChooseWay;  //0(Locality) 1(Balance) 2(Balance&&Locality) 

char 	IP[NODE_MAX][IPLENGYH];
int 	ipSequence;
int 	ipCount;

int	socketStart;
pthread_mutex_t			socketStartMutex;


//data
int BLK_2_ND[NODE_MAX][BLOCKID_MAX];
int	TASK_MAP[NODE_MAX];
int block_cur_max,node_cur_max;

//communicata struct 
int			datanodeSocket[NODE_MAX];    //connect smartnode(client)-->datanode(server)
int			stripHandlerCount;
pthread_mutex_t			stripHandlerCountMutex;
coding_strip_str*	encodingStrip[STRIPSIZE];
int					encodingStripID;


//pthread
pthread_t	encodeNodeChoosePthread;
pthread_attr_t	attr;

int					listenSocket;
int					smartSocket;
int					actionSocket[CLIENT_MAX];	   //for server socket
int					actionSocketCount;
struct sockaddr_in 	client;
static int doAsClient(int server_seq);
static void doAsServer();


//function
void init_wyc();
void free_wyc();
void getBlocks2NodesMap();
void printBlocks2NodesMap();
//void getRandomBlocksConfig(int blocks,int rep_factor,int nodes);
void chooseCodingnodeByLocality(int blockID,int k,int r,coding_strip_str* codestruct);
void chooseCodingnodeByBalance(int blockID,int k,int r,coding_strip_str* codestruct);
void chooseCodingnodeByBalAndLol(int blockID,int k,int r,coding_strip_str* codestruct);
void chooseCodingnodeByNoLocality(int blockID,int k,int r,coding_strip_str* codestruct);
void chooseCodingnode();
void printCodingStripStruct(coding_strip_str* ptr);
void printCodingresult(coding_result* res);
void freshTASTMAP(coding_strip_str* ptr);
void getIP();
//void smartSocketToDatanode();

void* encodeNodeChoose();

void* stripHandler(void* arg);

void init_wyc(){
	int i;
	//task init
//	for(i=0;i<NODE_MAX;i++){
//		datanodeRequest[i].start=NULL;
//		datanodeRequest[i].end=NULL
//	}
	for(i=0;i<NODE_MAX;i++)
		TASK_MAP[i]=0;
	for(i=0;i<STRIPSIZE;i++)
		encodingStrip[i]=NULL;
	actionSocketCount=0;
	encodingStripID=0;
	encodingNodeChooseWay=2;    //1 DArch 2 PArch

	getIP();					//read IP from file
//	smartSocketToDatanode();	//socket smartnode to every datanode
	getBlocks2NodesMap();		//get map from datanode to blockID
	printBlocks2NodesMap();		//print map from datanode to blockID

	if((pthread_attr_init(&attr))!=0){
		perror("error:pthread_attr_init attr");
		exit(1);
	}
	if((pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED))!=0){
		perror("error:pthread_attr_setdetachstate attr");
		exit(1);
	}

	stripHandlerCount=0;
	pthread_mutex_init(&stripHandlerCountMutex,NULL);

	socketStart=0;
	pthread_mutex_init(&socketStartMutex,NULL);
	
}

void free_wyc(){

	pthread_attr_destroy(&attr);

	pthread_mutex_destroy(&stripHandlerCountMutex);

	pthread_mutex_destroy(&socketStartMutex);

}

void freshTASTMAP(coding_strip_str* ptr){
	//recycle tasks
	int i;
	int localBlocks=0;
	for(i=0;i<ptr->data_blocks_num;i++){
		if(ptr->data_node_arr[i]!=ptr->coding_node){
			TASK_MAP[ptr->coding_node]--;
			TASK_MAP[ptr->data_node_arr[i]]--;
		}
	}
	TASK_MAP[ptr->coding_node] -= ptr->parity_blocks_num;
}

void recvcoding_result(coding_result* arg,int datanode){
	int res;
	if((res=read(datanodeSocket[datanode],arg,sizeof(coding_result)))!=sizeof(coding_result)){
		perror("error:read coding_result");
		printf("datanode:%d(%d)(%d)\n",datanode,datanodeSocket[datanode],res);
		exit(1);
	}
	printf("stringID:%d done!\n",arg->stripID);
}


//void* datanodeRecvFrom(void* arg){   //recv coding_result

//	int datanode=*(int*)arg;
//	coding_result ecdResult;
//	printf("datanodeRecvFrom start(%d)!\n",datanode);
//	while(1){
//		printf("datanodeRecvFrom\n");
//		recvcoding_result(&ecdResult,datanode);
//		freshTASTMAP(encodingStrip[ecdResult.stripID]);
//		printf("***smartnode:recv from datanode\n");
//		printCodingresult(&ecdResult);
//	}
//	
//	return ((void*)0);

//}

void sendcoding_strip_str(coding_strip_str* arg){
	printf("datanodeSocket:%d\n",datanodeSocket[arg->coding_node]);
	if(write(datanodeSocket[arg->coding_node],arg,sizeof(coding_strip_str))!=sizeof(coding_strip_str)){
		perror("error:write coding_strip_str");
		exit(1);
	}
}

void* stripHandler(void* arg){
	
	int stripID=*(int*)arg;
	coding_result res;

	int socket=doAsClient(encodingStrip[stripID]->coding_node);//PORT
	pthread_mutex_lock(&socketStartMutex);
	socketStart++;
	pthread_mutex_unlock(&socketStartMutex);	
	if((write(socket,encodingStrip[stripID],sizeof(coding_strip_str)))!=sizeof(coding_strip_str)){
		perror("error:smartnode write coding_strip_str");
		exit(1);
	}

	printCodingStripStruct(encodingStrip[stripID]);
	
	if((read(socket,&res,sizeof(res)))!=sizeof(coding_result)){
		perror("error:smartnode read coding_result");
		exit(1);
	}

	printCodingresult(&res);

	if(res.done==1){
		freshTASTMAP(encodingStrip[res.stripID]);
		free(encodingStrip[res.stripID]);
		encodingStrip[res.stripID]==NULL;
	}else{
		printf("smartnode:stripID(%d) fail\n",res.stripID);
		exit(1);
	}

	pthread_mutex_lock(&stripHandlerCountMutex);
	stripHandlerCount--;
	pthread_mutex_unlock(&stripHandlerCountMutex);
	
	return ((void*)0);
}

//void* datanodeSendTo(void* arg){  //send coding_strip_str

//	int stripID=*(int*)arg;
//	encode_request* ecdRequest=NULL;
//	while(1){
//		
//		if(datanodeRequest[datanode].start==NULL){
//			sleep(2);
//		}else{

//			pthread_mutex_lock(&datanodeReqestMutex);
//		
//			ecdRequest=datanodeRequest[datanode].start;
//			printf("start:stripID(%d)\n",ecdRequest->stripID);
//			datanodeRequest[datanode].start=datanodeRequest[datanode].start->next;

//			pthread_mutex_unlock(&datanodeReqestMutex);

//			
//			sendcoding_strip_str(encodingStrip[ecdRequest->stripID]);
//			printf("***smartnode:send to datanode");
//			printCodingStripStruct(encodingStrip[ecdRequest->stripID]);
//	
//		}
//		
//	}

//	return ((void*)0);
//}

void* encodeNodeChoose(){

	FILE*	nodeFile;

	int i;
	pthread_t	stripHandlerPthread;
//	encode_request* ecdRequest=NULL;//encodingRequest

	if((nodeFile=fopen(NODES_FILES,"w"))==NULL){
		perror("error:fopen NODE_FILE");
		exit(1);
	}

	if(block_cur_max%RS_K!=0){
		printf("error:block_cur_max mod RS_K!=0\n");
		exit(1);
	}
	for(i=0;i<(block_cur_max);i+=RS_K){

		while(encodingStrip[encodingStripID]!=NULL)
			encodingStripID++;

		if((encodingStrip[encodingStripID]=malloc(sizeof(coding_strip_str)))==NULL){
			perror("error:malloc coding_strip_str");
			exit(1);
		}
		encodingStrip[encodingStripID]->stripID = encodingStripID;
		switch (encodingNodeChooseWay){
			case 0:
				//CArch
				//chooseCodingnodeByLocality(i,RS_K,RS_R,encodingStrip[encodingStripID]);
				break;
			case 1:
				//DArch
				//chooseCodingnodeByBalance(i,RS_K,RS_R,encodingStrip[encodingStripID]);
				chooseCodingnodeByLocality(i,RS_K,RS_R,encodingStrip[encodingStripID]);
				break;
			case 2:
				//PArch
				//chooseCodingnodeByBalAndLol(i,RS_K,RS_R,encodingStrip[encodingStripID]);
				chooseCodingnodeByLocality(i,RS_K,RS_R,encodingStrip[encodingStripID]);
				break;
			case 3:
				//BArch
				chooseCodingnodeByNoLocality(i,RS_K,RS_R,encodingStrip[encodingStripID]);
				break;
			default:
				printf("error:encodingNodeChooseWay[0\1\2]:%d\n",encodingNodeChooseWay);
				exit(1);
		}
		encodingStrip[encodingStripID]->codeWay=encodingNodeChooseWay;
		printf("In encodeNodeChoose:");
		printCodingStripStruct(encodingStrip[encodingStripID]);
		if(fprintf(nodeFile,"%d,%d,%d,%d,%d,%d,%d\n",
			encodingStripID,
			encodingStrip[encodingStripID]->coding_node,
			encodingStrip[encodingStripID]->data_node_arr[0],
			encodingStrip[encodingStripID]->data_node_arr[1],
			encodingStrip[encodingStripID]->data_node_arr[2],
			encodingStrip[encodingStripID]->data_node_arr[3],
			encodingStrip[encodingStripID]->locality)<0){
				perror("error:fprintf NODEFILE");
				exit(1);
		}

		if((pthread_create(&stripHandlerPthread,&attr,stripHandler,&(encodingStrip[encodingStripID]->stripID)))!=0){
			perror("error:pthread_create datanodeSendTo");
			exit(1);
		}

		pthread_mutex_lock(&stripHandlerCountMutex);
		stripHandlerCount++;
		pthread_mutex_unlock(&stripHandlerCountMutex);
	}

	while(stripHandlerCount!=0){
		printf("socketStart:%d unfinished:%d\n",socketStart,stripHandlerCount);
		sleep(1);
	}
	//done

	if(fclose(nodeFile)!=0){
		perror("error:fclose NODE_FILE");
		exit(1);
	}
	
	return ((void*)0);
}

//void smartSocketToDatanode(){
//	int i;
//	printf("datanodeSocket:IP(socket)\n");
//	for(i=0;i<ipCount;i++){
//		datanodeSocket[i]=doAsClient(i);
//		printf("%s(%d)\n",IP[i],datanodeSocket[i]);
//	}
//}

static void doAsServer(){

	//open server socket PORT+sequence
	listenSocket = wyc_socket_server_create(PORT+ipSequence);
	
	while(1){
		//accept client 
		actionSocket[actionSocketCount] = wyc_socket_accept(listenSocket,&client);
		printf("client:%s\n",inet_ntoa(client.sin_addr));
		actionSocketCount++;
	}
	
	pthread_exit(NULL);

	return ;
}

static int doAsClient(int server_seq){
	printf("server_seq:%d\n",server_seq);
	
	int client_socket=-1;

	client_socket= wyc_socket_client_create();
		
	wyc_socket_connect(IP[server_seq],client_socket,PORT+server_seq);

	printf("Do_as_client:socket ready!\n");

	return client_socket;

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
	for(i=0;i<ipCount;i++)
		printf("%d ip(%d):%s\n",i,(int)strlen(IP[i]),IP[i]);
	printf("\n");
	if(fclose(ipfl)!=0){
		perror("error:fclose IPFILE");
		exit(1);
	}
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

void printTASKMAP(){
	int i;
	printf("TASKMAP:nodeID(Tasks)\n");
	for(i=0;i<NODE_MAX;i++)
		printf("%d(%d) ",i,TASK_MAP[i]);
	printf("\n");
}

void chooseCodingnode(){

	printTASKMAP();
	
	coding_strip_str code_arg1;
	code_arg1.stripID=1;
	chooseCodingnodeByLocality(0,4,1,&code_arg1);

	sendcoding_strip_str(&code_arg1);

	printTASKMAP();

	coding_strip_str code_arg2;
	code_arg2.stripID=2;
	chooseCodingnodeByLocality(4,4,1,&code_arg2);

	sendcoding_strip_str(&code_arg2);


	printTASKMAP();

	freshTASTMAP(&code_arg1);
	freshTASTMAP(&code_arg2);

	printTASKMAP();
	
}

void chooseCodingnodeByBalAndLol(int blockID,int k,int r,coding_strip_str* codestruct){
	//smart=max[locality-task]
	int i,j,smart,smartIndex,minTask,minTaskIndex;;
	int *localityCount=malloc(sizeof(int)*node_cur_max);
	codestruct->locality=0;
	for(i=0;i<node_cur_max;i++)
		localityCount[i]=0;
	for(i=0;i<k;i++){
		for(j=0;j<node_cur_max;j++){
			if(BLK_2_ND[j][blockID+i]==1)
				localityCount[j]++;
		}
	}
	for(i=0;i<node_cur_max;i++){
		if(localityCount[i]!=0)
			printf("node:%d,LocalityCount:%d\n",i,localityCount[i]);
	}
	//get first blockID
	codestruct->first_blockID=blockID;
	//get k
	if(k>K_MAX){
		printf("error:K_MAX\n");
		exit(1);
	}
	codestruct->data_blocks_num=k;
	//get r
	codestruct->parity_blocks_num=r;
	//get coding node
	smart=INT_MIN;
	smartIndex=-1;
	for(i=0;i<k;i++)
		for(j=0;j<node_cur_max;j++){
			if(localityCount[j]-TASK_MAP[j]>smart){
				smart=localityCount[j]-TASK_MAP[j];
				smartIndex=j;
			}
		}
	codestruct->coding_node=smartIndex;
	TASK_MAP[codestruct->coding_node] += codestruct->parity_blocks_num;
	//get data node
	for(i=0;i<k;i++){

		if(BLK_2_ND[codestruct->coding_node][blockID+i]==1){
			//coding node first
			codestruct->data_node_arr[i]=codestruct->coding_node;
			(codestruct->locality)++;
		}else{
			//min task node
			minTask=INT_MAX;
			minTaskIndex=-1;
			for(j=0;j<node_cur_max;j++)
				if(BLK_2_ND[j][blockID+i]==1)
					if(TASK_MAP[j]<minTask){
						minTask=TASK_MAP[j];
						minTaskIndex=j;
					}
			codestruct->data_node_arr[i]=minTaskIndex;
			if(minTaskIndex!=codestruct->coding_node){
				TASK_MAP[minTaskIndex]++;
				TASK_MAP[codestruct->coding_node]++;
			}
		}

	}
	free(localityCount);
}

void chooseCodingnodeByBalance(int blockID,int k,int r,coding_strip_str* codestruct){
	int i,j;
	int minTask,minTaskIndex;
	codestruct->locality=0;
	//get first blockID
	codestruct->first_blockID=blockID;
	//get k
	if(k>K_MAX){
		printf("error:K_MAX\n");
		exit(1);
	}
	codestruct->data_blocks_num=k;
	//get r
	codestruct->parity_blocks_num=r;
	//get coding node
	minTask=TASK_MAP[0];
	minTaskIndex=0;
	for(i=1;i<NODE_MAX;i++)
		if(TASK_MAP[i]<minTask){
			minTask=TASK_MAP[i];
			minTaskIndex=i;
		}
	codestruct->coding_node=minTaskIndex;
	TASK_MAP[codestruct->coding_node] += codestruct->parity_blocks_num;
	//get data node
	for(i=0;i<k;i++){
		minTask=INT_MAX;
		minTaskIndex=-1;
		for(j=0;j<node_cur_max;j++)
			if(BLK_2_ND[j][blockID+i]==1){
				if(TASK_MAP[j]<minTask){
					minTask=TASK_MAP[j];
					minTaskIndex=j;
				}
			}
		codestruct->data_node_arr[i]=minTaskIndex;
		if(minTaskIndex!=codestruct->coding_node){
			TASK_MAP[minTaskIndex]++;
			TASK_MAP[codestruct->coding_node]++;
		}else{
			(codestruct->locality)++;
		}
	}
}

void chooseCodingnodeByLocality(int blockID,int k,int r,coding_strip_str* codestruct){
	int i,j,max,maxIndex;
	int *localityCount=malloc(sizeof(int)*node_cur_max);

	codestruct->locality=0;
	
	for(i=0;i<node_cur_max;i++)
		localityCount[i]=0;
	for(i=0;i<k;i++){
		for(j=0;j<node_cur_max;j++){
			if(BLK_2_ND[j][blockID+i]==1)
				localityCount[j]++;
		}
	}
	for(i=0;i<node_cur_max;i++){
		if(localityCount[i]!=0)
			printf("node:%d,LocalityCount:%d\n",i,localityCount[i]);
	}
	//get first blockID
	codestruct->first_blockID=blockID;
	//get k
	if(k>K_MAX){
		printf("error:K_MAX\n");
		exit(1);
	}
	codestruct->data_blocks_num=k;
	//get r
	codestruct->parity_blocks_num=r;
	//get coding node
	max=0;
	maxIndex=0;
	for(i=0;i<node_cur_max;i++)
		if(localityCount[i]>max){
			max=localityCount[i];
			maxIndex=i;
		}
	codestruct->coding_node=maxIndex;
	TASK_MAP[codestruct->coding_node] += codestruct->parity_blocks_num;
	//get data node
	for(i=0;i<k;i++){
		max=0;
		maxIndex=0;
		for(j=0;j<node_cur_max;j++)
			if(BLK_2_ND[j][blockID+i]==1){
				if(localityCount[j]>max){
					max=localityCount[j];
					maxIndex=j;
				}
			}
		codestruct->data_node_arr[i]=maxIndex;
		if(maxIndex!=codestruct->coding_node){
			TASK_MAP[maxIndex]++;
			TASK_MAP[codestruct->coding_node]++;
		}else{
			(codestruct->locality)++;
		}
	}
	printf("coding node:%d\n",codestruct->coding_node);
	free(localityCount);
}

void chooseCodingnodeByNoLocality(int blockID,int k,int r,coding_strip_str* codestruct){
	int i,j;
	int *localityCount=malloc(sizeof(int)*node_cur_max);
	int minTask,minTaskIndex;

	codestruct->locality=0;
	
	for(i=0;i<node_cur_max;i++)
		localityCount[i]=0;
	for(i=0;i<k;i++){
		for(j=0;j<node_cur_max;j++){
			if(BLK_2_ND[j][blockID+i]==1)
				localityCount[j]++;
		}
	}
	for(i=0;i<node_cur_max;i++){
		if(localityCount[i]!=0)
			printf("node:%d,LocalityCount:%d\n",i,localityCount[i]);
	}
	//get first blockID
	codestruct->first_blockID=blockID;
	//get k
	if(k>K_MAX){
		printf("error:K_MAX\n");
		exit(1);
	}
	codestruct->data_blocks_num=k;
	//get r
	codestruct->parity_blocks_num=r;
	//get coding node
	minTask=TASK_MAP[0];
	minTaskIndex=0;
	for(i=1;i<NODE_MAX;i++)
		if(TASK_MAP[i]<minTask){
			minTask=TASK_MAP[i];
			minTaskIndex=i;
		}
	codestruct->coding_node=minTaskIndex;
	TASK_MAP[codestruct->coding_node] += codestruct->parity_blocks_num;
	//get data node
	for(i=0;i<k;i++){
		minTask=INT_MAX;
		minTaskIndex=0;
		for(j=0;j<node_cur_max;j++)
			if((j!=codestruct->coding_node)&&(BLK_2_ND[j][blockID+i]==1)){
				if(TASK_MAP[j]<minTask){
					minTask=TASK_MAP[j];
					minTaskIndex=j;
				}
			}
		codestruct->data_node_arr[i]=minTaskIndex;
		TASK_MAP[minTaskIndex]++;
		TASK_MAP[codestruct->coding_node]++;
	}
	printf("coding node:%d\n",codestruct->coding_node);
	free(localityCount);
}


void getBlocks2NodesMap(){
	
	int i,j;

	int blockID,nodeID;
	for(i=0;i<NODE_MAX;i++)
		for(j=0;j<BLOCKID_MAX;j++)
			BLK_2_ND[i][j]=0;

	block_cur_max=0;
	node_cur_max=0;
	
	FILE*	blks_cfg_fl;
	if((blks_cfg_fl=fopen(BLOCKS_CONFIG,"r"))==NULL){
		printf("error:open block_config\n");
		exit(1);
	}
	while(2==(fscanf(blks_cfg_fl,"%d,%d\n",&nodeID,&blockID))){
		if((blockID<=BLOCKID_MAX)&&(nodeID<=NODE_MAX)){
			BLK_2_ND[nodeID][blockID]=1;
			if(blockID>block_cur_max)
				block_cur_max=blockID;
			if(nodeID>node_cur_max)
				node_cur_max=nodeID;
		}else{
			printf("error:nodeID or blockID\n");
			exit(1);
		}
	}
	block_cur_max += 1;
	node_cur_max += 1;
	if(fclose(blks_cfg_fl)!=0){
		printf("error:close block_config\n");
		exit(1);
	}
	
}

void printBlocks2NodesMap(){
	int i,j;
	for(i=0;i<node_cur_max;i++){
		for(j=0;j<block_cur_max;j++){
			printf("%d ",BLK_2_ND[i][j]);
		}
		printf("\n");
	}
	printf("The max node number:%d,the current node number:%d\n",NODE_MAX,node_cur_max);
	printf("The max block number:%d,the current blockID:%d",BLOCKID_MAX,block_cur_max);
	printf("\n");
}

void getRandomBlocksConfig(int blocks,int rep_factor,int nodes,int cha){
	//node0:nodeelse=(1+cha):1
	int i,j,k;

	FILE*	blks_cfg_ram_fl;
	if((blks_cfg_ram_fl=fopen(BLOCKS_CONFIG_RAM,"w"))==NULL){
		perror("error:open block_config");
		exit(1);
	}

	int blk_node;
	int rep[3];
	srand((unsigned int)time(0));
	for(i=0;i<blocks;i++){
		for(j=0;j<rep_factor;j++){
			blk_node=rand()%(nodes+cha);
			if(blk_node>=nodes)
				blk_node=0;
			rep[j]=blk_node;
			for(k=0;k<j;k++)
				if(rep[k]==blk_node){
					j--;
					break;
				}
		}
		for(j=0;j<rep_factor;j++)
			if(fprintf(blks_cfg_ram_fl,"%d,%d\n",rep[j],i)<0){
				perror("error:fprintf BLOCK_CONFIG_RAM");
				exit(1);
			}
	}
	if(fclose(blks_cfg_ram_fl)!=0){
		printf("error:close block_config\n");
		exit(1);
	}
}

int main(){

	int i;
	int iarray[NODE_MAX];
	struct	timeval 	start,end;
	double timecast;

//	getRandomBlocksConfig(BLOCKID_MAX,2,NODE_MAX,0);

	init_wyc();

	gettimeofday(&start,NULL);

	if((pthread_create(&encodeNodeChoosePthread,NULL,encodeNodeChoose,NULL))!=0){
		perror("error:pthread_create encodeNodeChoose");
		exit(1);
	}


	pthread_join(encodeNodeChoosePthread,NULL);

	gettimeofday(&end,NULL);
	timecast=(end.tv_sec-start.tv_sec)+(end.tv_usec-start.tv_usec)*0.000001;

	printf("timecast:%lf\n",timecast);

	free_wyc();
	
	return 0;
}

