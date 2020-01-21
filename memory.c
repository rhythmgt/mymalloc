#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include "memory.h"

#define PAGE_SIZE 4096

static long long int* free_list[] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
static int sizeRef[] = {16, 32, 64, 128, 256, 512, 1024, 2048, 4096};

static void *alloc_from_ram(size_t size)
{
	assert((size % PAGE_SIZE) == 0 && "size must be multiples of 4096");
	void* base = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
	if (base == MAP_FAILED)
	{
		printf("Unable to allocate RAM space\n");
		exit(0);
	}
	return base;
}

static void free_ram(void *addr, size_t size)
{
	munmap(addr, size);
}

int getSize(int k){
	int ans = -1;
	for (int i = 0; i<=8; ++i){
		if (k<=sizeRef[i]){
			ans = i;
			break;
		}
	}
	if (ans!=-1){
		return ans;
	}
	return k;
}

int getSize_large(int k){
	int i = 1;
	while (4096*i < k){
		i++;
	}
	return 4096*i;
}

/* 
void* pop_from_list(long long int* ptr){
	if (ptr!=NULL){
		long long int* metadata = ptr - ((long long int)ptr)/8;
		*(metadata+1) = *(metadata+1) -1;	// decrement available space
		*((long long int*)*(ptr+1)) = NULL;
		return (void*)ptr;
	}


} */

void* pop_from_list(int i){
	if (free_list[i]!=NULL){
		long long int* metadata = free_list[i] - ((long long int)free_list[i])/8;
		*(metadata+1) = *(metadata+1) -1;	// decrement available space
		long long int * ptr = free_list[i];
		
		*((long long int*)*(free_list[i]+1)) = 0L;
		free_list[i] = (long long int*)*(free_list[i]+1);
		return (void*)ptr;
	}


}
void cleanup(int index, long long int* metaData){
	while (free_list[index] != NULL && (free_list[index] -((long long int )free_list[index] % 4096) == metaData)){
		pop_from_list(index);
	}
	
	if (free_list[index] != NULL){
		long long int* next =(long long int*) *(free_list[index]+1);
		long long int* ptr = free_list[index];
		while(next!= NULL){
			if (next - ((long long int)next %4096) == metaData){
				*(ptr+1) = *(next+1);
				*(long long int*)*(next+1) = (long long int)ptr;
			}
			ptr = next;
			next = (long long int*)*(next + 1);
			//prev = next;
		}
	}
	
	free_ram(metaData, 4096);
}

void add_to_list(long long int* ptr, long long int * block){
	*block = 0L;
	*(block+1) = (long long int)ptr;
	*ptr = (long long int)block;
	long long int* metaData = ptr - ((long long int)ptr%4096)/8;
	*(metaData+1) = *(metaData+1) + *(metaData);
	if (*(metaData+1)>=4096){
		int calcSize = getSize(*metaData);
		cleanup(calcSize, metaData);
	}
	ptr = block;
}


void myfree(void *ptr)
{
	long long int* page = (long long int*) ptr;
	page = page - ((long long int) page %4096);
	int size = *(page);
	if (size<=4080){
		int sz = getSize(size);
		long long int* list = free_list[sz];
		add_to_list(list, (long long int*) ptr);
	}
	else{
		free_ram((void*) page, size);
	}/* 
	printf("myfree is not implemented\n");
	abort(); */
}



void *mymalloc(size_t size)
{
	if (size > 4080){
		int calcSize = getSize(size);
		long long int * ptr = free_list[calcSize];
		if (ptr==NULL){
			long long int* page = (long long int *) alloc_from_ram(4096);
			long long int* endLimit = page + 4096;
			*page = size;
			page ++;
			*page = 4096 - 16;
			page ++;
			while(page < endLimit ){
				add_to_list(ptr, page);
				page += (size/8);
			}
		}
		return pop_from_list(calcSize);
	}
	
	else{
		int size = getSize_large(size+8);
		void* output = alloc_from_ram(size);
		long long int* output2 = (long long int *) output;
		*(output2) = size;
		output2 ++;
		return (void*) output2;
	}

	/* printf("mymalloc is not implemented\n");
	abort();
	return NULL; */
}
