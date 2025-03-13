/**
 *	File	: pc.c
 *
 *	Title	: Demo Producer/Consumer.
 *
 *	Short	: A solution to the producer consumer problem using pthreads.
 *
 *	Long 	:
 *
 *	Author	: Andrae Muys
 *
 *	Date	: 18 September 1997
 *
 *	Revised	: 13 March 2025
 */

// !Best performance with 250 consumers... While proccesor is 6 core 12 threads.

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>

#define QUEUESIZE 10
#define LOOP 100000
#define WORK 10

int full = 0;
int empty = 0;

void *producer(void *args);
void *consumer(void *args);

struct workFunction
{
  void *(*work)(void *); // Function pointer
  void *arg;             // Argument to the function
};

void *work(void *args);
void *computeSines(void *args);

struct queueItem
{
  struct workFunction wf;
  struct timespec timestamp;
};

typedef struct
{
  struct queueItem buf[QUEUESIZE];
  long head, tail;
  int full, empty;
  bool done; // Flag to indicate that all producers have finished
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
} queue;

queue *queueInit(void);
void queueDelete(queue *q);
void queueAdd(queue *q, struct workFunction in);
void queueDel(queue *q, struct queueItem *out);

// Statistics structure to tracschedulersk wait times
typedef struct
{
  long total_wait_time_usec;
  int count;
  pthread_mutex_t *mut;
} statistics;

statistics *statsInit(void);
void statsDelete(statistics *stats);
void statsAdd(statistics *stats, long wait_time_usec);
double statsGetAverage(statistics *stats);

typedef struct
{
  queue *fifo;
  statistics *stats;
} consumer_args;

int main(int argc, char *argv[])
{
  queue *fifo;
  statistics *stats;
  int p, q;

  // Check if the user has provided the correct number of arguments
  if (argc != 3)
  {
    fprintf(stderr, "Usage: %s <number of producers> <number of consumers>\n", argv[0]);
    exit(1);
  }

  // Convert the arguments to integers
  p = atoi(argv[1]);
  q = atoi(argv[2]);

  // Check if the arguments are valid
  if (p < 1 || q < 1)
  {
    fprintf(stderr, "The number of producers and consumers must be greater than 0\n");
    exit(1);
  }

  // Create the producer and consumer threads
  pthread_t pro[p];
  pthread_t con[q];

  fifo = queueInit();
  if (fifo == NULL)
  {
    fprintf(stderr, "main: Queue Init failed.\n");
    exit(1);
  }

  stats = statsInit();
  if (stats == NULL)
  {
    fprintf(stderr, "main: Statistics Init failed.\n");
    exit(1);
  }

  // Start timer 
  struct timespec start, finish;
  clock_gettime(CLOCK_MONOTONIC, &start);

  // Create the producer and consumer threads
  for (int i = 0; i < p; i++)
  {
    pthread_create(&pro[i], NULL, producer, fifo);
  }

  for (int i = 0; i < q; i++)
  {
    consumer_args *args = malloc(sizeof(consumer_args));
    args->fifo = fifo;
    args->stats = stats;
    pthread_create(&con[i], NULL, consumer, args);
  }

  // Join the threads after they have finished

  for (int i = 0; i < p; i++)
  {
    pthread_join(pro[i], NULL);
  }

  // Set the done flag and signal consumers
  pthread_mutex_lock(fifo->mut);
  fifo->done = 1;
  pthread_mutex_unlock(fifo->mut);
  pthread_cond_broadcast(fifo->notEmpty); // Wake up all consumers

  for (int i = 0; i < q; i++)
  {
    pthread_join(con[i], NULL);
  }

  // Stop timer
  clock_gettime(CLOCK_MONOTONIC, &finish);

  // Calculate the elapsed time
  long elapsed_time_nsec = (finish.tv_sec - start.tv_sec) * 1000000000L +
                           (finish.tv_nsec - start.tv_nsec);

  // Convert to microseconds
  long elapsed_time_ms = elapsed_time_nsec / 1000000;
  printf("Elapsed time: %ld us\n", elapsed_time_ms);

  // Print the average wait time
  double avg_wait_time = statsGetAverage(stats);
  printf("Average wait time: %f us\n", avg_wait_time);

  queueDelete(fifo);
  statsDelete(stats);

  printf("(full, empty): (%d, %d)\n", full, empty);

  return 0;
}

void *producer(void *q)
{
  queue *fifo;
  int i;

  fifo = (queue *)q;

  struct workFunction wf;
  double *angles = malloc(WORK * sizeof(double));

  for (i = 0; i < LOOP; i++)
  {

    // ******************  Produce some work  ******************

    for (int j = 0; j < WORK; j++)
    {
      angles[j] = (i * WORK + j) * 0.01; // Some random angles
    }

    wf.work = computeSines;
    wf.arg = angles;

    // *********************************************************

    pthread_mutex_lock(fifo->mut);
    while (fifo->full)
    {
      // printf("producer: queue FULL.\n");
      full++;
      pthread_cond_wait(fifo->notFull, fifo->mut);
    }

    queueAdd(fifo, wf);
    pthread_mutex_unlock(fifo->mut);
    pthread_cond_signal(fifo->notEmpty);
  }

  return (NULL);
}

