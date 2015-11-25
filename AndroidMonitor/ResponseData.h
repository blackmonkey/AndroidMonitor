#ifndef __RESPONSEDATA_H__
#define __RESPONSEDATA_H__

#define RESP_DATA_BLOCK_SIZE (20480) // 20k

typedef struct tagResponseData {
	char buf[RESP_DATA_BLOCK_SIZE];
	int size;
} ResponseData;

#endif // __RESPONSEDATA_H__
