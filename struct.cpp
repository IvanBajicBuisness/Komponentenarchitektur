#include <iostream>
#include <unistd.h>
#include <wait.h>
#include <string.h>
#include <fcntl.h>


struct Person
{
    const char* name;
    int length;
};

int main()
{
    int fd[2];
    pipe2(fd, O_DIRECT);

    pid_t pid = fork();

    if( pid < 0)
    {
        std::cout << "Fork rejected." << std::endl;
        return 0;
    }

    else
    {
        if(pid == 0)
        {
            //child
            Person buffer;
            
            read(fd[0], &buffer, sizeof(buffer));
            
            buffer.length = strlen(buffer.name);

            std::cout << "From child: Name:" << buffer.name << " length: " << buffer.length <<std::endl;

            //write(fd[1], &buffer, sizeof(buffer));
        }

        else
        {
            //parent
            Person person;
            person.name = "Ivan";

            write(fd[1], &person, sizeof(person));

            //read(fd[0], &person, sizeof(person));

            //std::cout << "From parent: Name:" << person.name << " length: " << person.length <<std::endl;

            wait(NULL);

            close(fd[0]);
            close(fd[1]);
        }
    }

    return 0;
}


