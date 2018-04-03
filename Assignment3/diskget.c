#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> 
struct __attribute__((__packed__)) dir_entry_timedate_t {
	uint16_t year;
	uint8_t month;
 	uint8_t day;
 	uint8_t hour;
 	uint8_t minute;
 	uint8_t second;
};
struct __attribute__((__packed__)) superblock_t {
    uint8_t   fs_id [8];
    uint16_t block_size;
    uint32_t file_system_block_count;
    uint32_t fat_start_block;
    uint32_t fat_block_count;
    uint32_t root_dir_start_block;
    uint32_t root_dir_block_count;
};
struct __attribute__((__packed__)) dir_entry_t {
	uint8_t status;
	uint32_t starting_block;
	uint32_t block_count;
	uint32_t size;
	struct dir_entry_timedate_t create_time;
	struct dir_entry_timedate_t modify_time;
	uint8_t filename[31];
	uint8_t unused[6];
};
/*
get starting block of the file and number of blocks allocated to the file
*/
int* get_info_in_root(int num_of_entries, void* start_block_of_root_dir,char* filename){
	int i;
	static int info[2]={-1,-1};
	for(i=0;i<num_of_entries;i++){
		struct dir_entry_t* det;
		//struct dir_entry_t* det;
		void* start_of_entry = start_block_of_root_dir+64*i;
		//memcpy(det,start_of_entry,64);
		det = (struct dir_entry_t*) start_of_entry;
		char ftype;
		if(det->status!=0){
			/*if(det->status==3){
				ftype='F';
			}else if(det->status==5){
				ftype='D';
			}*/
			//printf("%c	",ftype);
			//printf("status: %d\n",det->status);
			//printf("%d	",ntohl(det->size));
			//printf("%s	",det->filename);
			//printf("%d/%d/%d %d:%d:%d\n",ntohs(det->modify_time.year),(det->modify_time.month),(det->modify_time.day),(det->modify_time.hour),(det->modify_time.minute),(det->modify_time.second));
			//char this_file_name[31];
			//this_file_name = det->filename;
			char this_filename[31];
			strcpy(this_filename, (char *)det->filename);
			//printf("\nthis_filename: %s\n",this_filename);
			if(strcmp(filename,this_filename)==0){
				//printf("\nstart block is:%d \n",ntohl(det->starting_block));
				//printf("file found\n");
				printf("blocks of file: %d\n",ntohl(det->block_count));
				int start_block = ntohl(det->starting_block);
				int num_of_block = ntohl(det->block_count);
				info[0] = start_block;
				info[1] = num_of_block;
				//return start_block;	
			}
		}
		
	}
	return info;
	
}
/*
use starting block to figure out all blocks allocated to the file,
store blocks in an array and return it
*/
int* find_file_blocks(void* fat_start,int start_entry,int num_of_blocks){
	//printf("%d blocks has been allocated to this file\n",num_of_blocks);
	const int length = num_of_blocks;
	//static int blocks[length];
	int temp_start = start_entry;
	static int blocks[100];
	blocks[0] = start_entry;
	int i;
	int content_in_entry=0;
	//store each allocated block to the array
	for(i=0;i<num_of_blocks;i++){
		memcpy(&content_in_entry,fat_start+temp_start*4,4);
		//printf("\ncontent is %d\n",ntohl(content_in_entry));	
		blocks[i+1]=ntohl(content_in_entry);
		temp_start = ntohl(content_in_entry);
	}
	
	return blocks;
}
void copy_file(int *blocks, void* starting_pointer_of_fs, int size_of_block, char* new_name){
	int* temp = blocks;
	FILE * fp;
	fp = fopen(new_name,"w");
	char content_in_block[512];
	int j=0;
	while(1){
		if(temp[j]==-1) break;
		memcpy(content_in_block,starting_pointer_of_fs+temp[j]*size_of_block,512);
		//printf("107 content of block 1 is: %s\n",content_in_block);
		//fprintf(fp,"%s",content_in_block);	
		fwrite(content_in_block,sizeof(char),512,fp);
		j++;
	}
	
	//print blocks allocated to the file
	/*int i=0;
	while(1){
		if(temp[i]==-1) break;
		printf("111: %d\n",temp[i]);
		i++;
	}*/
	fclose(fp);
}
int main(int argc, char* argv[]) {
	//printf("%s\n",argv[0]);
	if(argv[1]==NULL){
		printf("Please enter an img file\n");
		exit(0);
	}
	char *img_file = argv[1];
	if(argv[2]==NULL){
		printf("Please enter a directory\n");
		exit(0);
	}
	char *subdir = argv[2];
	/*tokenize userinput by '/'*/
	//char buf[] ="abc/qwe/ccd";
    int i = 0;
    char *p = strtok (subdir, "/");
    char *array[10];
    while (p != NULL)
    {
        array[i++] = p;
        p = strtok (NULL, "/");
    }
	char* file_in_dir = array[0];
	/****************************/
	
	int fd = open(img_file, O_RDWR);
    struct stat buffer;
    int status = fstat(fd, &buffer);
    void* address=mmap(NULL, buffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    struct superblock_t* sb;
    sb=(struct superblock_t*)address;
    /*
    *point to start of root directory
    */
    int block_size = ntohs(sb->block_size);
    void* start_block_of_root_dir = address+ntohs(sb->block_size)*ntohl(sb->root_dir_start_block);
    //struct dir_entry_t* det;
    //det = (struct dir_entry_t*)start_block_of_root_dir;
    //printf("blocks in root dir: %d\n",ntohl(sb->root_dir_block_count));
    int entries_in_rootDir = (ntohl(sb->root_dir_block_count)*ntohs(sb->block_size))/64;
	int *result = get_info_in_root(64, start_block_of_root_dir,file_in_dir);
	if(result[0]<0){
		printf("file not found\n");
		return 0;
	}
	//printf("file found and start entry in FAT is:%d\n",result[0]);
	int FAT_start = ntohl(sb->fat_start_block);
	//printf("FAT starts at block:%d\n",FAT_start);
	void* fat_start = address+ntohs(sb->block_size)*FAT_start;
	int* result2 = find_file_blocks(fat_start, result[0],result[1]);
	if(argv[3]==NULL){
		printf("Please enter the destination file you want to copy into\n");
		exit(0);
	}
	copy_file(result2,address,block_size,argv[3]); 
	/*
	int i=0;
	//print blocks allocated to the file
	while(1){
		if(result2[i]==-1) break;
		printf("132: %d\n",result2[i]);
		i++;
	}*/
    munmap(address,buffer.st_size);
    close(fd);
    return 0;
}

