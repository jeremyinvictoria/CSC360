#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
void count_available_blocks(void * start_of_fat, int num_of_blocks_in_fat, int block_size){
	void* start_block_addr = start_of_fat;
	int total_byte_of_fat = num_of_blocks_in_fat * block_size;
	int num_of_entries = total_byte_of_fat/4;
	int i;
	int content=0;
	int count_free = 0;
	int count_reserved = 0;
	int count_allocated = 0;
	/*
	*get available blocks for file
	*/
	for(i=0;i<num_of_entries;i++){
		memcpy(&content, start_block_addr+i*4,4);
		content = htonl(content);
		if(content==0){
			count_free++;
		}
		if(content==1){
			count_reserved++;
		}
		if((content!=1)&&(content!=0)){
			count_allocated++;
		}
	}
	printf("Free Blocks: %d\n",count_free);
	printf("Reserved Blocks: %d\n",count_reserved);
	printf("Allocated Blocks: %d\n",count_allocated);
}
int main(int argc, char* argv[]) {
	if(argv[1]==NULL){
		printf("Please Enter an img file!\n");
		exit(0);
	}
	char* userInput= argv[1];
    int fd = open(userInput, O_RDWR);
    //int fd = open("test.img", O_RDWR);
    struct stat buffer;
    int status = fstat(fd, &buffer);
    char* address=mmap(NULL, buffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    int blocksize = 0;
    memcpy(&blocksize,address+8,2);
    blocksize=htons(blocksize);
    printf("Super block information:\n");
    printf("Block size: %d\n",blocksize);
    int blockCount=0;
    memcpy(&blockCount,address+10,4);
    blockCount = htonl(blockCount);
    printf("Block count: %d\n",blockCount);
    int Fat_start = 0;
    memcpy(&Fat_start, address+14,4);
    Fat_start = htonl(Fat_start);
    printf("FAT starts: %d\n",Fat_start);
    int num_of_blocks_in_fat = 0;
    memcpy(&num_of_blocks_in_fat,address+18,4);
    num_of_blocks_in_fat = htonl(num_of_blocks_in_fat);
    printf("FAT blocks: %d\n",num_of_blocks_in_fat);
    int root_dir_start = 0;
    memcpy(&root_dir_start,address+22,4);
    root_dir_start = htonl(root_dir_start);
    printf("Root directory start: %d\n",root_dir_start);
    int root_blocks = 0;
    memcpy(&root_blocks, address+26,4);
    root_blocks = htonl(root_blocks);
    printf("Root directory blocks: %d\n",root_blocks);
	void* fat_start_addr = address+blocksize*Fat_start;
	printf("\nFAT information:\n");
	count_available_blocks(fat_start_addr,num_of_blocks_in_fat,blocksize);
    munmap(address,buffer.st_size);
    close(fd);
}
