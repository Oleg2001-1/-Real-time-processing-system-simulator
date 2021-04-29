#include <stdio.h> 
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/stat.h> 
#include <string.h> 
#include <errno.h> 
#include <fcntl.h> 
#include <pthread.h> 
#include <unistd.h>
#include <semaphore.h>
#include "time.h"

#define N 10 // zr
#define M 64 // kol dan
#define K 3 // liczba kom buf
#define B 512
#define min(a, b) (a<b?a:b)

int wsego_pacz = 0;
int wsego_strum = 0;
int strac_pacz = 0;
int strac_strum = 0;

typedef struct timespec timespec_t;

class Condition
{
    friend class Monitor;

    private:
        int num;
        sem_t sem;
    public:
        Condition() : num(0) { sem_init(&sem, 0, 1); }
        ~Condition() { sem_destroy(&sem); }
        void wait() { sem_wait(&sem); }
        bool signal()
        {
            if (num > 0)
            {
                num--;
                sem_post(&sem);
                return true;
            }
            else
                return false;
        }
        
};

class Monitor
{
    private:
        sem_t sem;
    public:
        Monitor() { sem_init(&sem, 0, 1); }
        ~Monitor() { sem_destroy(&sem); }

        void enter() { sem_wait(&sem); }
        void leave() { sem_post(&sem); }
        void wait (Condition* cond)
        {
            cond->num++;
            leave();
            cond->wait();
        }

        void signal(Condition* cond)
        {
            if( cond->signal() )
			    enter();
        }
};

pid_t create_producer(int queue[2][B/2], int queue_id[2], Monitor* pacz_mon, Monitor* strum_mon, Condition* pacz_cond, Condition* strum_cond);
pid_t create_consumer(int id, int queue[2][B/2], int queue_id[2], Monitor* pacz_mon, Monitor* strum_mon, Condition* pacz_cond, Condition* strum_cond);
void work_producer(int queue[2][B/2], int queue_id[2], Monitor* pacz_mon, Monitor* strum_mon, Condition* pacz_cond, Condition* strum_cond);
void work_consumer(int id, int queue[2][B/2], int queue_id[2], Monitor* pacz_mon, Monitor* strum_mon, Condition* pacz_cond, Condition* strum_cond);

pid_t create_producer(int queue[2][B/2], int queue_id[2], Monitor* pacz_mon, Monitor* strum_mon, Condition* pacz_cond, Condition* strum_cond)
{
    pid_t id_ = fork();
    if (id_ == 0)
    {
        work_producer(queue, queue_id, pacz_mon, strum_mon, pacz_cond, strum_cond);
        exit(0);
    }
    return id_;
}

pid_t create_consumer(int id, int queue[2][B/2], int queue_id[2], Monitor* pacz_mon, Monitor* strum_mon, Condition* pacz_cond, Condition* strum_cond)
{
    pid_t id_ = fork();
    if (id_ == 0)
    {
        work_consumer(id, queue, queue_id, pacz_mon, strum_mon, pacz_cond, strum_cond);
        exit(0);
    }
    return id_;
}

void work_producer(int queue[2][B/2], int queue_id[2], Monitor* pacz_mon, Monitor* strum_mon, Condition* pacz_cond, Condition* strum_cond)
{
    timespec_t next_trans = {0, 0};
    int type_of_data;
    int num_of_data;
    int data;
    while(1)
    {
        next_trans.tv_nsec = 1e8 + (rand()%41 - 20)/100;
        nanosleep(&next_trans, &next_trans);

        type_of_data = rand()%2;
        if (type_of_data) // strumien
        {
            num_of_data = rand()%471 + 30;
            for (int i = 0; i < num_of_data; i++)
            {
                data = rand();
                // sem_wait(&strum_mon);
                strum_mon->enter();

                wsego_strum++;
                if (queue_id[1] < B/2-1) queue[1][queue_id[1]++] = data;
                else {}// printf("Utracono danych ze strumieni: %f%%!\n", 100*((double)++strac_strum)/(double)wsego_strum);
                // sem_post(&strum_mon);
                strum_mon->leave();
                strum_mon->signal(strum_cond);

                next_trans.tv_nsec = 1e5;
                nanosleep(&next_trans, &next_trans);
            }
        }
        else // paczka
        {
            num_of_data = rand()%101;
            for (int i = 0; i < num_of_data; i++)
            {
                data = rand();
                next_trans.tv_nsec = 5*1e5;
                nanosleep(&next_trans, &next_trans);
                // sem_wait(&pacz_mon);
                pacz_mon->enter();

                wsego_pacz++;
                if (queue_id[0] < B/2-1) queue[0][queue_id[0]++] = data;
                else {}//printf("Utracono danych z paczki: %f%%!\n", 100*((double)++strac_pacz)/(double)wsego_pacz);
                // sem_post(&pacz_mon);
                pacz_mon->leave();
                pacz_mon->signal(pacz_cond);
            }
        }
    }
}

