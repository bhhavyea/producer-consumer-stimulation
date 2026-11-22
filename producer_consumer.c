#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#define BUFFER_SIZE 5
#define ITEMS_TO_PRODUCE 10

static int buffer[BUFFER_SIZE];
static int in_pos = 0;
static int out_pos = 0;

#ifdef _WIN32
#include <windows.h>

static CRITICAL_SECTION buffer_mutex;
static HANDLE empty_slots;
static HANDLE filled_slots;

static DWORD WINAPI producer(LPVOID arg) {
    (void)arg;

    for (int item = 1; item <= ITEMS_TO_PRODUCE; item++) {
        WaitForSingleObject(empty_slots, INFINITE);

        EnterCriticalSection(&buffer_mutex);
        buffer[in_pos] = item;
        printf("Producer produced: %d at position %d\n", item, in_pos);
        in_pos = (in_pos + 1) % BUFFER_SIZE;
        LeaveCriticalSection(&buffer_mutex);

        ReleaseSemaphore(filled_slots, 1, NULL);
        Sleep(150);
    }

    return 0;
}

static DWORD WINAPI consumer(LPVOID arg) {
    (void)arg;

    for (int count = 1; count <= ITEMS_TO_PRODUCE; count++) {
        WaitForSingleObject(filled_slots, INFINITE);

        EnterCriticalSection(&buffer_mutex);
        int item = buffer[out_pos];
        printf("Consumer consumed: %d from position %d\n", item, out_pos);
        out_pos = (out_pos + 1) % BUFFER_SIZE;
        LeaveCriticalSection(&buffer_mutex);

        ReleaseSemaphore(empty_slots, 1, NULL);
        Sleep(250);
    }

    return 0;
}

int main(void) {
    InitializeCriticalSection(&buffer_mutex);
    empty_slots = CreateSemaphore(NULL, BUFFER_SIZE, BUFFER_SIZE, NULL);
    filled_slots = CreateSemaphore(NULL, 0, BUFFER_SIZE, NULL);

    if (empty_slots == NULL || filled_slots == NULL) {
        fprintf(stderr, "Error creating semaphores.\n");
        DeleteCriticalSection(&buffer_mutex);
        if (empty_slots != NULL) CloseHandle(empty_slots);
        if (filled_slots != NULL) CloseHandle(filled_slots);
        return EXIT_FAILURE;
    }

    HANDLE producer_thread = CreateThread(NULL, 0, producer, NULL, 0, NULL);
    HANDLE consumer_thread = CreateThread(NULL, 0, consumer, NULL, 0, NULL);

    if (producer_thread == NULL || consumer_thread == NULL) {
        fprintf(stderr, "Error creating threads.\n");
        if (producer_thread != NULL) CloseHandle(producer_thread);
        if (consumer_thread != NULL) CloseHandle(consumer_thread);
        CloseHandle(empty_slots);
        CloseHandle(filled_slots);
        DeleteCriticalSection(&buffer_mutex);
        return EXIT_FAILURE;
    }

    HANDLE threads[] = { producer_thread, consumer_thread };
    WaitForMultipleObjects(2, threads, TRUE, INFINITE);

    CloseHandle(producer_thread);
    CloseHandle(consumer_thread);
    CloseHandle(empty_slots);
    CloseHandle(filled_slots);
    DeleteCriticalSection(&buffer_mutex);

    printf("\nSimulation completed successfully.\n");
    return EXIT_SUCCESS;
}

#else
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

static sem_t empty_slots;
static sem_t filled_slots;
static pthread_mutex_t mutex;

static void *producer(void *arg) {
    (void)arg;

    for (int item = 1; item <= ITEMS_TO_PRODUCE; item++) {
        sem_wait(&empty_slots);
        pthread_mutex_lock(&mutex);
        buffer[in_pos] = item;
        printf("Producer produced: %d at position %d\n", item, in_pos);
        in_pos = (in_pos + 1) % BUFFER_SIZE;
        pthread_mutex_unlock(&mutex);
        sem_post(&filled_slots);
        usleep(150000);
    }

    return NULL;
}

static void *consumer(void *arg) {
    (void)arg;

    for (int count = 1; count <= ITEMS_TO_PRODUCE; count++) {
        sem_wait(&filled_slots);
        pthread_mutex_lock(&mutex);
        int item = buffer[out_pos];
        printf("Consumer consumed: %d from position %d\n", item, out_pos);
        out_pos = (out_pos + 1) % BUFFER_SIZE;
        pthread_mutex_unlock(&mutex);
        sem_post(&empty_slots);
        usleep(250000);
    }

    return NULL;
}

int main(void) {
    pthread_t producer_thread, consumer_thread;

    sem_init(&empty_slots, 0, BUFFER_SIZE);
    sem_init(&filled_slots, 0, 0);
    pthread_mutex_init(&mutex, NULL);

    if (pthread_create(&producer_thread, NULL, producer, NULL) != 0 ||
        pthread_create(&consumer_thread, NULL, consumer, NULL) != 0) {
        fprintf(stderr, "Error creating threads.\n");
        return EXIT_FAILURE;
    }

    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);
    sem_destroy(&empty_slots);
    sem_destroy(&filled_slots);
    pthread_mutex_destroy(&mutex);

    printf("\nSimulation completed successfully.\n");
    return EXIT_SUCCESS;
}
#endif
