#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WEIGHTED_ROUND_ROBIN "RR"
#define DEFICIT_ROUND_ROBIN "DRR"
#define MAX_LINE_LEN 100
#define MAX_WORDS_IN_LINE 8
#define PKT_ID 0
#define TIME 1
#define S_ADD 2
#define S_PORT 3
#define D_ADD 4
#define D_PORT 5
#define LENGTH 6
#define WEGIHT 7


struct PKT_Params {
	unsigned int Sport, Dport, weight, length;
	char *Sadd, *Dadd;
	long int pktID, Time;
};

struct Node {
	struct Node* next;
	long pktID;
	long time;
	int length;
};

struct Queue {
	struct Node* head;
	struct Node* tail;
	struct Queue* next_queue;
	unsigned int Sport;
	unsigned int Dport;
	char *Sadd;
	char *Dadd;
	int weight;

};

struct Master_Queue {
	struct Queue* head;
	struct Queue* tail;
};

int retrieve_arguments(char* scheduler_type, char* input_file, char* output_file, int* default_weight, int* quantum, char** argv);
void enqueue(struct PKT_Params *pkt_params);
int dequeue();
struct Queue* search_flow(struct PKT_Params *pkt_params);
struct Queue* allocate_queue(struct PKT_Params *pkt_params);
struct Node* allocate_node(struct PKT_Params *pkt_params);
void invoke_WRR_scheduler(FILE *input_fp, FILE *output_fp, int default_weight);
void invoke_DRR_scheduler();
int read_line(struct PKT_Params *pkt_params, FILE *input_fp, int default_weight);
void serve_packet(int* queue_serve_count);


struct Master_Queue master_queue;
long local_time = 0;

int main(int argc, char** argv) {
	char* scheduler_type=NULL; char* input_file=NULL; char* output_file=NULL; int default_weight; int quantum;
	FILE *input_fp, *output_fp;
	if (argc != 6) {
		printf("Wrong number of arguments provided!\n");
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

struct Queue* allocate_queue(struct PKT_Params *pkt_params) {
	struct Queue* queue = (struct Queue*) malloc(sizeof(struct Queue));
	queue->Sadd = pkt_params->Sadd;
	queue->Sport = pkt_params->Sport;
	queue->Dadd = pkt_params->Dadd;
	queue->Dport = pkt_params->Dport;
	queue->weight = pkt_params->weight;
	return queue;
}

struct Node* allocate_node(struct PKT_Params *pkt_params) {
	struct Node* node = (struct Node*)malloc(sizeof(struct Node));
	node->pktID = pkt_params->pktID;
	node->time = pkt_params->Time;
	node->length = pkt_params->length;
	node->next = NULL;
	return node;
}


void enqueue(struct PKT_Params *pkt_params) { /* enqueue to the end of the queue*/
	struct Queue *to_insert_queue;
	struct Node *pkt_node = allocate_node(pkt_params);

	if (master_queue.head == NULL) {
		master_queue.head = allocate_queue(pkt_params);
		master_queue.tail = master_queue.head;
	}
	if ( (to_insert_queue = search_flow(pkt_params)) == NULL) {
		/*new flow type*/
		to_insert_queue = allocate_queue(pkt_params);
	}
	to_insert_queue->tail->next = pkt_node;
	to_insert_queue->tail = to_insert_queue->tail->next;
	if (to_insert_queue->head == NULL) {
		/*first element in queue*/
		to_insert_queue->head = to_insert_queue->tail;
	}
	return;
}

int dequeue() {
	struct Node *tmp_node;
	struct Queue *tmp_queue;
	int ret = 0;

	tmp_node = master_queue.head->head;

	if (master_queue.head->head != master_queue.head->tail) {
		master_queue.head->head = master_queue.head->head->next;
	}
	else {
		tmp_queue = master_queue.head;
		master_queue.head = master_queue.head->next_queue;
		free(tmp_queue);
		ret = 1;
	}
	free(tmp_node);
	return ret;
}

struct Queue* search_flow(struct PKT_Params *pkt_params) {
	struct Queue *curr_queue = master_queue.head;

	while (curr_queue != NULL) {
		if ( strcmp(curr_queue->Dadd, pkt_params->Dadd) == 0 &&
		curr_queue->Dport == pkt_params->Dport &&
		strcmp(curr_queue->Sadd, pkt_params->Sadd) == 0 &&
		curr_queue->Sport == pkt_params->Sport )
			return curr_queue;
	}
	return NULL;
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




void invoke_WRR_scheduler(FILE *input_fp, FILE * output_fp, int default_weight) {

	int queue_serve_count = 0;
	long int old_local_time = local_time;
	struct PKT_Params pkt_params;

	while (read_line(&pkt_params, input_fp, default_weight) == 0) {
		while (pkt_params.Time >= local_time) {
			if (old_local_time != local_time) {
				queue_serve_count = 0;
				old_local_time = local_time;
			}	
			serve_packet(&queue_serve_count);
		}
		enqueue(&pkt_params); /* if the right queue exists, add the node. else, make new queue, add the node*/
	}
	 /* when finished serving packet, read lines and enqueue until local_time>=time_of_packet*/
	/* in case scheudler can send few packets, hwo to handle that case.*/
	/* hold a counter, in case of equality, check who has minimum counter. every packet arrive increase counter*/

}

int read_line(struct PKT_Params *pkt_params, FILE *input_fp, int default_weight) {

	char line[MAX_LINE_LEN];
	int i = 0;
	char *tempWord;

	if (fgets(line, MAX_LINE_LEN, input_fp) == NULL) {
		return -1;
	}

	tempWord = strtok(line, " \t\r\n");
	while (tempWord != NULL && i < MAX_WORDS_IN_LINE) {
		switch (i)
		{
		case PKT_ID:
			pkt_params->pktID = strtoul(tempWord, NULL, 10);
			break;
		case TIME:
			pkt_params->Time = strtoul(tempWord, NULL, 10);
			break;
		case S_ADD:
			pkt_params->Sadd = (char*) tempWord;
			break;
		case S_PORT:
			pkt_params->Sport = (int)strtoul(tempWord, NULL, 10);
			break;
		case D_ADD:
			pkt_params->Dadd = (char*)strtoul(tempWord, NULL, 10);
			break;
		case D_PORT:
			pkt_params->Dport = (int)strtoul(tempWord, NULL, 10);
			break;
		case LENGTH:
			pkt_params->length = (int)strtoul(tempWord, NULL, 10);
			break;
		case WEGIHT:
			pkt_params->weight = (int)strtoul(tempWord, NULL, 10);
			break;
		}
		tempWord = strtok(NULL, " \t\r\n");
		i++;
	}
	if (i == MAX_WORDS_IN_LINE) {
		pkt_params->weight = default_weight;
	}
	return 0;
}


void serve_packet(int* queue_serve_count) {
	int fin_queue;

	if (master_queue.head == NULL)
		return;
	
	/*write here to output file*/
	fin_queue = dequeue();
	(*queue_serve_count)++;
	if (*queue_serve_count == master_queue.head->weight || fin_queue) {
		*queue_serve_count = 0;
		master_queue.tail = master_queue.head;
		master_queue.head = master_queue.head->next_queue;
	}

}