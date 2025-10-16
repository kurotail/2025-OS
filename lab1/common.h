#define _POSIX_C_SOURCE 199309L

#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>

#define MSG_PASSING 1
#define SHARED_MEM 2
#define TEXT_LEN_MAX 1024
#define SEM_SEND "os_lab1_send"
#define SEM_RECV "os_lab1_recv"
#define SHM "os_lab1_shm"
#define MQ "/os_lab1_mq"

typedef struct {
    double time;
    int flag;      // 1 for message passing, 2 for shared memory
    union{
        int msqid; //for system V api. You can replace it with structure for POSIX api
        char* shm_addr;
    }storage;
} mailbox_t;


typedef struct {
    /*  TODO: 
        Message structure for wrapper
    */
    long mType;
    char msgText[TEXT_LEN_MAX];
} message_t;

struct mq_attr MQ_ATTR = {
    0,
    10,
    sizeof(message_t),
    0
};

