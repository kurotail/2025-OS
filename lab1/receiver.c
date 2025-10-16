#include "receiver.h"

void receive(message_t* message_ptr, mailbox_t* mailbox_ptr){
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    switch (mailbox_ptr->flag)
    {
    case SHARED_MEM:
        memcpy(message_ptr, mailbox_ptr->storage.shm_addr, sizeof(message_t));
        break;

    case MSG_PASSING:
        unsigned pri;
        int recv_bytes = mq_receive(mailbox_ptr->storage.msqid, (void*)message_ptr, sizeof(message_t), &pri);
        if (recv_bytes == -1) {
            fprintf(stderr, "Err receiving message, %d\n", errno);
            exit(-1);
        }
        break;
    
    default:
        fprintf(stderr, "Err mailbox flag %d\n", mailbox_ptr->flag);
        exit(-1);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    mailbox_ptr->time += (end.tv_sec-start.tv_sec)+(end.tv_nsec-start.tv_nsec)*1e-9;
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
    if ( (sem_send = sem_open(SEM_SEND, 0)) == SEM_FAILED) {
        printf("Error creating sem_send\n");
        exit(-1);
    } 
    if ( (sem_recv = sem_open(SEM_RECV, 0)) == SEM_FAILED) {
        printf("Error creating sem_recv\n");
        exit(-1);
    } 
    switch (mailbox.flag)
    {
    case SHARED_MEM:
        int fd = shm_open(SHM, O_RDWR, 0666);
        mailbox.storage.shm_addr = (void*)mmap(NULL, sizeof(message_t), PROT_READ, MAP_SHARED, fd, 0);
        close(fd);
        break;

    case MSG_PASSING:
        if (-1 == (mailbox.storage.msqid = mq_open(MQ, O_RDONLY | O_CREAT, 0666, &MQ_ATTR))) {
            printf("Error opening message queue\n");
            exit(-1);
        }
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
        sem_post(sem_recv);
        if (strcmp(msg->msgText, "$") == 0) break;
        printf("Received \"%s\"\n", msg->msgText);
    }
    printf("Sender exit\n");
    printf("Time taken: %lf\n", mailbox.time);

    free(msg);
    switch (mailbox.flag)
    {
    case SHARED_MEM:
        munmap(mailbox.storage.shm_addr, sizeof(message_t));
        break;
    
    case MSG_PASSING:
        mq_close(mailbox.storage.msqid);
        mq_unlink(MQ);
        break;
    
    default:
        fprintf(stderr, "Err mailbox flag %d\n", mailbox.flag);
        exit(-1);
        break;
    }
    sem_close(sem_send);
    sem_close(sem_recv);
}
