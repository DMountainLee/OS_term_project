#include "queue.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <wait.h>
#include <time.h>
#define time_quantum 225
#define interval 4000
#define MAX 2000

pid_t pid;
int tq;
int cpu_time;
int io_time;
int key;
int msgq;
int end_msgq;
int tick_count = 0;

struct sigaction new_sa;
struct itimerval new_itimer, old_itimer;
struct timeval begin,end;

void scheduler_handler(int signo);
void process_handler(int signo);
void tick(int a, int b);
void io_reduce();

queue* wait_queue;
queue* run_queue;

typedef struct msg{
    int mtype;
    pid_t pid;
    int io_time;
    int cpu_time;
} msgbuf;

typedef struct pcb{
    pid_t pid;
    int io_time;
    int cpu_time;
    int tq;
} PCB;

PCB* pcb;
msgbuf msg;
FILE *fp;
char buffer[100];

int main(void)
{
    fp = fopen("schedule_dump.txt", "w");

    // timer start
    gettimeofday(&begin, NULL);
    memset(&new_sa, 0, sizeof(new_sa));

    // tick 마다 run queue에 front에 있는 process cpu time -1 , wait queue 전체 i/o time -1
    new_sa.sa_handler = &scheduler_handler;
    sigaction(SIGALRM, &new_sa, NULL);
    
    run_queue = createqueue();
    wait_queue = createqueue();
    int count;

    for(count=0;count<10;count++)
    {
        pid=fork();
        if(pid==0) break;
        
        else if(pid>0)
        {
            pcb = (PCB*)malloc(sizeof(PCB));
            // msgq creat
            key=1111;
            if((msgq = msgget(key, IPC_CREAT | 0666)) == -1)
            
            {
                perror("msgget() failed!");
                exit(1);
            }
            
            // recive infomation
            while((msgrcv(msgq, &msg, sizeof(msgbuf ),0,IPC_NOWAIT))==-1);
            
            io_time = msg.io_time;
            // PCB information
            pcb->pid = msg.pid;
            pcb->io_time=msg.io_time;
            pcb->cpu_time=msg.cpu_time;
            pcb->tq = time_quantum;

            enqueue(run_queue,pcb);
        }     
        else printf("fork error!\n");
    }

    // 1. process(child)는 cpu time를 random으로 생성하고 msgq를 만들어서 메세지를 보냄
    // 2. 기다리다 kernel(parent)에서 msg에 따라 기능 수행
    // 2-1. cpu time -1
    // 2-2. i/o time -1
    // 2-3. process kill
    // 3. 계속 반복하다 process kill msg를 받으면 process 종료
    if(pid==0)
    {
        // msgq creat
        key=1111;
        if((msgq = msgget(key, IPC_CREAT | 0666)) == -1)
        {
            perror("msgget() failed!");
            exit(1);
        }

        // msgq creat
        int end_key=2222;
        if((end_msgq = msgget(end_key, IPC_CREAT | 0666)) == -1)
        {
            perror("msgget() failed!");
            exit(1);
        }
        srand(getpid());
        io_time = 0;
        cpu_time=(rand()%MAX)+1;
        
        // send infomation
        msg.mtype=0;
        msg.pid=getpid();
        msg.io_time=io_time;
        msg.cpu_time=cpu_time;
        msgsnd(msgq, &msg,sizeof(msg),NULL);
        new_sa.sa_handler = &process_handler;
        sigaction(SIGUSR1,&new_sa, NULL);
        while(1) sleep(1);
    }

    // 1. kernel(parent)에서 process한테 cpu time를 담은 msg를 받기전까지 멈춤
    // 2. msg를 받으면 해당 process의 pcb를 생성하여 run queue에 넣음
    else if(pid > 0)
    {
        sprintf(buffer,"----process pids get CPU Time-----\n");
        fputs(buffer,fp);

        for(Node* traverse=run_queue->front;traverse != NULL; traverse=traverse->next)
        {
            sprintf(buffer,"%d ",((PCB*)(traverse->dataptr))->cpu_time);
            fputs(buffer,fp);
        }

        sprintf(buffer,"\n----------------------------------\n\n");
        fputs(buffer,fp);
    }
    
    tick(0,interval);

    // end msgq
    key= 2222;
    end_msgq = msgget(key,IPC_CREAT|0666);
    
    // process(child)이 끝날 때까지 kernel(parent)은 계속 수행
    while (count > 0) 
    {
		child_pid = wait(&status);
		printf("\t*process [PID:%d] finished.\n", child_pid);
		count--;
		sleep(1);
	}

    return 0;    
}

