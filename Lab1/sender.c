#include "sender.h"
#include "time.h"

void send(message_t message, mailbox_t* mailbox_ptr) {
    struct timespec start, end;
    
    // Message Passing
    if (mailbox_ptr->flag == 1) {

	// Only measure sending process    
	clock_gettime(CLOCK_MONOTONIC, &start);  
	
	// data = 1 ACK = 2 
	message.mType = 1;
        
	// msgsnd(message queue ID, sending message, message data size, control 0 means queuing
	// if return -1 queue full, stop sending
	if (msgsnd(mailbox_ptr->storage.msqid, &message, sizeof(message.msgText), 0) == -1) {
            exit(1);
        }
	clock_gettime(CLOCK_MONOTONIC, &end);  

	total_time += (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;
	
	if (strcmp(message.msgText, "EOF") != 0) {
            printf("Sending message: %s", message.msgText);
	    
	    // wait ACK, so that send 1 message first time
	    message_t ack;
	    // receive message ACK, if no ACK just wait until receiver return ACK
	    if (msgrcv(mailbox_ptr->storage.msqid, &ack, sizeof(ack.msgText), 2, 0) == -1) {
    	        //exit(1);
	    }
	}
    } 
    // Shared Memory
    else if (mailbox_ptr->flag == 2) {
        // waiting until memory can be write
	sem_wait(sem_receiver);                                  
        
	// Only measure writing into memory process
	clock_gettime(CLOCK_MONOTONIC, &start);
        
	// copy message to shm_addr shared memory
	strcpy(mailbox_ptr->storage.shm_addr, message.msgText); 
	clock_gettime(CLOCK_MONOTONIC, &end); 
        
	// tell receiver can read
	sem_post(sem_sender);                                     
	total_time += (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;

	if (strcmp(message.msgText, "EOF") != 0)
            printf("Sending message: %s", message.msgText);
    }
}

int main(int argc, char* argv[]) {

    int method = atoi(argv[1]);
    char* input_file = argv[2];

    mailbox_t mailbox;
    mailbox.flag = method;
    message_t message;

    key_t key;
    int shmid;

    // Sender = 0 Receiver = 1
    sem_sender = sem_open("/sem_sender", O_CREAT, 0644, 0);      
    sem_receiver = sem_open("/sem_receiver", O_CREAT, 0644, 1);  

   
    // Initiation
    if (method == 1) {

	// key is let sender and receiver connect with same message queue
        key = ftok(".", 65);
	// open message queue and save queue ID, if no create
        mailbox.storage.msqid = msgget(key, 0666 | IPC_CREAT);
    } 
    else if (method == 2) {

	// same as message passing, but connect with same memory
        key = ftok(".", 65);
        shmid = shmget(key, 1024, 0666 | IPC_CREAT);
        mailbox.storage.shm_addr = (char*)shmat(shmid, (void*)0, 0);
    }

    // Sending
    FILE* file = fopen(input_file, "r");
    
    if (method == 1) 
        printf("Message Passing\n");
    else if (method == 2)
        printf("Shared Memory\n");

    while (fgets(message.msgText, sizeof(message.msgText), file) != NULL) {
	send(message, &mailbox);
    }
    
    // End of sending
    // copy EOF to message to let receiver know sender exit
    strcpy(message.msgText, "EOF");
    send(message, &mailbox);

    // Unlink and delete the share memory
    if (method == 2) {
        shmdt(mailbox.storage.shm_addr);
        shmctl(shmid, IPC_RMID, NULL);
    }

    printf("End of input file! exit!\n");
    printf("Total time taken in sending msg: %.9f s\n", total_time);

    fclose(file);
    sem_close(sem_sender);
    sem_close(sem_receiver);

    return 0;
}
