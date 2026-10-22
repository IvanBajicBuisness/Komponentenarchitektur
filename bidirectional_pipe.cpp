#include <iostream>
#include <unistd.h>
#include <wait.h>
#include <string.h>
#include <fcntl.h>

// Bidirectional pipe with harmonized send-receive cardinality. 
// Parent sends a c-string, the child returns the size.

int main()
{
    int fd[2];
    pipe2(fd, O_DIRECT);

    pid_t pid = fork();

    if(pid < 0)
    {
        std::cout << "Fork fail" << std::endl;
    }

    else
    {
        if(pid == 0)
        {
            //child
            int size;
            char buffer[100];

            read(fd[0], &buffer, sizeof(buffer));
            int length = strlen(buffer);

            //Konsolenausgabe des Wortes
            write(1, &buffer, length);

            write(fd[1], &length, sizeof(length));
        }

        else
        {
            //parent
            const char* string = "Ivan ist cool!";
            int length = 0;

            write(fd[1], string, strlen(string)+1);

            sleep(1);  // Nur um Fkt zu demonstrieren

            read(fd[0], &length, sizeof(length));

            std::cout << "Length from parent: " << length << std::endl;

            wait(NULL);
        }
    }
}

/*
1. Warum wird 1819043144 ausgegeben?

Der Parent liest seine eigene Nachricht "Hello!" aus der Pipe,
bevor das Child sie empfangen kann.

Da read() nur sizeof(int) = 4 Bytes liest, werden die ersten
vier Zeichen "Hell" als Integer interpretiert.

Auf einem Little-Endian-System entspricht das:
0x6C6C6548 = 1819043144.

2. Warum beendet sich das Programm nicht?

Durch O_DIRECT arbeitet die Pipe im Paketmodus.
Da der Parent nur 4 Bytes des 7-Byte-Pakets liest,
werden die restlichen Bytes verworfen.

Das Child wartet anschließend mit read() auf neue Daten,
die nicht mehr kommen. Modus Blocking.

Gleichzeitig wartet der Parent mit wait(NULL) auf das Child.

Ergebnis: Deadlock.

O_DIRECT garantiert Paketgrenzen, aber keine Zuordnung
der Nachrichten zu den jeweiligen Empfängern.

Mit sleep funktionierts!
*/
