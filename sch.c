#include <stdio.h>
#include <stdlib.h>

#define WEIGHTED_ROUND_ROBIN "RR"
#define DEFICIT_ROUND_ROBIN "DRR"

struct Node {
	struct Node* next;
	long pktID;
	long time;
	int length;
};

struct Queue {
	struct Node* next;
	unsigned int Sport;
	unsigned int Dport;
	char *Sadd;
	char *Dadd;
	int weight;

};

struct Master_Queue {
	struct Queue* head;
};

int retrieve_arguments(char* scheduler_type, char* input_file, char* output_file, int* default_weight, int* quantum, char** argv);
void enqueue(struct Node* node);
struct Queue* allocate_queue(char* Sadd, unsigned int Sport, char *Dadd, unsigned int Dport, int weight);
struct Node* allocate_node(long pktID, long time, int length);
void invoke_WRR_scheduler(int input_fp, int output_fp, int default_weight);


struct Master_Queue master_queue;
long local_time = 0;

int main(int argc, char** argv) {
	char* scheduler_type=NULL; char* input_file=NULL; char* output_file=NULL; int default_weight; int quantum;
	int input_fp, output_fp;
	if (argc != 6) {
		printf("Wrong number of arguments provided!");
		return 1;
	}
	retrieve_arguments(scheduler_type, input_file, output_file, &default_weight, &quantum, argv);
	input_fp = fopen(input_file, "r"); output_fp = fopen(output_file, "w");
	if (strcmp(scheduler_type, WEIGHTED_ROUND_ROBIN) == 0) {
		invoke_WRR_scheduler(input_fp, output_fp, default_weight);
	}
	else if (strcmp(scheduler_type, DEFICIT_ROUND_ROBIN) == 0) {
		invoke_DRR_scheduler();
	}



	
	return 0;
}

struct Queue* allocate_queue(char* Sadd, unsigned int Sport, char *Dadd, unsigned int Dport, int weight) {
	struct Queue* queue = (struct Queue*) malloc(sizeof(struct Queue));
	queue->Sadd = Sadd;
	queue->Sport = Sport;
	queue->Dadd = Dadd;
	queue->Dport = Dport;
	queue->weight = weight;
	return queue;
}

struct Node* allocate_node(long pktID, long time, int length) {
	struct Node* node = (struct Node*)malloc(sizeof(struct Node));
	node->pktID = pktID;
	node->time = time;
	node->length = length;
	return node;
}


void enqueue(struct Node* node) { /* enqueue to the end of the queue*/
	if (master_queue.head == NULL) {
	
	}
		
}


int retrieve_arguments(char* scheduler_type, char* input_file, char* output_file, int* default_weight, int* quantum, char** argv) {
	scheduler_type = argv[1];
	input_file = argv[2];
	output_file = argv[3];
	if ((*default_weight = atoi(argv[4])) == 0) {
		printf("Bad default weight!\n");
		return 1;
	}
	if ((*quantum = atoi(argv[5])) == 0) {
		printf("Bad quantum!\n");
		return 1;
	}
	return 0;
}




void invoke_WRR_scheduler(int input_fp,int output_fp,int default_weight) {
	read_line();
	enqueue(); /* if the right queue exists, add the node. else, make new queue, add the node*/
	serve_packet(); /* when finished serving packet, read lines and enqueue until local_time>=time_of_packet*/
	/* in case scheudler can send few packets, hwo to handle that case.*/
	/* hold a counter, in case of equality, check who has minimum counter. every packet arrive increase counter*/

}