#include"smartArchive.h"
#define __USE_GNU 1
#include<fcntl.h>
#include<unistd.h>

#define	DISKFILE "disk.config"

struct data_request{
	int		blockID;
	int		codingNode;
};
typedef struct data_request data_request; 

struct erase_code_main{
//	int 	done;
	int 	totoalNum;
	int		curNum;
	char*	dataBuffer[K_MAX];
//	char*	checkBuffer[R_MAX];
//	int		dataBufferReady[K_MAX];
//	int		checkBufferReady[K_MAX];
};
typedef struct erase_code_main erase_code_main;

struct erase_code_sub{
	int		blockID;
	int		codingNode;
	int		socket;
	int 	toalNum;
	int		readNum;
	int		sendNum;
	char*	dataBuffer[CHUNK_NUM];//[CHUNK_SIZE]
};
typedef struct erase_code_sub erase_code_sub;