void tick(int a, int b)
{
    tick_count++;
    new_itimer.it_interval.tv_sec = a;
    new_itimer.it_interval.tv_usec = b;
    new_itimer.it_value.tv_sec = a;
    new_itimer.it_value.tv_usec = b;
    setitimer(ITIMER_REAL, &new_itimer, NULL);
}

// tik 마다 run queue와 wait queue에서 process 처리
void scheduler_handler(int signo)
{
    PCB* pcb = NULL;
    tick_count++;
    
    if((emptyqueue(run_queue))&&(emptyqueue(wait_queue)))
    {
        double diff;
        gettimeofday(&end, NULL);
        diff = end.tv_sec + end.tv_usec / 1000000.0 - begin.tv_sec - begin.tv_usec / 1000000.0;
        sprintf(buffer,"\nfinish time : %f / time tick : %d\n",diff, tick_count);
        fputs(buffer,fp);
        printf("end\n");
    }

    // wait queue에 process(chiid)가 있다면 i/o time -1
    if(!emptyqueue(wait_queue))
    {
        io_reduce();

        // wait queue front에 pcb를 가져온 후, i/o time = 0이면 run queue다시 넣는다.
        queuefront(wait_queue,(void**)&pcb);
        if(pcb->io_time==0)
        {
            dequeue(wait_queue,(void**)&pcb);
            enqueue(run_queue,(void*)pcb);
        }
    }

    if(!emptyqueue(run_queue))
    {
        queuefront(run_queue,(void**)&pcb);
        pcb->tq--;
        pcb->cpu_time--;
        
        // pcb에서의 cpu time를 해당 process에서도 반영하기위해 IPC(kill)를 사용
        kill(pcb->pid,SIGUSR1);

        // cpu time이 0이면 해당 process kill
        if(pcb->cpu_time == 0)
        {
            dequeue(run_queue,(void**)&pcb);
            kill(pcb->pid,SIGKILL);
            free(pcb);           
        }

        // if time slice is zero
        // 1. time silce reset, i/o time randomly generate 2. context switch
        else if(pcb->tq==0)
        {
            pcb->tq=time_quantum;
            srand(pcb->cpu_time);
            pcb->io_time = rand() % getpid();
            dequeue(run_queue,(void**)&pcb);
            enqueue(wait_queue,(void*)pcb);
            
            // when text-switch occurs, print operations(time, remaining cpu-burst of process pid)
            double diff;
            gettimeofday(&end, NULL);
            diff = end.tv_sec + end.tv_usec / 1000000.0 - begin.tv_sec - begin.tv_usec / 1000000.0;
            sprintf(buffer,"\n----------------------------------------\n");
            fputs(buffer,fp);
            sprintf(buffer,"text switch time : %f\n", diff );
            fputs(buffer,fp);
            sprintf(buffer,"----run queue remaining cpu burst-----\n");
            fputs(buffer,fp);
            
            for(Node* traverse=run_queue->front;traverse != NULL; traverse=traverse->next)
            {
                sprintf(buffer,"%d ",((PCB*)(traverse->dataptr))->cpu_time);
                fputs(buffer,fp);
            }

            sprintf(buffer,"\n--------------------------------------\n");
            fputs(buffer,fp);
            sprintf(buffer,"----wait queue remaining cpu burst-----\n");
            fputs(buffer,fp);

            for(Node* traverse=wait_queue->front;traverse != NULL; traverse=traverse->next)
            {
                sprintf(buffer,"%d ",((PCB*)(traverse->dataptr))->cpu_time);
                fputs(buffer,fp);
            }
            
            sprintf(buffer,"\n---------------------------------------\n");
            fputs(buffer,fp);
            sprintf(buffer,"----------------------------------------\n\n");
        }
    }
}

void io_reduce()
{
    PCB* pcbptr;
    Node* traverse;
    for(traverse = wait_queue->front;traverse!=NULL;traverse=traverse->next)
    {
        pcbptr=traverse->dataptr;
        if(pcbptr->io_time > 0) pcbptr->io_time--;
        else continue;
    }
}

void process_handler(int signo)
{
    cpu_time--;
    return;
}