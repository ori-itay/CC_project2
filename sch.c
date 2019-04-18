#include <stdio.h>

struct Node {
	struct Node* next;
	long pktID;
	long time;

};

struct Queue {
	struct Node* next;
	unsigned int Sport;
	unsigned int Dport;
	char *Sadd;
	char *Dadd;
	int length;
	int weight;

};

struct Master_Queue {
	struct Queue* head;
};

int retrieve_arguments(char* scheduler_type, char* input_file, char* output_file, int* default_weight, int* quantum, char** argv);
void enqueue(struct Node* node);
struct Queue* allocate_queue(char* Sadd, unsigned int Sport, char *Dadd, unsigned int Dport, int length, int weight);
struct Node* allocate_node(long pktID, long time);


struct Master_Queue master_queue;

int main(int argc, char** argv) {
	char* scheduler_type;
	char* input_file; char* output_file; int default_weight; int quantum;
	if (argc != 6) {
		printf("Wrong number of arguments provided!")
		return 1;
	}
	retrieve_arguments(scheduler_type, input_file, output_file, &default_weight, &quantum, argv);

	
	return 0;
}

struct Queue* allocate_queue(char* Sadd, unsigned int Sport, char *Dadd, unsigned int Dport, int length, int weight) {
	struct Queue* queue = (struct Queue*) malloc(sizeof(struct Queue));
	queue->Sadd = Sadd;
	queue->Sport = Sport;
	queue->Dadd = Dadd;
	queue->Dport = Dport;
	queue->length = length;
	queue->weight = weight;
}

struct Node* allocate_node(long pktID, long time) {
	struct Node* node = (struct Node*)malloc(sizeof(struct Node));
	node->pktID = pktID;
	node->time = time;
	return node;
}


void enqueue(struct Node* node) {
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
}