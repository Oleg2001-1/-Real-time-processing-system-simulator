#include "stdio.h"
#include "pthread.h"
#include "semaphore.h"
#include "stdlib.h"
#include "stdbool.h"
#include "unistd.h"
#include "time.h"
#include "stdlib.h"
#define N 100 // zr
#define M 64 // kol dan
#define K 3 // liczba kom buf
#define B 512
#define min(a, b) (a<b?a:b)

typedef struct timespec timespec_t;

pthread_t zrodla[N];
int queue[2][B/2]; // queue[0] - bufer paczki, [1] - strumienia
int queue_id[2]; // elementow w buforze queue
int k_buf[K][M]; // k_buf[i] - bufer kanala i-tego
int k_id[K]; // k_id[i] - elementow w buforze kanala i-tego
int k_pakiet[K]; // numer pakietu do wyslania (0-A, 1-B, [2-10]-D, 11-C, [-1]-uzupelnienie C)
sem_t pacz_buf;
sem_t strum_buf;
int suma_pacz = 0;
int suma_strum = 0;
int utrac_pacz = 0;
int utrac_strum = 0;

void* producer(void*);
void* consumer(void*);

void* producer(void* _)
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
                sem_wait(&strum_buf);
                suma_strum++;
                if (queue_id[1] < B/2-1) queue[1][queue_id[1]++] = data;
                else {} //printf("Utracono danych ze strumieni: %f%%!\n", 100*((double)++utrac_strum)/(double)suma_strum);
                sem_post(&strum_buf);
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
                sem_wait(&pacz_buf);
                suma_pacz++;
                if (queue_id[0] < B/2-1) queue[0][queue_id[0]++] = data;
                else {} //printf("Utracono danych z paczki: %f%%!\n", 100*((double)++utrac_pacz)/(double)suma_pacz);
                sem_post(&pacz_buf);
            }
        }
    }
    return NULL;
}

void* consumer(void* void_id)
{   
    int id = *((int*) void_id);
    timespec_t wait_time = {0, 0};
    int zost;
    while(1)
    {
        wait_time.tv_nsec = M;
        nanosleep(&wait_time, &wait_time);
        switch(k_pakiet[id])
        {
            case -1: // C uzup.
            case 0: // A
                if (queue_id[0] == 0) break;
                sem_wait(&pacz_buf);
                zost = min(queue_id[0], M - k_id[id]);
                while (zost > 0)
                {
                    k_buf[id][k_id[id]++] = queue[0][0];
                    for (int i = 0; i < queue_id[0]-1; i++)
                        queue[0][i] = queue[0][i+1];
                    zost--;
                    queue_id[0]--;
                }
                sem_post(&pacz_buf);
                if (k_id[id] == M)
                {
                    k_id[id] = 0;
                    k_pakiet[id]++;
                    if (k_pakiet[id] == 1) printf("Pakiet A zostal wyslany przez %i!\n", id);
                    else printf("Pakiet C zostal wyslany przez %i!\n", id);
                }
                break;

            case 1: // B
                sem_wait(&strum_buf);
                zost = min(queue_id[1], M);
                for (int i = 0; i < M - zost; i++)
                    k_buf[id][k_id[id]++] = 0;
                while (zost > 0)
                {
                    k_buf[id][k_id[id]++] = queue[1][0];
                    for (int i = 0; i < queue_id[1]-1; i++)
                        queue[1][i] = queue[1][i+1];
                    zost--;
                    queue_id[1]--;
                }
                sem_post(&strum_buf);
                k_pakiet[id]++;
                k_id[id] = 0;
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
                if (queue_id[1] == 0) break;
                sem_wait(&strum_buf);
                zost = min(queue_id[1], M - k_id[id]);
                while (zost > 0)
                {
                    k_buf[id][k_id[id]++] = queue[1][0];
                    for (int i = 0; i < queue_id[1]-1; i++)
                        queue[1][i] = queue[1][i+1];
                    zost--;
                    queue_id[1]--;
                }
                sem_post(&strum_buf);
                if (k_id[id] == M)
                {
                    k_id[id] = 0;
                    k_pakiet[id]++;
                    printf("Pakiet D zostal wyslany przez %i!\n", id);
                }
                break;

            case 10: // C
                if (queue_id[0] + queue_id[1] == 0) break;
                sem_wait(&strum_buf);
                zost = min(queue_id[1], M - k_id[id]);
                while (zost > 0)
                {
                    k_buf[id][k_id[id]++] = queue[1][0];
                    for (int i = 0; i < queue_id[1]-1; i++)
                        queue[1][i] = queue[1][i+1];
                    zost--;
                    queue_id[1]--;
                }
                sem_post(&strum_buf);
                if (k_id[id] == M)
                {
                    k_id[id] = 0;
                    k_pakiet[id] = 0;
                    printf("Pakiet C zostal wyslany przez %i!\n", id);
                    break;
                }
                if ( queue_id[0] == 0) break;
                sem_wait(&pacz_buf);
                zost = min(queue_id[0], M - k_id[id]);
                while (zost > 0)
                {
                    k_buf[id][k_id[id]++] = queue[0][0];
                    for (int i = 0; i < queue_id[0]-1; i++)
                        queue[0][i] = queue[0][i+1];
                    zost--;
                    queue_id[0]--;
                }
                sem_post(&pacz_buf);
                if (k_id[id] == M)
                {
                    k_id[id] = 0;
                    k_pakiet[id] = 0;
                    printf("Pakiet C zostal wyslany przez %i!\n", id);
                    break;
                }
                k_pakiet[id] = -1;
                break;
        }
    }
    return NULL;
}

int main()
{   
    srand(time(NULL));
    for (int i = 0; i < 2; i++)
    {
        queue_id[i] = 0;
    }
    pthread_t nadawcy[N], odbiorcy[K];
    sem_init(&pacz_buf, 0, 1);
    sem_init(&strum_buf, 0, 1);
    int nadawcy_i[N], odbiorcy_i[K];
    for (int i = 0; i < N; i++)
    {
        nadawcy_i[i] = i;
        pthread_create(&nadawcy[i], NULL, &producer, (void*)&nadawcy_i[i]);
    }
        
    for (int i = 0; i < K; i++)
    {
        odbiorcy_i[i] = i;
        k_pakiet[i] = 0;
        k_id[i] = 0;
        pthread_create(&odbiorcy[i], NULL, &consumer, (void*)&odbiorcy_i[i]);
    }
        

    sem_destroy(&pacz_buf);
    sem_destroy(&strum_buf);
    pthread_exit(NULL);
    return 0;
}
