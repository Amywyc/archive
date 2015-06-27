#include"smartArchive.h"
#define __USE_GNU 1
#include<fcntl.h>
#include<unistd.h>
#include"wyc_list.h"

#define	DISKFILE "disk.config"

struct data_request{
	int 	codeWay;
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

struct parch_pthread_arg{
	int 	pbuf_index;
	int 	node_index;
};
typedef struct parch_pthread_arg parch_pthread_arg;

typedef struct p_buf{
	size_t				cur_size;
	size_t				max_size;
	int 				count;
	off_t				cur_offset;
	list_head			head;
	pthread_mutex_t		lock;
	pthread_mutex_t		head_lock;
	char	*buf;
}p_buf;

struct pbuf_manager{
	int 	total;
	int		used;
	int*	pbuf_usage;
	p_buf**	pbuf_address;
	pthread_mutex_t	managerMutex;
};
typedef struct pbuf_manager pbuf_manager;