#include "sender.h"

void send(message_t message, mailbox_t* mailbox_ptr){
    clock_t start, end;
    switch (mailbox_ptr->flag)
    {
    case SHARED_MEM:
        start = clock();
        memcpy(mailbox_ptr->storage.shm_addr, &message, sizeof(message_t));
        end = clock();
        mailbox_ptr->time += (double)(end-start) * 1e-3;
        break;

    case MSG_PASSING:
        start = clock();
        int send_bytes = mq_send(mailbox_ptr->storage.msqid, (void*)&message, sizeof(message_t), 0);
        if (send_bytes == -1) {
            fprintf(stderr, "Err sending message, %d\n", errno);
            exit(-1);
        }
        end = clock();
        mailbox_ptr->time += (double)(end-start) * 1e-3;
        break;
    
    default:
        fprintf(stderr, "Err mailbox flag %d\n", mailbox_ptr->flag);
        exit(-1);
    }
}
 
char *ftos(FILE *fp) {
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);
    char *buffer = (char *)malloc(file_size + 3);
    size_t bytes_read = fread(buffer, 1, file_size, fp);
    memcpy(&buffer[bytes_read], "\n$\n", 3);
    fclose(fp);
    return buffer;
}

int main(int argc, char *argv[]){
    if (argc != 3) {
        fprintf(stderr, "Usage: sender <COMM_MODE> <INPUT_FILE>\n");
        exit(-1);
    }
    
    FILE *fp = fopen(argv[2], "r");
    if (!fp) {
        fprintf(stderr, "Err opening %s\n", argv[1]);
        exit(-1);
    }
    char *buffer = ftos(fp);

    mailbox_t mailbox;
    mailbox.time = 0.0;
    mailbox.flag = atoi(argv[1]);

    sem_t *sem_send, *sem_recv;
    if ( (sem_send = sem_open(SEM_SEND, O_CREAT, 0666, 0)) == SEM_FAILED) {
        printf("Error creating sem_send\n");
        exit(-1);
    } 
    if ( (sem_recv = sem_open(SEM_RECV, O_CREAT, 0666, 0)) == SEM_FAILED) {
        printf("Error creating sem_recv\n");
        exit(-1);
    } 
    switch (mailbox.flag)
    {
    case SHARED_MEM:
        int fd = shm_open(SHM, O_CREAT | O_RDWR, 0666);
        if (ftruncate(fd, sizeof(message_t)) == -1) {
            fprintf(stderr, "Err ftruncate\n");
            exit(-1);
        }
        mailbox.storage.shm_addr = (void*)mmap(NULL, sizeof(message_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        close(fd);
        break;

    case MSG_PASSING:
        if (-1 == (mailbox.storage.msqid = mq_open(MQ, O_RDWR | O_CREAT, 0666, &MQ_ATTR))) {
            printf("Error opening message queue\n");
            exit(-1);
        }
        break;
    
    default:
        fprintf(stderr, "Err mailbox flag %d\n", mailbox.flag);
        exit(-1);
        break;
    }

    message_t msg; 
    for (char *line = strtok(buffer, "\n"); line[0] != '$'; line = strtok(NULL, "\n"))
    {
        sem_wait(sem_recv);
        msg.mType = strlen(line);
        if (msg.mType > TEXT_LEN_MAX) msg.mType = TEXT_LEN_MAX;
        strcpy(msg.msgText, line);
        msg.msgText[msg.mType-1] = '\0';
        printf("Sending message \"%s\"\n", msg.msgText);
        send(msg, &mailbox);
        sem_post(sem_send);
    }
    printf("End of file\n");
    sem_wait(sem_recv);
    strcpy(msg.msgText, "$");
    send(msg, &mailbox);
    sem_post(sem_send);
    printf("Time taken: %lf\n", mailbox.time);
    sem_wait(sem_recv);
    
    switch (mailbox.flag)
    {
    case SHARED_MEM:
        munmap(mailbox.storage.shm_addr, sizeof(message_t));
        shm_unlink(SHM);
        break;
    
    case MSG_PASSING:
        mq_close(mailbox.storage.msqid);
        break;
    
    default:
        fprintf(stderr, "Err mailbox flag %d\n", mailbox.flag);
        exit(-1);
        break;
    }
    sem_close(sem_send);
    sem_close(sem_recv);
    sem_unlink("os_lab1_send");
    sem_unlink("os_lab1_recv");
    free(buffer);
}
