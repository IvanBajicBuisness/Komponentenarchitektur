#include <iostream>
#include <unistd.h>
#include <wait.h>
#include <string.h>

int main()
{
    int fd[2];
    pipe(fd);   //pipe vor fork!!

    pid_t pid = fork();

    if(pid < 0)
    {
        std::cout << "Forking process failed" << std::endl;
    }

    else
    {
        if(pid == 0)
        {
            //child
            close(fd[1]);  // 0 read

            char buffer[100];

            read(fd[0], &buffer, sizeof(buffer));

            write(1, &buffer, 8);

            close(fd[0]);

            return 0;
        }

        else
        {
            //parent
            close(fd[0]);  // 1 write

            const char* string = "Hallo!";

            write(fd[1], string, strlen(string)+1);

            close(fd[1]);

            wait(NULL);
        }
    }

    return 0;
}
