# include <iostream>
#include <pthread.h>
#include <unistd.h>

// 3 Threads, 1 Prozess, 2 Empfaenger, Heap

struct Blackboard
{
    int value = 0;
    pthread_mutex_t mutex;
};

void* thread1Fct(void* argument)
{
    Blackboard* blackboard = static_cast<Blackboard*>(argument);

    int n = 0;

    for(int i = 0; i < 40;i++)
    {
        pthread_mutex_lock(&blackboard->mutex);
        n = blackboard->value;
        pthread_mutex_unlock(&blackboard->mutex);

        std::cout << "Receiver 20 Hz: " << n << std::endl;

        usleep(50000);
    }
    return 0;
}

void* thread2Fct(void* argument)
{
    Blackboard* blackboard = static_cast<Blackboard*>(argument);

    int n = 0;

    for(int i = 0; i < 25;i++)
    {
        pthread_mutex_lock(&blackboard->mutex);
        n = blackboard->value;
        pthread_mutex_unlock(&blackboard->mutex);

        std::cout << "Receiver 12.5 Hz: " << n << std::endl;

        usleep(80000);
    }
    return 0;

}



int main()
{
    Blackboard* blackboard = new Blackboard{};

    pthread_t thread1, thread2;

    pthread_mutex_init(&blackboard->mutex, nullptr);

    pthread_create(&thread1, nullptr, thread1Fct, blackboard);
    pthread_create(&thread2, nullptr, thread2Fct, blackboard);

    for(int i = 0; i < 20; i++)
    {
        pthread_mutex_lock(&blackboard->mutex);

        blackboard->value = i;
        
        pthread_mutex_unlock(&blackboard->mutex);

        usleep(100000);
    }

    pthread_join(thread1, nullptr);
    pthread_join(thread2, nullptr);

    pthread_mutex_destroy(&blackboard->mutex);
    delete blackboard;

    return 0;
}
