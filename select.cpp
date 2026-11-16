#include <iostream>
#include <wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

// 3 Prozesse, 3 Threads 
//select: vorher mit pipe: viele erfolglose read Aufrufe -> hohe CPU_Last
// jetzt: Thread schläft bis FD bereit

/*
 * PIPE + SELECT – WICHTIGSTE ERKENNTNISSE
 *
 * - pipe(): Standardmäßig blockierend, überträgt einen Bytestrom.
 * - pipe2(O_NONBLOCK): read() wartet nicht; ohne Daten -> -1 (EAGAIN).
 * - pipe2(O_DIRECT): Paketmodus statt reinem Bytestrom.
 *
 * - select(): Überwacht mehrere FDs gleichzeitig auf Bereitschaft.
 * - select() blockiert bei Timeout NULL, solange kein FD bereit ist.
 * - Dadurch kein Busy Waiting wie bei ständigem NONBLOCK-read().
 * - select() liest selbst keine Daten, sondern meldet Bereitschaft.
 *
 * - fd_set: Menge der zu überwachenden Dateideskriptoren.
 * - FD_ZERO(): Menge leeren (vor JEDEM select()-Aufruf).
 * - FD_SET(): FD zur Überwachung hinzufügen.
 * - select(maxfd+1, ...): Höchster überwachter FD + 1.
 * - FD_ISSET(): Prüfen, welcher FD bereit ist.
 * - Zwei getrennte if-Abfragen, da beide Pipes bereit sein können.
 *
 * - read() > 0: Daten erfolgreich gelesen.
 * - read() == 0: EOF, alle Schreibenden geschlossen und Pipe leer.
 * - read() == -1: Fehler (bei NONBLOCK z.B. EAGAIN).
 * - EOF gilt bei select() ebenfalls als Lesebereitschaft!
 *
 * - Pipe offen, aber keine Daten -> select() wartet, kein Busy Waiting.
 * - Pipe geschlossen + EOF ignoriert -> select() kehrt ständig
 *   sofort zurück -> Busy Waiting!
 * - Lösung: EOF erkennen, FD schließen und aus fd_set entfernen.
 *
 * - select() funktioniert auch mit pipe2(O_NONBLOCK).
 * - Bei einfachen Pipes ist O_NONBLOCK mit select() meist unnötig.
 * - select() garantiert nicht, dass ein vollständiges struct
 *   gelesen werden kann (Pipe = Bytestrom).
 */

int main()
{
    int pipe_Hitze[2], pipe_Druck[2];

    pipe(pipe_Druck);
    pipe(pipe_Hitze);

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
                
                fd_set readfds;  //Menge an Dateideskriptoren
                int maxfd; //größerere Dateideskriptor

                if(pipe_Hitze[0] > pipe_Druck[0])
                {
                    maxfd = pipe_Hitze[0];
                }
                else 
                {
                    maxfd = pipe_Druck[0];
                }

                while(true)
                {
                    FD_ZERO(&readfds);  //leert jede Schleife Dateideskriptoren

                    FD_SET(pipe_Hitze[0], &readfds);  //fuegt Leseenden zur Ueberwachung hinzu
                    FD_SET(pipe_Druck[0], &readfds);

                    int ret = select(maxfd+1, &readfds, NULL, NULL, NULL);  //blockiert solange keine pipe lesbar

                    if(ret < 0)
                    {
                        return 1;
                    }

                    if(FD_ISSET(pipe_Hitze[0], &readfds))  //prüft ob Hitze pipe ready ist
                    {
                        n = read(pipe_Hitze[0], &buffer, sizeof(buffer));
                        if(n > 0)
                        {
                            write(1, &buffer, n);
                        }
                    }

                    if(FD_ISSET(pipe_Druck[0], &readfds))
                    {
                        n = read(pipe_Druck[0], &buffer, sizeof(buffer));
                        if(n > 0)
                        {
                            write(1, &buffer, n);
                        }
                    }
                }

                close(pipe_Druck[0]);
                close(pipe_Hitze[0]);

                wait(NULL);
                wait(NULL);

                return 0;
            }
            
        }
    }
}
