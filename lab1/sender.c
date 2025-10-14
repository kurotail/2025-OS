#include "sender.h"

void send(message_t message, mailbox_t* mailbox_ptr){
    switch (mailbox_ptr->flag)
    {
    case SHARED_MEM:
        if (strcmp(message.msgText, "$")) printf("Sending message \"%s\"\n", message.msgText);
        clock_t start = clock();
        memcpy(mailbox_ptr->storage.shm_addr, &message, sizeof(message_t));
        clock_t end = clock();
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
    if ( (sem_send = sem_open("os_lab1_send", O_CREAT, 0666, 0)) == SEM_FAILED) {
        printf("Error creating sem_send\n");
        exit(-1);
    } 
    if ( (sem_recv = sem_open("os_lab1_recv", O_CREAT, 0666, 0)) == SEM_FAILED) {
        printf("Error creating sem_recv\n");
        exit(-1);
    } 
    int fd = shm_open("os_lab1_shm", O_CREAT | O_RDWR, 0666);
    if (ftruncate(fd, sizeof(message_t)) == -1) {
        fprintf(stderr, "Err ftruncate\n");
        exit(-1);
    }
    mailbox.storage.shm_addr = (void*)mmap(NULL, sizeof(message_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    message_t msg; 
    for (char *line = strtok(buffer, "\n"); line[0] != '$'; line = strtok(NULL, "\n"))
    {
        sem_wait(sem_recv);
        msg.mType = strlen(line);
        if (msg.mType > 1023) msg.mType = 1023;
        msg.msgText[msg.mType] = '\0';
        strcpy(msg.msgText, line);
        send(msg, &mailbox);
        sem_post(sem_send);
    }
    printf("End of file\n");
    sem_wait(sem_recv);
    strcpy(msg.msgText, "$");
    send(msg, &mailbox);
    sem_post(sem_send);
    
        /*  TODO: 
        1) [x] Call send(message, &mailbox) according to the flow in slide 4
        2) [x] Measure the total sending time
        3) [x] Get the mechanism and the input file from command line arguments
            • e.g. ./sender 1 input.txt
                    (1 for Message Passing, 2 for Shared Memory)
        4) [x] Get the messages to be sent from the input file
        5) [x] Print information on the console according to the output format
        6) [x] If the message form the input file is EOF, send an exit message to the receiver.c
        7) [x] Print the total sending time and terminate the sender.c
    */

    printf("Time taken: %lf\n", mailbox.time);

    munmap(mailbox.storage.shm_addr, sizeof(message_t));
    shm_unlink("os_lab1_shm");
    sem_close(sem_send);
    sem_close(sem_recv);
    sem_unlink("os_lab1_send");
    sem_unlink("os_lab1_recv");
    free(buffer);
}
