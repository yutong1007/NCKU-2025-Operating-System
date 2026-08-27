#include "receiver.h"
#include "time.h"

void receive(message_t* message_ptr, mailbox_t* mailbox_ptr) {
    struct timespec start, end;

    // Message Passing
    if (mailbox_ptr->flag == 1) {
	
	// Only measure receiving process
        clock_gettime(CLOCK_MONOTONIC, &start);

	// msgrcv(message queue ID, receive message to, message data size, only mType 1, control 0)
        if (msgrcv(mailbox_ptr->storage.msqid, message_ptr, sizeof(message_ptr->msgText), 1, 0) == -1) {
            exit(1);
        }
        clock_gettime(CLOCK_MONOTONIC, &end);
        total_time += (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;

	
	// return ACK Ok, sender sent next message
	
	message_t ack;
	ack.mType = 2;
	strcpy(ack.msgText, "OK");
	// send ACK message back
	if (msgsnd(mailbox_ptr->storage.msqid, &ack, strlen(ack.msgText) + 1, 0) == -1) {
            exit(1);
	}
	
    } 
    // Shared Memory
    else if (mailbox_ptr->flag == 2) {
        // waiting sender finish
	sem_wait(sem_sender);
	
	// Only measure reading memory process
        clock_gettime(CLOCK_MONOTONIC, &start);

	// reading message from shared memory	
        strcpy(message_ptr->msgText, mailbox_ptr->storage.shm_addr); 
        clock_gettime(CLOCK_MONOTONIC, &end);                                            
        
	// tell sender can sent next
	sem_post(sem_receiver);                                     
        total_time += (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;
    } 
}

int main(int argc, char* argv[]) {

    int method = atoi(argv[1]);

    mailbox_t mailbox;
    mailbox.flag = method;
    message_t message;

    // Sender = 0 Receiver = 1
    sem_sender = sem_open("/sem_sender", O_CREAT, 0644, 0);      
    sem_receiver = sem_open("/sem_receiver", O_CREAT, 0644, 1);  

    // Initiation
    if (method == 1) {
        key_t key = ftok(".", 65);
        mailbox.storage.msqid = msgget(key, 0666);
    } 
    else if (method == 2) {
        key_t key = ftok(".", 65);
        int shmid = shmget(key, 1024, 0666);
        mailbox.storage.shm_addr = (char*)shmat(shmid, (void*)0, 0);
    }

    // Receiving
    if (method == 1)
        printf("Message Passing\n");
    else if (method == 2)
        printf("Shared Memory\n");

    while (1) {
        receive(&message, &mailbox);
        // if message text is EOF, end of sending 
	if (strcmp(message.msgText, "EOF") == 0) break;
        printf("Receiving message: %s", message.msgText);
    }

    // Delete message queue
    if (method == 1) {
        msgctl(mailbox.storage.msqid, IPC_RMID, NULL);
    }

    printf("Sender exit!\n");
    printf("Total time taken in receiving msg: %.9f s\n", total_time);

    sem_close(sem_sender);
    sem_close(sem_receiver);

    return 0;
}
