#include "receiver.h"

void receive(message_t* message_ptr, mailbox_t* mailbox_ptr){
    switch (mailbox_ptr->flag)
    {
    case SHARED_MEM:
        clock_t start = clock();
        memcpy(message_ptr, mailbox_ptr->storage.shm_addr, sizeof(message_t));
        clock_t end = clock();
        mailbox_ptr->time += (double)(end-start) * 1e-3;
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
    mailbox.time = 0.0;
    mailbox.flag = atoi(argv[1]);

    sem_t *sem_send, *sem_recv;
    if ( (sem_send = sem_open("os_lab1_send", 0)) == SEM_FAILED) {
        printf("Error creating sem_send\n");
        exit(-1);
    } 
    if ( (sem_recv = sem_open("os_lab1_recv", 0)) == SEM_FAILED) {
        printf("Error creating sem_recv\n");
        exit(-1);
    } 
    switch (mailbox.flag)
    {
    case SHARED_MEM:
        int fd = shm_open("os_lab1_shm", O_RDWR, 0666);
        mailbox.storage.shm_addr = (void*)mmap(NULL, sizeof(message_t), PROT_READ, MAP_SHARED, fd, 0);
        close(fd);
        break;
    
    default:
        fprintf(stderr, "Err mailbox flag %d\n", mailbox.flag);
        exit(-1);
        break;
    }
    
    message_t *msg = (message_t*)malloc(sizeof(message_t));
    sem_post(sem_recv); 
    while (1)
    {
        sem_wait(sem_send);
        receive(msg, &mailbox);
        if (strcmp(msg->msgText, "$") == 0) break;
        printf("Received \"%s\"\n", msg->msgText);
        sem_post(sem_recv);
    }
    printf("Sender exit\n");
    printf("Time taken: %lf\n", mailbox.time);
        
    /*  TODO: 
        1) [x] Call receive(&message, &mailbox) according to the flow in slide 4
        2) [x] Measure the total receiving time
        3) [x] Get the mechanism from command line arguments
            • e.g. ./receiver 1
        4) [x] Print information on the console according to the output format
        5) [x] If the exit message is received, print the total receiving time and terminate the receiver.c
    */

    free(msg);
    switch (mailbox.flag)
    {
    case SHARED_MEM:
        munmap(mailbox.storage.shm_addr, sizeof(message_t));
        break;
    
    default:
        fprintf(stderr, "Err mailbox flag %d\n", mailbox.flag);
        exit(-1);
        break;
    }
    sem_close(sem_send);
    sem_close(sem_recv);
}
