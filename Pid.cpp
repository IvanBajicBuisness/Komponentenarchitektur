#include <iostream>
#include <sys/wait.h>
#include <unistd.h>


int main()
{

    pid_t pid = fork();

    if(pid < 0)
    {
        std::cout << "Fork failed." << std::endl;
        return 0;
    }

    else
    {
        if( pid == 0)
        {
            //child
            std::cout << "Child ID from child: " << getpid() << std::endl;
            std::cout << "Parent ID from child: " << getppid() << std::endl;
            sleep(2);      //Force termination violation 
            std::cout << "Parent ID from child after termination violation: " << getppid() << std::endl;
            return 0;
        }

        else
        {
            //parent
            std::cout << "Parent ID from parent: " << getpid() << std::endl;
            std::cout << "Child ID from parent: " << pid << std::endl;   //Bei Fork bekommt parent in Pid Wert von Child gespeichert.
            std::cout << "Process over parent ID from parent: " << getppid() << std::endl;
            //wait(NULL);
        }
    }

    return 0;
}
