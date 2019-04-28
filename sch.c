#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WEIGHTED_ROUND_ROBIN "RR"
#define DEFICIT_ROUND_ROBIN "DRR"
#define MAX_LINE_LEN 100
#define MAX_WORDS_IN_LINE 8
#define IP_ADDR_LEN 16
#define PKT_ID 0
#define TIME 1
#define S_ADD 2
#define S_PORT 3
#define D_ADD 4
#define D_PORT 5
#define LENGTH 6
#define WEGIHT 7
#define QUEUE_NOT_FIN 0
#define QUEUE_FIN 1
#define LINE_NOT_READ 0
#define LINE_READ 1
#define EMPTY_QUEUE 0
#define NOT_EMPTY_QUEUE 1





struct PKT_Params {
	unsigned int Sport, Dport, weight, length;
	char Sadd[MAX_LINE_LEN], Dadd[MAX_LINE_LEN];
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
	char Sadd[IP_ADDR_LEN];
	char Dadd[IP_ADDR_LEN];
	int weight;

};

struct Master_Queue {
	struct Queue* head;
	struct Queue* tail;
};

int retrieve_arguments(char** scheduler_type, char** input_file, char** output_file, int* default_weight, int* quantum, char** argv);
void enqueue(struct PKT_Params *pkt_params);
int dequeue();
struct Queue* search_flow(struct PKT_Params *pkt_params);
struct Queue* allocate_queue(struct PKT_Params *pkt_params);
struct Node* allocate_node(struct PKT_Params *pkt_params);
void invoke_WRR_scheduler(FILE *input_fp, FILE *output_fp, int default_weight);
void invoke_DRR_scheduler();
int read_line(struct PKT_Params *pkt_params, FILE *input_fp, int default_weight, int *was_enqueued);
int serve_packet(int* queue_serve_count, FILE *output_fp, int *curr_queue_bytes_sent);
void write_line_to_output(FILE *output_fp);


struct Master_Queue master_queue;
long local_time = 0;

int main(int argc, char** argv) {
	char* scheduler_type=NULL; char* input_file=NULL; char* output_file=NULL; int default_weight; int quantum;
	FILE *input_fp, *output_fp;
	if (argc != 6) {
		printf("Wrong number of arguments provided!\n");
		return 1;
	}
	retrieve_arguments(&scheduler_type, &input_file, &output_file, &default_weight, &quantum, argv);
	input_fp = fopen(input_file, "r"); output_fp = fopen(output_file, "w");
	if (strcmp(scheduler_type, WEIGHTED_ROUND_ROBIN) == 0) {
		invoke_WRR_scheduler(input_fp, output_fp, default_weight);
	}
	else if (strcmp(scheduler_type, DEFICIT_ROUND_ROBIN) == 0) {
		invoke_DRR_scheduler();
	}
	fclose(input_fp);
	fclose(output_fp);
	return 0;
}

struct Queue* allocate_queue(struct PKT_Params *pkt_params) {
	struct Queue* queue = (struct Queue*) malloc(sizeof(struct Queue));
	strcpy(queue->Sadd, pkt_params->Sadd);
	queue->Sport = pkt_params->Sport;
	strcpy(queue->Dadd, pkt_params->Dadd);
	queue->Dport = pkt_params->Dport;
	queue->weight = pkt_params->weight;
	queue->head = NULL;
	queue->tail = NULL;
	queue->next_queue = NULL;
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

	/*insert queue (flow)*/
	if (master_queue.head == NULL) {
		master_queue.head = allocate_queue(pkt_params);
		master_queue.tail = master_queue.head;
		master_queue.head->next_queue = master_queue.tail;
		to_insert_queue = master_queue.tail;
		master_queue.tail->next_queue = master_queue.head;
	}
	else if ((to_insert_queue = search_flow(pkt_params)) == NULL) {
		/*new flow type*/
		to_insert_queue = allocate_queue(pkt_params);
		to_insert_queue->next_queue = master_queue.head;
		master_queue.tail->next_queue = to_insert_queue;
		master_queue.tail = to_insert_queue;
	}
	
	
	/*insert node*/
	if (to_insert_queue->head == NULL) {
		/*first element in queue*/
		to_insert_queue->head = pkt_node;
		to_insert_queue->head->next = to_insert_queue->tail;
		to_insert_queue->tail = to_insert_queue->head;
	}
	else {
		to_insert_queue->tail->next = pkt_node;
		to_insert_queue->tail = to_insert_queue->tail->next;
	}
	return;
}

int dequeue() {
	struct Node *tmp_node;
	struct Queue *tmp_queue;
	int ret = QUEUE_NOT_FIN;

	tmp_node = master_queue.head->head;

	if (master_queue.head->head != master_queue.head->tail) {
		master_queue.head->head = master_queue.head->head->next;
	}
	else {
		tmp_queue = master_queue.head;
		if (master_queue.head == master_queue.tail) {
			master_queue.head = NULL;
		}
		else {
			master_queue.head = master_queue.head->next_queue;
			master_queue.tail->next_queue = master_queue.head;
		}
		free(tmp_queue);
		ret = QUEUE_FIN;

	}

	free(tmp_node);
	return ret;
}

