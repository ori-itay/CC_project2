#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WEIGHTED_ROUND_ROBIN "RR"
#define DEFICIT_ROUND_ROBIN "DRR"
#define WRR_TYPE 0
#define DRR_TYPE 1
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
#define NON_DEQUEUE_STATE 0
#define DEQUEUE_STATE 1





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
	int drr_credit;
};

struct Master_Queue {
	struct Queue* head;
	struct Queue* tail;
	struct Queue* first_element;
	struct Queue* queue_before_head;
};

int retrieve_arguments(int* scheduler_type_num, char** input_file, char** output_file, int* default_weight, int* quantum, char** argv);
void enqueue(struct PKT_Params *pkt_params);
int dequeue();
void advance_flow(int dequeue_state);
struct Queue* search_flow(struct PKT_Params *pkt_params);
struct Queue* allocate_queue(struct PKT_Params *pkt_params);
struct Node* allocate_node(struct PKT_Params *pkt_params);
void invoke_scheduler(FILE *input_fp, FILE *output_fp, int default_weight, int scheduler_type, int quantum);
int read_line(struct PKT_Params *pkt_params, FILE *input_fp, int default_weight, int *was_enqueued);
int serve_packet(int* queue_serve_count, FILE *output_fp, int *curr_queue_bytes_sent, int scheduler_type_num, int quantum, int was_enqueued);
void write_line_to_output(FILE *output_fp, int scheduler_type_num);
void devide_credits(int quantum);
void update_queue_before_head();
void print_queue();


struct Master_Queue master_queue;
long local_time = 0;

int main(int argc, char** argv) {
	char* input_file = NULL; char* output_file = NULL; int default_weight; int quantum, scheduler_type_num;
	FILE *input_fp, *output_fp;
	if (argc != 6) {
		printf("Wrong number of arguments provided!\n");
		return 1;
	}
	retrieve_arguments(&scheduler_type_num, &input_file, &output_file, &default_weight, &quantum, argv);
	input_fp = fopen(input_file, "r"); output_fp = fopen(output_file, "w");
	invoke_scheduler(input_fp, output_fp, default_weight, scheduler_type_num, quantum);
	fclose(input_fp);
	fclose(output_fp);
	return 0;
}

