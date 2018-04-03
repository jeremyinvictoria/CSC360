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
void print_info_in_root(int num_of_entries, void* start_block_of_root_dir){
	int i;
	
	for(i=0;i<num_of_entries;i++){
		struct dir_entry_t* det;
		//struct dir_entry_t* det;
		void* start_of_entry = start_block_of_root_dir+64*i;
		//memcpy(det,start_of_entry,64);
		det = (struct dir_entry_t*) start_of_entry;
		char ftype;
		if(det->status!=0){
			if(det->status==3){
				ftype='F';
			}else if(det->status==5){
				ftype='D';
			}
			printf("%c\t",ftype);
			//printf("status: %d\t",det->status);
			printf("%d\t",ntohl(det->size));
			printf("%s\t",det->filename);
			printf("%d/%d/%d %d:%d:%d\n",ntohs(det->modify_time.year),(det->modify_time.month),(det->modify_time.day),(det->modify_time.hour),(det->modify_time.minute),(det->modify_time.second));
		}
		
	}
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
	char *sub_dir = argv[2];
	//printf("%s\n",file);
	int fd = open(img_file, O_RDWR);
	if(fd<0){
		printf("Invalid img file\n");
		return 0;
	}
    struct stat buffer;
    int status = fstat(fd, &buffer);
    void* address=mmap(NULL, buffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    struct superblock_t* sb;
    sb=(struct superblock_t*)address;
    void* start_block_of_root_dir = address+ntohs(sb->block_size)*ntohl(sb->root_dir_start_block);
    //struct dir_entry_t* det;
    //det = (struct dir_entry_t*)start_block_of_root_dir;
    //printf("blocks in root dir: %d\n",ntohl(sb->root_dir_block_count));
    int entries_in_rootDir = (ntohl(sb->root_dir_block_count)*ntohs(sb->block_size))/64;
    if(strcmp(argv[2],"/")==0){
		print_info_in_root(entries_in_rootDir, start_block_of_root_dir);
	}else{
		printf("Subdirectory not found\n");
		exit(0);
	}
    munmap(address,buffer.st_size);
    close(fd);
    return 0;
}

