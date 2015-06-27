#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<time.h>
#include<unistd.h>

#define		NODE_MAX		8
#define		BLOCKID_MAX		60
#define 	K_MAX			10
#define		K_MIN			4
#define		R_MAX			3
#define		CLIENT_MAX		5
#define 	PORT			12345
#define		DATAPORT		12000
#define 	IPLENGYH		16			//###.###.###.###
#define		RS_K			4
#define		RS_R			1
#define		IPFILE			("ip.config")
#define		BLOCK_SIZE		(64*1024*1024)   	//B
#define		CHUNK_SIZE		(64*1024)			//B
#define		CHUNK_NUM		((BLOCK_SIZE)/(CHUNK_SIZE))
#define		MEM_BUF_SIZE	(8*1024*1024)
#define		MEM_BUF_NUM		((MEM_BUF_SIZE)/(CHUNK_SIZE))
#define		TOTAL_MEM		(2*1024)			//MB

#define		DISKRATEFILE	("diskrate.txt")
//#define		SOCKETRATEFILE	("socketrate.txt")
#define		STRIPRESULTFILE	("striptimeresult.txt")

struct coding_strip_str{
	int		stripID;
	int		codeWay;			//0 CArch 1 DArch 2 PArch 3 BArch
	int		first_blockID;
	int 	data_blocks_num;
	int		parity_blocks_num;
	int		coding_node;
	int		locality;
	int 	data_node_arr[K_MAX];
};
typedef struct coding_strip_str coding_strip_str;

struct coding_result{
	int		stripID;
	int		done;
};
typedef struct coding_result coding_result; 

