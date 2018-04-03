#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct t{
	int num;
	char dir;
    int load_time;
    int travel_time;
    //int p;
    double finish_loading_time;
    struct t* next;
}t;
t* head=NULL;
t* station_head=NULL;

int priority=1;
int count_for_thread_creation=0;
int trains_in_station=0;
char trains[256][256];
//pthread_cond_t count_cond 

pthread_cond_t wait_loading_signal_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t loading_signal_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t station_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t train_list_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t last_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t disp_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t disp_cond = PTHREAD_COND_INITIALIZER;
/*
read file & store trains in a two-dimension array
*/
void read_file(char* name){
	FILE* file = fopen(name,"r");
	char line[256];
	int i=0;
	while(fgets(line,sizeof(line),file)){
		//printf("%s",line);
		strcpy(trains[i],line);
		i++;
	}
	strcpy(trains[i],"end");
	fclose(file);
}
void print_list(t* h) {
    t* current = h;
    int count = 0;
    while (current != NULL) {
        count++;
        printf("%d: %c, %d, %d\n", current->num,current->dir,current->load_time,current->travel_time);
        current = current->next;
    }
}
/*
get size of linked-list/# of trains
*/
int get_size(t* h){
    t* current = h;
    int count = 0;
    while (current != NULL) {
        count++;
        current = current->next;
    }
    return count;
}
struct t *deleteHead(t *h){
	if(h!=NULL){
		t* temp = h;
		h = h->next;
	}
	return h;
}
void deleteTrain(t** h, int num){
	t* temp = *h,*prev;
	if(temp!=NULL&&temp->num==num){
		*h = temp->next;
		free(temp);
		return;
	}
	while(temp!=NULL && temp->num != num){
		prev = temp;
		temp = temp->next;
	}
	if(temp==NULL) return;
	prev->next = temp->next;
	free(temp);
}
struct t *append(int num,char dir,int load_time,int travel_time,double finish_loading_time,t* h){
    t *new = malloc(sizeof(t));
    new->num=num;
    new->dir = dir;
    new->load_time = load_time;
    new->travel_time = travel_time;
    new->finish_loading_time=finish_loading_time;
    new->next=NULL;
    if(h==NULL){
        h = new;
    }else{
        t *cur = h;
        while(cur->next!=NULL){
            cur = cur->next;
        }
        cur->next = new;
    }
    return h;
}
/*
extract each train from two-Dimension array to generate a train linked-list
*/
void generate_train_list(char arr[256][256]){
	int j=0;
	while(strcmp(arr[j],"end")!=0){
		char *argv[100];
        argv[0] = strtok(arr[j]," ");
        int i=0;
        while(argv[i]!=NULL){
        	//printf("%s\n",argv[i]);
            argv[i+1] = strtok(NULL, " ");
            i++;
        }
        int load_time = atoi(argv[1]);
        int travel_time = atoi(argv[2]);
        char dir = argv[0][0];
        head = append(j,dir,load_time,travel_time,0,head);
        //print_list(head);
        j++;
	}
}
/*compare with trains which have same loading time at train station*/
void comp_load_time(){
	t* current=station_head;
	while(current!=NULL){
		if(current->next!=NULL){
			if(current->finish_loading_time==current->next->finish_loading_time){
				if((current->dir=='e'||current->dir=='w')&&(current->next->dir=='E'||current->next->dir=='W')){
					int temp_num = current->num;
					char temp_dir = current->dir;
					int temp_load = current->load_time;
					int temp_travel = current->travel_time;
					int temp_p = current->finish_loading_time;
					current->num=current->next->num;
					current->dir = current->next->dir;
					current->load_time = current->next->load_time;
					current->travel_time = current->next->travel_time;
     				current->finish_loading_time = current->next->finish_loading_time;
     				current->next->num=temp_num;
     				current->next->dir=temp_dir;
     				current->next->load_time = temp_load;
     				current->next->travel_time= temp_travel;
     				current->next->finish_loading_time = temp_p;
     			}
			}
		}
		current = current->next;
	}
}
void comp_num(){
	t* current=station_head;
	while(current!=NULL){
		if(current->next!=NULL){
			if(current->finish_loading_time==current->next->finish_loading_time){
				if((current->dir=='E'||current->dir=='W')&&(current->next->dir=='E'||current->next->dir=='W')){
					if(current->num>current->next->num){
						int temp_num = current->num;
						char temp_dir = current->dir;
						int temp_load = current->load_time;
						int temp_travel = current->travel_time;
						int temp_p = current->finish_loading_time;
						current->num=current->next->num;
						current->dir = current->next->dir;
						current->load_time = current->next->load_time;
						current->travel_time = current->next->travel_time;
     					current->finish_loading_time = current->next->finish_loading_time;
     					current->next->num=temp_num;
     					current->next->dir=temp_dir;
     					current->next->load_time = temp_load;
     					current->next->travel_time= temp_travel;
     					current->next->finish_loading_time = temp_p;
     				}
     			}
			}
		}
		current = current->next;
	}
}
/*get milliseconds*/
double get_timestamp(double result){
    int new_result = (int)(result*10);
    double result1 = (double)(new_result/10.);
    return result1;
}
/*
thread method
*/
void* thread_function(void *arguments){
	pthread_mutex_lock(&count_mutex);
	count_for_thread_creation++;
	//printf("count for thread creation: %d\n",count_for_thread_creation);
	pthread_mutex_unlock(&count_mutex);
	struct timespec tstart={0,0}, tend={0,0};
	t* train_thread = arguments;
	
	int train_num = train_thread->num;
	char dir = train_thread->dir;
	int load_time = train_thread->load_time;
	int travel_time = train_thread->travel_time;
	/*Waiting for broadcast signal*/
	pthread_mutex_lock(&loading_signal_mutex);
	pthread_cond_wait(&wait_loading_signal_cond,&loading_signal_mutex);
	pthread_mutex_unlock(&loading_signal_mutex);
	/*Start loading*/
	clock_gettime(CLOCK_MONOTONIC, &tstart);
    usleep(load_time*100000);
    /*
    remove loaded train from train list
    */
    clock_gettime(CLOCK_MONOTONIC, &tend);
    //pthread_mutex_lock(&train_list_mutex);
    
    //pthread_mutex_unlock(&train_list_mutex);
    
    double result =((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) -((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec);
    double new_result = get_timestamp(result);
    int hours=0;
	int minutes=0;

    printf("%02d:%02d:%04.1f train %d finishes loading\n",hours,minutes,new_result,train_num);
    /*
    finishes loading, add to queue, wait for getting dispatched 
    */
    pthread_mutex_lock(&station_mutex);
    deleteTrain(&head,train_num);
    station_head = append(train_num,dir,load_time,travel_time,new_result,station_head);
    if(station_head->next!=NULL){
    	comp_load_time();
    	comp_num();
    }
    trains_in_station++;
    //printf("there are %d trains in station currently.\n",trains_in_station);
    pthread_mutex_unlock(&station_mutex);
    pthread_mutex_lock(&last_mutex);
    count_for_thread_creation--;
    pthread_cond_signal(&disp_cond);
    pthread_mutex_unlock(&last_mutex);
    
    
	pthread_exit(NULL);
}

void print_arr(char arr[256][256]){
	int j=0;
	while(strcmp(arr[j],"end")!=0){
		printf("%s",arr[j]);
		j++;
	}
}
int main(int argc, const char * argv[]) {
	pthread_t east_station;
	pthread_t west_station;
	char *east = "east";
	char *west = "west";
	//pthread_create(&east_station,NULL,station_function,(void*)east);
	//pthread_create(&west_station,NULL,station_function,(void*)west);
	read_file("trains.txt");
	int j=0;
	while(strcmp(trains[j],"end")!=0){
		j++;
	}
	generate_train_list(trains);
	//print_list(head);
	pthread_t threads[get_size(head)];
    t* current = head;
    int i = 0;
    while (current != NULL) {
        pthread_create(&threads[i],NULL,thread_function,(void*)current);
        i++;
        current = current->next;
    }
    while(count_for_thread_creation<get_size(head)){
    }
	sleep(1);
    pthread_cond_broadcast(&wait_loading_signal_cond);
    
	//printf("main after broadcast %d\n",result);
	/*
	do dispatch thing 
	*/
	//usleep(5*100000);
	//station_head = deleteHead(station_head);
	//print_list(station_head);
	//printf("deleted head number is %d\n",station_head->num);
	while(station_head==NULL){	
	}
	double start_time = station_head->finish_loading_time;
	int hours=0;
	int minutes=0;
	while(1){
		if((head==NULL)&&(station_head==NULL)){
			break;
		}
		struct timespec waitstart={0,0}, waitend={0,0};
		clock_gettime(CLOCK_MONOTONIC, &waitstart);
		while(station_head==NULL){
			pthread_mutex_lock(&disp_mutex);
			pthread_cond_wait(&disp_cond,&disp_mutex);
			pthread_mutex_unlock(&disp_mutex);
		}
		clock_gettime(CLOCK_MONOTONIC, &waitend);
		double waiting_time = ((double)waitend.tv_sec + 1.0e-9*waitend.tv_nsec) -((double)waitstart.tv_sec + 1.0e-9*waitstart.tv_nsec);
		double new_waiting_time = get_timestamp(waiting_time);
		struct timespec tstart={0,0}, tend={0,0};
		//double start_time = station_head->finish_loading_time;
		//printf("train finish loading time is: %f\n",station_head->finish_loading_time);
		int cur_num = station_head->num;
		char cur_dir = station_head->dir;
		char *direction;
		if(cur_dir=='e'||cur_dir=='E'){
			direction="East";
		}else{
			direction="West";
		}
		int cur_travel_time = station_head->travel_time;
		station_head = deleteHead(station_head);
		start_time = start_time+new_waiting_time;
		printf("%02d:%02d:%04.1f Train %d is ON the main track going %s\n",hours,minutes,start_time,cur_num,direction);
		clock_gettime(CLOCK_MONOTONIC, &tstart);
		for(i=0;i<cur_travel_time;i++){
			usleep(1*100000);
		}
		clock_gettime(CLOCK_MONOTONIC, &tend);
    	double result =((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) -((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec);
    	double new_result = get_timestamp(result);
    	start_time = new_result+start_time;
		printf("%02d:%02d:%04.1f Train %d is OFF the main track after going %s\n",hours,minutes,start_time,cur_num,direction);
		//printf("Train %d travel time is %f\n",cur_num,new_result);
	}
	//sleep(3);
	//station_head = deleteHead(station_head);
	//deleteTrain(&station_head,4);
	//printf("after delete num=4 :\n");
	//print_list(station_head);
    //sleep(3);
	return 0;
}
