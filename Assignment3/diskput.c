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
int* find_file_blocks(void* fat_start,int start_entry,int num_of_blocks){
	printf("%d blocks has been allocated to this file\n",num_of_blocks);
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
/*void copy_file(int *blocks, void* starting_pointer_of_fs, int size_of_block){
	int* temp = blocks;
	FILE * fp;
	fp = fopen("abcde.txt","w");
	char content_in_block[512];
	int j=0;
	while(1){
		if(temp[j]==-1) break;
		memcpy(content_in_block,starting_pointer_of_fs+temp[j]*size_of_block,512);
		printf("107 content of block 1 is: %s\n",content_in_block);
		//fprintf(fp,"%s",content_in_block);	
		fwrite(content_in_block,sizeof(char),512,fp);
		j++;
	}
	
	//print blocks allocated to the file
	int i=0;
	while(1){
		if(temp[i]==-1) break;
		printf("111: %d\n",temp[i]);
		i++;
	}
	fclose(fp);
}*/
/*
find free blocks by FAT, fill request blocks in FAT entry,
and return the allocated blocks for the file;
*/
int* find_request_blocks(void * start_of_fat, int length_file, int num_of_blocks_in_fat, int block_size){
	void* start_block_addr = start_of_fat;
	int total_byte_of_fat = num_of_blocks_in_fat * block_size;
	int num_of_entries = total_byte_of_fat/4;
	int number_of_request_blocks = length_file/block_size;
	//printf("%d blocks needed for this file\n",number_of_request_blocks);
	int i;
	int content=0;
	
	static int block_array[100];
	//block_array[number_of_request_blocks]=-1;
	int count = 0;
	/*
	*get available blocks for file
	*/
	for(i=0;i<num_of_entries;i++){
		memcpy(&content, start_block_addr+i*4,4);
		if(content==0){
			block_array[count]=i;
			count++;
			if(count>number_of_request_blocks){
				break;
			}
		}
	}
	if(count<number_of_request_blocks){
		printf("No more available blocks for the file!\n");
		exit(0);
	}
	block_array[number_of_request_blocks]=-1;

	/*int j;
	for(j=0;j<sizeof(block_array)/4;j++){
		printf("149: %d\n",block_array[j]);
	}*/
	
	/*
	write the linke-list-allocation array to FAT
	*/
	int j;
	int end=0xffffffff;
	//temp int used to test
	int temp_content=0;
	//int start_block = block_array[0];
	for(j=0;j<sizeof(block_array)/4;j++){
		//int addr_put_in = block_array[j];
		if(j+1<sizeof(block_array)/4){
			int hex = htonl(block_array[j+1]);
			//memcpy(start_block_addr+block_array[j]*4,&block_array[j+1],4);
			memcpy(start_block_addr+block_array[j]*4,&hex,4);
			//memcpy(&temp_content, start_block_addr+block_array[j]*4,4);
			//printf("169 in entry %d: %d\n",block_array[j],htonl(temp_content));
		}else{
			//int hex = htons(block_array[j+1]);
			memcpy(start_block_addr+block_array[j]*4,&end,4);
			memcpy(&temp_content, start_block_addr+block_array[j]*4,4);
			//printf("169 in entry %d: %d\n",block_array[j],temp_content);
		}
	}
	return block_array;
}
/*
according to allocated blocks, copy content of file in those blocks
*/
void put_file_in_fs(int* block_array, int block_size, void* address, char* filename){
	void* temp_addr = address;
	int* temp_arr = block_array;
	FILE* fp = fopen(filename,"r");
	char content_in_file[512];
	//fread(content_in_file,sizeof(char),512,fp);
	int i=0;
	while(1){
		if(temp_arr[i]==-1){
			break;
		}
		fread(content_in_file,sizeof(char),512,fp);
		//printf("199:we are going to write in block %d\n",temp_arr[i]);
		memcpy(temp_addr+temp_arr[i]*block_size,content_in_file,512);
		
		i++;
		
	}
	fclose(fp);
	
}
/*
find free entry in root dir, fill the information of new file into it
*/
void update_root_dir(void* root_dir_address, int entries_in_root,int file_start_block, int file_size, int block_count,char* new_filename){
	void* start_block_of_root_dir = root_dir_address;
	struct dir_entry_t det;
	time_t T= time(NULL);
    struct  tm tm = *localtime(&T);
	det.status=3;
	det.starting_block=ntohl(file_start_block);
	det.block_count=ntohl(block_count);
	det.size=ntohl(file_size);
	// printf("System Date is: %02d/%02d/%04d\n",tm.tm_mday, tm.tm_mon+1, tm.tm_year+1900);
    //printf("System Time is: %02d:%02d:%02d\n",tm.tm_hour, tm.tm_min, tm.tm_sec);
	det.create_time.year=ntohs(tm.tm_year+1900);
	det.create_time.month = tm.tm_mon+1;
	det.create_time.day = tm.tm_mday;
	det.create_time.hour = tm.tm_hour;
	det.create_time.minute = tm.tm_min;
	det.create_time.second = tm.tm_sec;
	det.modify_time.year=ntohs(tm.tm_year+1900);
	det.modify_time.month = tm.tm_mon+1;
	det.modify_time.day = tm.tm_mday;
	det.modify_time.hour = tm.tm_hour;
	det.modify_time.minute = tm.tm_min;
	det.modify_time.second = tm.tm_sec;
	char temp_name[31];
	strcpy(temp_name,new_filename);
	strcpy((char *)det.filename,temp_name);
	//strcpy((char *)det.unused,"ffffff");
	//det->filename= new_filename;
	//det->unused="ffffff";
	void* start_of_valid_entry;
	int i;
	for(i=0;i<entries_in_root;i++){
		struct dir_entry_t* det1;
		start_of_valid_entry = start_block_of_root_dir+64*i;
		det1 = (struct dir_entry_t*) start_of_valid_entry;
		if(det1->status==0){
			break;
		}
	}
	memcpy(start_of_valid_entry,&det,64);
	long int unused = 0xffffffffff00;
	memcpy(start_of_valid_entry+58,&unused,6);
	
}
int main(int argc, char* argv[]) {
	//printf("%s\n",argv[0]);
	//char *file = argv[1];
	//char *subdir = argv[2];
	char* img_file = argv[1];
	//printf("%s\n",file);
	int fd = open(img_file, O_RDWR);
    //int fd = open("test.img", O_RDWR);
    struct stat buffer;
    int status = fstat(fd, &buffer);
    void* address=mmap(NULL, buffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    struct superblock_t* sb;
    sb=(struct superblock_t*)address;
    /*
    *point to start of root directory
    */
    int blocks_in_fat = ntohl(sb->fat_block_count);
    int block_size = ntohs(sb->block_size);
    void* start_block_of_root_dir = address+ntohs(sb->block_size)*ntohl(sb->root_dir_start_block);
    //struct dir_entry_t* det;
    //det = (struct dir_entry_t*)start_block_of_root_dir;
    //printf("blocks in root dir: %d\n",ntohl(sb->root_dir_block_count));
    int entries_in_rootDir = (ntohl(sb->root_dir_block_count)*ntohs(sb->block_size))/64;
	//int *result = get_info_in_root(64, start_block_of_root_dir,user_file);
	//if(result[0]<0){
	//	printf("file not found\n");
	//	return 0;
	//}
	//printf("file found and start entry in FAT is:%d\n",result[0]);
	int FAT_start = ntohl(sb->fat_start_block);
	//printf("FAT starts at block:%d\n",FAT_start);
	void* fat_start = address+ntohs(sb->block_size)*FAT_start;
	/*
	file will be copied to file system
	*/
	if(argv[2]==NULL){
		printf("Source file name required\n");
		exit(0);
	}
	char* source_file = argv[2];
	int fd2 = open(source_file, O_RDWR);
	if(fd2<0){
		printf("Source file not exists\n");
		return 0;
	}
	struct stat buf;
	fstat(fd2, &buf);
	int file_size = buf.st_size;
	//printf("size of abcde.txt in byte is:%d\n",file_size);
	int number_of_request_blocks = file_size/block_size;
	//printf("%d blocks needed for this file\n",number_of_request_blocks);
	
	int* test_temp_blocks = find_request_blocks(fat_start, file_size, blocks_in_fat,block_size);
	int k=0;
	while(1){
		if(test_temp_blocks[k]==-1){
			break;
		}
		//printf("230: %d\n",test_temp_blocks[k]);
		k++;
	}
	//source file
	//dest file
	if(argv[3]==NULL){
		printf("Destination file name required\n");
		exit(0);
	}
	/*tokenize userinput by '/'*/
    int i = 0;
    char *p = strtok (argv[3], "/");
    char *array[10];
    while (p != NULL)
    {
        array[i++] = p;
        p = strtok (NULL, "/");
    }
	char* dest_file = array[0];
	/****************************/
	
	put_file_in_fs(test_temp_blocks,block_size,address,source_file);
	int file_start_block = test_temp_blocks[0];
	update_root_dir(start_block_of_root_dir,entries_in_rootDir,file_start_block,file_size,number_of_request_blocks,dest_file);
    munmap(address,buffer.st_size);
    close(fd);
    return 0;
}

