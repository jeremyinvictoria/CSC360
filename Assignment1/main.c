//
//  main.c
//  C_test
//
//  Created by Jeremy Zhang on 2018/1/17.
//  Copyright © 2018年 Jeremy Zhang. All rights reserved.
//
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
typedef struct bg_pro{
    pid_t pid;
    char *name;
    struct bg_pro* next;
}bg_pro;
bg_pro* Root = NULL;
void print_list() {
    bg_pro * current = Root;
    int count = 0;
    while (current != NULL) {
        count++;
        printf("%d: %s\n", current->pid,current->name);
        current = current->next;
    }
    printf("Total Background jobs: %d\n",count);
}
void append(pid_t id,char *name){
    bg_pro *new = malloc(sizeof(bg_pro));
    new->pid=id;
    new->name = name;
    if(Root==NULL){
        Root = new;
        Root->next=NULL;
    }else{
        bg_pro *cur = Root;
        while(cur->next!=NULL){
            cur = cur->next;
        }
        cur->next = new;
        cur->next->next=NULL;
    }
}
void remove_node(pid_t n){
	bg_pro *cur = Root;
	bg_pro *temp = NULL;
	while(cur!=NULL){
		if(cur->pid == n){
			if(cur == Root){
				Root = Root->next;
			}else{
				temp->next = cur->next;
			}
			free(cur);
			return;
		}
		temp = cur;
		cur = cur->next;
	}
}
char *concat(char** names){
    char *result = malloc(sizeof(100));
    int i=1;
    strcpy(result, names[1]);
    i=2;
    while(names[i]!=NULL){
        strcat(result, names[i]);
        strcat(result, " ");
        i++;
    }

    return result;
}
int main(int argc, const char * argv[]) {
    char buff[1024];
    static char user_input[1024];
    while (1) {
    	
        getcwd(buff, sizeof(buff));
        printf("SSI: %s > ",buff);
        gets(user_input);
        /*if(user_input[0]==NULL){
        	continue;
        }*/
        /**********tokenize user input**********/
        char *argv[100];
        argv[0] = strtok(user_input," ");
        int i=0;
        while(argv[i]!=NULL){
            argv[i+1] = strtok(NULL, " ");
            i++;
        }
        if(argv[0]==NULL){
        	continue;
        }
        //printf("%s\n",argv[0]);
        if(strcmp(argv[0],"exit")==0){
        	exit(0);
        }
        if(strcmp(argv[0],"bglist")==0){
            print_list();
        }
        if(strcmp(argv[0],"bg")==0){
            pid_t pid = fork();
            if(pid==0){//in child
            	/*for(int i=0;i<3;i++){
            		//printf("Child Process: This is child process\n");
            		sleep(2);
                }*/
                
                if(execvp(argv[1], &argv[1])==-1){
                	perror("exec");
                }
                return(0);
                
            }else if(pid>0){//in parent process
            	printf("Child process id is %d\n",pid);
            	char *command = concat(argv);
            	append(pid, command);
            	//pid_t ter = waitpid(0,NULL,WNOHANG);
            	
            	/*if(ter>0){
            		remove_node(ter);
                	printf("Parent Process: Child Process with id %d Complete\n",ter);
                }*/
            }else{
                fprintf(stderr,"fork() failed\n");
            }
        }
        if(strcmp(argv[0],"bglist")!=0 && strcmp(argv[0],"bg")!=0 && strcmp(argv[0],"cd")!=0){
        	pid_t pid = fork();
        	if(pid<0){
            	fprintf(stderr,"fork() failed!\n");
        	}else if(pid>0){//parent process
            	waitpid(0,NULL,WNOHANG);        
        	}else{//child process
            	if(execvp(argv[0], argv)==-1){
            		perror("exec");
            	}
            	return(0);
            
        	}
        
        }
        char home_dir[strlen(getenv("HOME"))];
        strcpy(home_dir,getenv("HOME"));
        /****************/
        if(strcmp(argv[0], "cd")==0){
            if(argv[1]==NULL||strcmp(argv[1],"~")==0){
            	chdir(home_dir);
            }
            chdir(argv[1]);
        } 
        if(Root!=NULL){
    		pid_t ter = waitpid(0,NULL,WNOHANG);
    		while (ter>0){
    			if(ter>0){
    				remove_node(ter);
    				printf("Child Process with p_id %d has terminated\n",ter);
    			}
    			ter = waitpid(0,NULL,WNOHANG);
    		}
    	}
    }
    
    return 0;
}
