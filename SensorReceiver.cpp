#include <iostream>
#include <wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

// 3 Prozesse, 3 Threads 
// O_NONBLOCK: read() blockiert nicht.
// Keine Daten verfügbar -> read() liefert -1 (errno = EAGAIN).
// n wird bei jedem read() aktualisiert.
// Nur wenn n > 0, werden tatsächlich gelesene Bytes ausgegeben.
// Der Buffer bleibt bei EAGAIN unverändert.

int main()
{
    int pipe_Hitze[2], pipe_Druck[2];

    pipe2(pipe_Druck, O_NONBLOCK | O_DIRECT);
    pipe2(pipe_Hitze, O_NONBLOCK | O_DIRECT);

    pid_t pid_Hitze = fork();

    if(pid_Hitze < 0)
    {
        std::cout << "Heat sensor error." << std::endl;
        return 1;
    }

    else
    {
        if(pid_Hitze == 0)
        {
            //Sensor Hitze

            close(pipe_Druck[0]);
            close(pipe_Druck[1]);
            close(pipe_Hitze[0]);

            const char* string = "Hitze\n";

            while(true)
            {
                write(pipe_Hitze[1], string, strlen(string)+1);
                sleep(7);
            }

            close(pipe_Hitze[1]);
            return 0;
        }

        else
        {
            //parent
            pid_t pid_Druck = fork();   // erst hier noch mal forken!!

            if(pid_Druck < 0)
            {
                std::cout << "Pressure sensor error." << std::endl;
                return 1;
            }

            else if(pid_Druck == 0)
            {
                //Sensor Druck
                close(pipe_Druck[0]);
                close(pipe_Hitze[0]);
                close(pipe_Hitze[1]);

                const char* string = "Druck\n";

                while(true)
                {
                    write(pipe_Druck[1], string, strlen(string)+1);
                    sleep(5);
                }

                close(pipe_Druck[1]);

                return 0;
            }
            
            //restlicher parent Block
            else
            {
                close(pipe_Druck[1]);
                close(pipe_Hitze[1]);

                char buffer[100];
                ssize_t n;
                
                while(true)
                {
                    n = read(pipe_Hitze[0], &buffer, sizeof(buffer));  // leerer read -> -1 ; read aus Pipe!! bytestream

                    if(n > 0)
                    {
                        write(1, &buffer, n);  //nur schreiben wenn neu gelesen wurde!!
                    }

                    n = read(pipe_Druck[0], &buffer, sizeof(buffer));

                    if(n > 0)
                    {
                        write(1, &buffer, n);  //nur schreiben wenn neu gelesen wurde!!
                    }
                }

                close(pipe_Druck[0]);
                close(pipe_Hitze[0]);

                wait(NULL);

                return 0;
            }
            
        }
    }
}