struct Queue* search_flow(struct PKT_Params *pkt_params) {
	struct Queue *curr_queue = master_queue.head;

	while (1) {
		if ( strcmp(curr_queue->Dadd, pkt_params->Dadd) == 0 &&
		curr_queue->Dport == pkt_params->Dport &&
		strcmp(curr_queue->Sadd, pkt_params->Sadd) == 0 &&
		curr_queue->Sport == pkt_params->Sport )
			return curr_queue;
		curr_queue = curr_queue->next_queue;
		if (curr_queue == master_queue.head) { break; }
	}
	return NULL;
}


int retrieve_arguments(char** scheduler_type, char** input_file, char** output_file, int* default_weight, int* quantum, char** argv) {
	*scheduler_type = argv[1];
	*input_file = argv[2];
	*output_file = argv[3];
	if ((*default_weight = atoi(argv[4])) == 0) {
		printf("Bad default weight!\n");
		exit(1);
	}
	*quantum = atoi(argv[5]);
	return 0;
}


void invoke_WRR_scheduler(FILE *input_fp, FILE *output_fp, int default_weight) {

	int queue_serve_count = 0, curr_queue_bytes_sent = 0, read_line_value, queue_state, was_enqueued = 1;
	long int old_local_time = local_time;
	struct PKT_Params pkt_params;

	while (1) {	
		while (was_enqueued && (read_line_value = read_line(&pkt_params, input_fp, default_weight, &was_enqueued))
			&& pkt_params.Time <= local_time) {
			enqueue(&pkt_params);
			was_enqueued = 1;
		}

		if (!was_enqueued && pkt_params.Time <= local_time && read_line_value){
			enqueue(&pkt_params);
			was_enqueued = 1;
		}
		queue_state = serve_packet(&queue_serve_count, output_fp, &curr_queue_bytes_sent);
		if (!queue_state && !read_line_value) {
			break;
		}
	}
}

int read_line(struct PKT_Params *pkt_params, FILE *input_fp, int default_weight, int *was_enqueued) {

	char line[MAX_LINE_LEN];
	int i = 0;
	char *tempWord;

	if (fgets(line, MAX_LINE_LEN, input_fp) == NULL) {
		return LINE_NOT_READ;
	}

	*was_enqueued = 0;
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
			strcpy(pkt_params->Sadd, tempWord);
			break;
		case S_PORT:
			pkt_params->Sport = (int)strtoul(tempWord, NULL, 10);
			break;
		case D_ADD:
			strcpy(pkt_params->Dadd, tempWord);
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
	if (i < MAX_WORDS_IN_LINE) {
		pkt_params->weight = default_weight;
	}
	return LINE_READ;
}


int serve_packet(int* queue_serve_count, FILE *output_fp, int *curr_queue_bytes_sent) {
	int queue_fin;

	if (master_queue.head == NULL) {
		local_time++;
		return EMPTY_QUEUE;
	}
		
	if (master_queue.head->head->length > *curr_queue_bytes_sent) {
		(*curr_queue_bytes_sent)++;
		local_time++;
		return NOT_EMPTY_QUEUE;
	}
	if (master_queue.head == NULL && local_time < 1057718) {
		printf("NULL\n");
	}
	else if (local_time < 1057718) {
		printf("TIME: %d, master queue tail->next flow: %s, %d, %s, %d\n",
			local_time, master_queue.tail->next_queue->Sadd, master_queue.tail->next_queue->Sport, master_queue.tail->next_queue->Dadd, master_queue.tail->next_queue->Dport);
	}
	
	write_line_to_output(output_fp);
	queue_fin = dequeue();
	*curr_queue_bytes_sent = 0;
	(*queue_serve_count)++;
	if(queue_fin)
		*queue_serve_count = 0;
	else if (*queue_serve_count == master_queue.head->weight) {
		master_queue.tail = master_queue.head;
		master_queue.head = master_queue.head->next_queue;
		*queue_serve_count = 0;
	}

	return NOT_EMPTY_QUEUE;
}

void write_line_to_output(FILE *output_fp) {
	char line[MAX_LINE_LEN] = { 0 };
	long int length = master_queue.head->head->length;
	long int pkt_id = master_queue.head->head->pktID;
	int bytes_wrote;

	sprintf(line, "%ld: %ld\n", local_time - length, pkt_id);
	bytes_wrote = fwrite(line, sizeof(char), strlen(line), output_fp);
	if (bytes_wrote <= 0) {
		printf("Error writing to output file. exiting... \n");
		exit(1);
	}
	return;
}

void invoke_DRR_scheduler() {

}