void work_consumer(int id, int queue[2][B/2], int queue_id[2], Monitor* pacz_mon, Monitor* strum_mon, Condition* pacz_cond, Condition* strum_cond)
{
    timespec_t wait_time = {0, 0};
    int buf[M];
    int pakiet_nr = 0;
    int read_alr;
    int zost;
    while(1)
    {
        wait_time.tv_nsec = M;
        nanosleep(&wait_time, &wait_time);
        switch(pakiet_nr)
        {
            case 0: // A
                read_alr = 0;
                pacz_mon->enter();
                while (read_alr < M)
                {
                    if (queue_id[0] > 0)
                        while (queue_id[0] > 0 && read_alr < M)
                        {
                            buf[read_alr++] = queue[0][0];
                            for (int i = 0; i < queue_id[0]-1; i++)
                                queue[0][i] = queue[0][i+1];
                            queue_id[0]--;
                            printf("read\n");
                        }
                    else
                        pacz_mon->wait(pacz_cond);
                }
                pacz_mon->leave();

                pakiet_nr++;
                printf("Pakiet A zostal wyslany przez %i!\n", id);
                break;

            case 1: // B
                strum_mon->enter();
                zost = min(queue_id[1], M);
                for (int i = 0; i < M - zost; i++)
                    buf[i] = 0;
                while (zost > 0)
                {
                    buf[M-zost] = queue[1][0];
                    for (int i = 0; i < queue_id[1]-1; i++)
                        queue[1][i] = queue[1][i+1];
                    zost--;
                    queue_id[1]--;
                }
                strum_mon->leave();
                pakiet_nr++;
                printf("Pakiet B zostal wyslany przez %i!\n", id);
                break;

            case 2: // D
            case 3: // D
            case 4: // D
            case 5: // D
            case 6: // D
            case 7: // D
            case 8: // D
            case 9: // D
            case 10: // D
                read_alr = 0;
                strum_mon->enter();
                while (read_alr < M)
                {
                    if (queue_id[1] > 0)
                        while (queue_id[1] > 0 && read_alr < M)
                        {
                            buf[read_alr++] = queue[1][0];
                            for (int i = 0; i < queue_id[1]-1; i++)
                                queue[1][i] = queue[1][i+1];
                            queue_id[1]--;
                        }
                    else
                        strum_mon->wait(strum_cond);
                }
                strum_mon->leave();

                pakiet_nr++;
                printf("Pakiet D zostal wyslany przez %i!\n", id);
                break;

            case 11: // C
                zost = M;
                strum_mon->enter();
                while (zost > 0 && queue_id[1] > 0)
                {
                    buf[M-zost] = queue[1][0];
                    for (int i = 0; i < queue_id[1]-1; i++)
                        queue[1][i] = queue[1][i+1];
                    queue_id[1]--;
                    zost--;
                }
                strum_mon->leave();

                pacz_mon->enter();
                while (zost > 0)
                {
                    if (queue_id[0] > 0)
                        while (queue_id[0] > 0 && zost > 0)
                        {
                            buf[M-zost] = queue[0][0];
                            for (int i = 0; i < queue_id[0]-1; i++)
                                queue[0][i] = queue[0][i+1];
                            queue_id[0]--;
                            zost--;
                        }
                    else
                        pacz_mon->wait(pacz_cond);
                }
                pacz_mon->leave();

                pakiet_nr = 0;
                printf("Pakiet C zostal wyslany przez %i!\n", id);
                break;
        }
    }
}

int main()
{
    srand(time(NULL));
    pid_t nadawcy[N], odbiorcy[K];
    int queue[2][B/2]; // queue[0] - bufer paczki, [1] - strumienia
    int queue_id[2]; // elementow w buforze queue

    Monitor pacz_mon, strum_mon;
    Condition pacz_cond, strum_cond;
    for (int i = 0; i < N; i++)
        nadawcy[i] = create_producer(queue, queue_id, &pacz_mon, &strum_mon, &pacz_cond, &strum_cond);
    for (int i = 0; i < K; i++)
        odbiorcy[i] = create_consumer(i, queue, queue_id, &pacz_mon, &strum_mon, &pacz_cond, &strum_cond);
    while(1) { sleep(100000); }
}