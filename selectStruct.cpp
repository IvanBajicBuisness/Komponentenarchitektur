#include <iostream>
#include <sys/select.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

struct Person{
    char name[50];
    int age;
};

struct Animal{
    char species[50];
    char group[50];
};

int main()
{
    int pipe_animal[2], pipe_person[2];
    
    pipe(pipe_animal);
    pipe(pipe_person);

    pid_t pid = fork();

    if(pid < 0)
    {
        std::cout << "Fork failed." << std::endl;
        return 1;
    }

    else
    {
        if(pid == 0)
        {
            //child - listens
            close(pipe_animal[1]);
            close(pipe_person[1]);

            fd_set readfds;

            int maxfd;

            int n;
            int counter =0;

            Animal buffer_animal[10];
            Person buffer_person[10];

            if(pipe_animal[0] > pipe_person[0])
            {
                maxfd = pipe_animal[0];
            }

            else
            {
                maxfd = pipe_person[0];
            }

            while(counter < 2)
            {
                FD_ZERO(&readfds);
                FD_SET(pipe_animal[0], &readfds);
                FD_SET(pipe_person[0], &readfds);

                select(maxfd+1, &readfds, NULL, NULL, NULL);

                if(FD_ISSET(pipe_animal[0], &readfds))
                {
                    n = read(pipe_animal[0], &buffer_animal, sizeof(buffer_animal));
                    if(n>0)
                    {
                        write(1, buffer_animal->group, strlen(buffer_animal->group));
                        counter++;
                    }
                }

                if(FD_ISSET(pipe_person[0], &readfds))
                {
                    n = read(pipe_person[0], &buffer_person, sizeof(buffer_person));
                    if(n>0)
                    {
                        write(1, buffer_person->name, strlen(buffer_person->name));
                        counter++;
                    }
                    
                }
            }


            close(pipe_animal[0]);
            close(pipe_person[0]);
            return 0;
        }

        else
        {
            //parent sends
            close(pipe_animal[0]);
            close(pipe_person[0]);

            Person Ivan {"Ivan", 24};

            Animal Dog {"Labrador", "Mammal"};

            sleep(1);

            write(pipe_animal[1], &Dog, sizeof(Dog));

            sleep(3);

            write(pipe_person[1], &Ivan, sizeof(Ivan));


            close(pipe_animal[1]);
            close(pipe_person[1]);

            wait(NULL);
            return 0;
        }
    }
}
