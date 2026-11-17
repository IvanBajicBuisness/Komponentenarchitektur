#include <iostream>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>

struct Queue {
    int values[5];
    int head = 0;
    int tail = 0;
    int count = 0;

    void push(int value) {
        values[tail] = value;
        tail = (tail + 1) % 5;
        count++;
    }

    int pop() {
        int value = values[head];
        head = (head + 1) % 5;
        count--;
        return value;
    }
};

struct Shared
{
    sem_t open_spots;
    sem_t existing_elements;
    Queue queue;
    pthread_mutex_t mutex;
};


void* thread1Fct(void* argument)
{
    Shared* shared = static_cast<Shared*>(argument);

    for(int i = 1; i <= 10; i++)
    {
        sem_wait(&shared->open_spots);
        pthread_mutex_lock(&shared->mutex);

        shared->queue.push(i);

        pthread_mutex_unlock(&shared->mutex);
        sem_post(&shared->existing_elements);
        usleep(1);
    }

    return 0;
}

void* thread2Fct(void* argument)
{
    Shared* shared = static_cast<Shared*>(argument);

    for(int i = 11; i <= 20; i++)
    {
        sem_wait(&shared->open_spots);
        pthread_mutex_lock(&shared->mutex);

        shared->queue.push(i);

        pthread_mutex_unlock(&shared->mutex);
        sem_post(&shared->existing_elements);

        usleep(1);
    }

    return 0;

}

int main()
{
    Shared shared;

    sem_init(&shared.open_spots, 0, 5);
    sem_init(&shared.existing_elements, 0, 0);
    pthread_mutex_init(&shared.mutex, nullptr);

    pthread_t thread1; 
    pthread_t thread2;

    pthread_create(&thread1, nullptr, thread1Fct, &shared);
    pthread_create(&thread2, nullptr, thread2Fct, &shared);

    int n = 0;

    for(int i = 0; i <= 19; i++)
    {
        sem_wait(&shared.existing_elements);
        pthread_mutex_lock(&shared.mutex);

        n = shared.queue.pop();

        pthread_mutex_unlock(&shared.mutex);
        sem_post(&shared.open_spots);

        std::cout << n << std::endl;
    }

    pthread_join(thread1, nullptr);
    pthread_join(thread2, nullptr);

    sem_destroy(&shared.open_spots);
    sem_destroy(&shared.existing_elements);
    pthread_mutex_destroy(&shared.mutex);

    return 0;
}