void *consumer(void *args)
{
  consumer_args *c_args = (consumer_args *)args; // Fixed variable name
  queue *fifo = c_args->fifo;
  statistics *stats = c_args->stats;

  while (1)
  {
    pthread_mutex_lock(fifo->mut);
    while (fifo->empty)
    {

      // If the queue is empty AND producers are done, exit
      if (fifo->done)
      {
        pthread_mutex_unlock(fifo->mut);
        return NULL;
      }

      // printf("consumer: queue EMPTY.\n");
      empty++;
      pthread_cond_wait(fifo->notEmpty, fifo->mut);
    }

    struct queueItem item;
    queueDel(fifo, &item);

    struct timespec dequeueTime;
    clock_gettime(CLOCK_MONOTONIC, &dequeueTime);

    // Calculate wait time in nanoseconds
    long wait_time_nsec = (dequeueTime.tv_sec - item.timestamp.tv_sec) * 1000000000L +
                          (dequeueTime.tv_nsec - item.timestamp.tv_nsec);

    // Convert to microseconds
    long wait_time_usec = wait_time_nsec / 1000;
    statsAdd(stats, wait_time_usec);
    // printf("Consumer: Wait time: %ld us\n", wait_time_usec);

    pthread_mutex_unlock(fifo->mut);
    pthread_cond_signal(fifo->notFull);

    // ******************  Consume the work  ******************

    struct workFunction wf = item.wf;
    wf.work(wf.arg);

    // *********************************************************
  }

  free(c_args);
  return (NULL);
}

/*
  typedef struct {
  struct queueItem buf[QUEUESIZE];
  long head, tail;
  int full, empty;
  bool done;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
  } queue;
*/

queue *queueInit(void)
{
  queue *q;

  q = (queue *)malloc(sizeof(queue));
  if (q == NULL)
    return (NULL);

  q->done = false;

  q->empty = 1;
  q->full = 0;
  q->head = 0;
  q->tail = 0;
  q->mut = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(q->mut, NULL);
  q->notFull = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
  pthread_cond_init(q->notFull, NULL);
  q->notEmpty = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
  pthread_cond_init(q->notEmpty, NULL);

  return (q);
}

void queueDelete(queue *q)
{
  pthread_mutex_destroy(q->mut);
  free(q->mut);
  pthread_cond_destroy(q->notFull);
  free(q->notFull);
  pthread_cond_destroy(q->notEmpty);
  free(q->notEmpty);
  free(q);
}

void queueAdd(queue *q, struct workFunction in)
{
  struct queueItem item;
  item.wf = in;
  clock_gettime(CLOCK_MONOTONIC, &item.timestamp); // Use CLOCK_MONOTONIC for reliable timing

  q->buf[q->tail] = item;
  q->tail++;

  if (q->tail == QUEUESIZE)
    q->tail = 0;

  if (q->tail == q->head)
    q->full = 1;

  q->empty = 0;

  return;
}

void queueDel(queue *q, struct queueItem *out)
{
  *out = q->buf[q->head];

  q->head++;

  if (q->head == QUEUESIZE)
    q->head = 0;

  if (q->head == q->tail)
    q->empty = 1;

  q->full = 0;

  return;
}

void *computeSines(void *args)
{
  double *angles = (double *)args;
  double result;

  for (int i = 0; i < WORK; i++)
  {
    result = sin(angles[i]);

    // In a real application, we would do something with the results
    // ...
  }

  return NULL;
}

statistics *statsInit(void)
{
  statistics *stats = (statistics *)malloc(sizeof(statistics));
  if (stats == NULL)
    return NULL;

  stats->total_wait_time_usec = 0;
  stats->count = 0;
  stats->mut = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(stats->mut, NULL);

  return stats;
}

void statsDelete(statistics *stats)
{
  pthread_mutex_destroy(stats->mut);
  free(stats->mut);
  free(stats);
}

void statsAdd(statistics *stats, long wait_time_usec)
{
  pthread_mutex_lock(stats->mut);
  stats->total_wait_time_usec += wait_time_usec;
  stats->count++;
  pthread_mutex_unlock(stats->mut);
}

double statsGetAverage(statistics *stats)
{
  double avg;
  pthread_mutex_lock(stats->mut);
  if (stats->count > 0)
  {
    avg = (double)stats->total_wait_time_usec / stats->count;
  }
  else
  {
    avg = 0.0;
  }
  pthread_mutex_unlock(stats->mut);
  return avg;
}