int retrieve_arguments(int* scheduler_type, char** input_file, char** output_file, int* default_weight, int* quantum, char** argv) {

	if (strcmp(argv[1], WEIGHTED_ROUND_ROBIN) == 0) {
		*scheduler_type = WRR_TYPE;
	}
	else if (strcmp(argv[1], DEFICIT_ROUND_ROBIN) == 0) {
		*scheduler_type = DRR_TYPE;
	}
	*input_file = argv[2];
	*output_file = argv[3];
	if ((*default_weight = atoi(argv[4])) == 0) {
		printf("Bad default weight!\n");
		exit(1);
	}
	*quantum = atoi(argv[5]);
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
	queue->drr_credit = 0;
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
	if (master_queue.first_element == NULL) {
		master_queue.head = allocate_queue(pkt_params);
		master_queue.tail = master_queue.head;
		master_queue.first_element = master_queue.head;
		master_queue.head->next_queue = master_queue.tail;	
		to_insert_queue = master_queue.head;
	}
	else if ((to_insert_queue = search_flow(pkt_params)) == NULL) {
		/*new flow type*/
		to_insert_queue = allocate_queue(pkt_params);
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
	struct Node *tmp_node = master_queue.head->head;
	struct Queue *tmp_queue = tmp_queue = master_queue.head;	
	int queue_fin = QUEUE_NOT_FIN;

	if (master_queue.head->head != master_queue.head->tail) {
		/*advance in the same flow*/
		master_queue.head->head = master_queue.head->head->next;
	}
	else {
		advance_flow(DEQUEUE_STATE);
		free(tmp_queue);
		queue_fin = QUEUE_FIN;
	}
	free(tmp_node);
	return queue_fin;
}

void advance_flow(int dequeue_state) {

	if (master_queue.head == master_queue.first_element->next_queue) {
		master_queue.queue_before_head = master_queue.first_element;
	}

	/*current flow ended*/
	if (master_queue.first_element == master_queue.tail) {
		/*all flows ended*/
		if (dequeue_state) {
			master_queue.first_element = NULL;
			master_queue.head = NULL;
			master_queue.tail = NULL;
		}
	}
	else if (master_queue.head->next_queue != NULL) {
		/*current flow is not the tail flow*/
		if (master_queue.head != master_queue.first_element && dequeue_state) {
			/*current flow is not the first flow*/
			master_queue.queue_before_head->next_queue = master_queue.head->next_queue;
		}
		else if(dequeue_state){
			/*current flow is the first flow*/
			master_queue.first_element = master_queue.first_element->next_queue;
		}
		master_queue.head = master_queue.head->next_queue;
	}
	else {
		/*current flow is the tail flow*/
		if (dequeue_state) {
			master_queue.tail = master_queue.queue_before_head;
			master_queue.tail->next_queue = NULL;
		}
		master_queue.head = master_queue.first_element;
		master_queue.queue_before_head = NULL;
	}
}

struct Queue* search_flow(struct PKT_Params *pkt_params) {
	struct Queue *curr_queue = master_queue.first_element;

	while (1) {
		if ( strcmp(curr_queue->Dadd, pkt_params->Dadd) == 0 &&
		curr_queue->Dport == pkt_params->Dport &&
		strcmp(curr_queue->Sadd, pkt_params->Sadd) == 0 &&
		curr_queue->Sport == pkt_params->Sport )
			return curr_queue;
		curr_queue = curr_queue->next_queue;
		if (curr_queue == master_queue.first_element || curr_queue == NULL) { break; }
	}
	return NULL;
}


void invoke_scheduler(FILE *input_fp, FILE *output_fp, int default_weight, int scheduler_type_num, int quantum) {

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
		update_queue_before_head();
		queue_state = serve_packet(&queue_serve_count, output_fp, &curr_queue_bytes_sent, scheduler_type_num, quantum, was_enqueued);
		if (master_queue.head == NULL) {
			master_queue.head = master_queue.first_element;
			master_queue.queue_before_head = NULL;
		}
		else if (master_queue.head == master_queue.first_element->next_queue) {
			master_queue.queue_before_head = master_queue.first_element;
		}
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


int serve_packet(int* queue_serve_count, FILE *output_fp, int *curr_queue_bytes_sent, int scheduler_type_num, int quantum, int was_enqueued) {
	int is_curr_flow_fin;

	if (master_queue.head == NULL && master_queue.first_element == NULL) {
		local_time++;
		return EMPTY_QUEUE;
	}

	if (scheduler_type_num == DRR_TYPE && (*queue_serve_count) == 0) {
		while (1) {
			if (master_queue.head->head->length <= master_queue.head->drr_credit) {
				(*queue_serve_count)++;
				local_time++;
				break;
			}
			master_queue.head->drr_credit += quantum * master_queue.head->weight;
			master_queue.queue_before_head = master_queue.head;
			advance_flow(NON_DEQUEUE_STATE);
			update_queue_before_head();
			if (master_queue.head == master_queue.first_element) {
				//devide_credits(quantum);
				//break;
			}	
		}
		return NOT_EMPTY_QUEUE;
	}
	else if(scheduler_type_num == DRR_TYPE && (*queue_serve_count) < master_queue.head->head->length){
		(*queue_serve_count)++;
		local_time++;
		return NOT_EMPTY_QUEUE;
	}
	else if (scheduler_type_num == DRR_TYPE) {
		master_queue.head->drr_credit -= master_queue.head->head->length;
		*queue_serve_count = 0;
	}
	else if (scheduler_type_num == WRR_TYPE && master_queue.head->head->length > *curr_queue_bytes_sent) {
		(*curr_queue_bytes_sent)++;
		local_time++;
		return NOT_EMPTY_QUEUE;
	}
	/*
	if (scheduler_type_num == DRR_TYPE && master_queue.head == master_queue.first_element) {
		//full round - increase credits for all flows
		devide_credits(quantum);
	}*/
	//print_queue();
	write_line_to_output(output_fp, scheduler_type_num);
	is_curr_flow_fin = dequeue();
	if (scheduler_type_num == WRR_TYPE) {
		*curr_queue_bytes_sent = 0;
		(*queue_serve_count)++;
	}
	else if (master_queue.head != NULL && master_queue.head->head->length > master_queue.head->drr_credit) {
		master_queue.queue_before_head = master_queue.head;
		advance_flow(NON_DEQUEUE_STATE);
	}

	if(scheduler_type_num == WRR_TYPE && is_curr_flow_fin)
		*queue_serve_count = 0;
	else if (scheduler_type_num == WRR_TYPE && *queue_serve_count == master_queue.head->weight) {
		master_queue.queue_before_head = master_queue.head;
		master_queue.head = master_queue.head->next_queue;
		*queue_serve_count = 0;
	}
	return NOT_EMPTY_QUEUE;
}

void write_line_to_output(FILE *output_fp, int scheduler_type_num) {
	char line[MAX_LINE_LEN] = { 0 };
	long int length = master_queue.head->head->length;
	long int pkt_id = master_queue.head->head->pktID;
	int bytes_wrote, time;

	time = local_time - length;
	sprintf(line, "%ld: %ld\n", time, pkt_id);
	bytes_wrote = fwrite(line, sizeof(char), strlen(line), output_fp);
	if (bytes_wrote <= 0) {
		printf("Error writing to output file. exiting... \n");
		exit(1);
	}
	return;
}

void devide_credits(int quantum) {
	struct Queue *tmp_queue = master_queue.head;

	while (1) {
		master_queue.head->drr_credit += quantum * master_queue.head->weight;
		advance_flow(NON_DEQUEUE_STATE);
		if (master_queue.head == tmp_queue) {
			return;
		}
	}
}

void print_queue() {
	struct Queue *tmp_queue = master_queue.first_element;

	printf("--------------------------------------------------------------\n");
	while (tmp_queue != NULL) {
		printf("time: %d. credit: %d.  queue: %s, %d, %s, %d.next_pkt_id:%d,  next_pkt_len: %d \n",local_time, tmp_queue->drr_credit,
			tmp_queue->Sadd, tmp_queue->Sport, tmp_queue->Dadd, tmp_queue->Dport, tmp_queue->head->pktID,tmp_queue->head->length);
		tmp_queue = tmp_queue->next_queue;
	}
}

void update_queue_before_head() {
	if (master_queue.head != master_queue.first_element &&
		master_queue.queue_before_head->next_queue != master_queue.head) {
		master_queue.queue_before_head = master_queue.first_element;
		while (master_queue.queue_before_head->next_queue != master_queue.head) {
			master_queue.queue_before_head = master_queue.queue_before_head->next_queue;
		}
	}
	else if (master_queue.head == master_queue.first_element)
		master_queue.queue_before_head = NULL;
}