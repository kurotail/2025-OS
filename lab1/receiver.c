#include "receiver.h"

void receive(message_t* message_ptr, mailbox_t* mailbox_ptr){
    switch (mailbox_ptr->flag)
    {
    case SHARED_MEM:
        char buf[TEXT_LEN_MAX];
        clock_t start = clock();
        strcpy(buf, message_ptr->msgText);
        clock_t end = clock();
        mailbox_ptr->time += (double)(end-start) * 1e-3;
        if (strcmp(buf, "$")) printf("Received \"%s\"\n", buf);
        break;
    
    default:
        fprintf(stderr, "Err mailbox flag %d\n", mailbox_ptr->flag);
        exit(-1);
    }
}

int main(int argc, char *argv[]){
    if (argc != 2) { 
        fprintf(stderr, "Usage: receiver <COMM_MODE>\n");
        exit(-1);
    }

    mailbox_t mailbox;
    mailbox.flag = atoi(argv[1]);

    sem_t *sem_send, *sem_recv;
    if ( (sem_send = sem_open("os_lab1_send", 0)) == SEM_FAILED || 
         (sem_recv = sem_open("os_lab1_recv", 0)) == SEM_FAILED)
        printf("Error creating sem\n");
    int fd = shm_open("os_lab1_shm", O_RDWR, 0666);
    message_t *msg = mailbox.storage.shm_addr = (message_t*)mmap(NULL, sizeof(message_t), PROT_READ, MAP_SHARED, fd, 0);

    sem_post(sem_recv); 
    while (1)
    {
        sem_wait(sem_send);
        receive(msg, &mailbox);
        if (strcmp(msg->msgText, "$") == 0) break;
        sem_post(sem_recv);
    }
        
    /*  TODO: 
        1) [x] Call receive(&message, &mailbox) according to the flow in slide 4
        2) [x] Measure the total receiving time
        3) [x] Get the mechanism from command line arguments
            • e.g. ./receiver 1
        4) [x] Print information on the console according to the output format
        5) [x] If the exit message is received, print the total receiving time and terminate the receiver.c
    */

    printf("Time taken: %lf\n", mailbox.time);

    munmap(msg, sizeof(message_t));
    sem_close(sem_send);
    sem_close(sem_recv);
}
