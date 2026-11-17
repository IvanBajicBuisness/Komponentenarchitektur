#include <pthread.h>    // Threads und Mutex
#include <semaphore.h>  // Semaphoren: sem_init, sem_wait, sem_post
#include <unistd.h>     // usleep()
#include <iostream>     // Ausgabe

// Die Queue darf höchstens 6 Elemente gleichzeitig enthalten.
constexpr int MAX_ELEMENTE = 6;

// Insgesamt werden 10000 Messages geschrieben und gelesen.
constexpr int ANZAHL_MESSAGES = 10000;

// Für jeden Versuch nur EINEN Schutz auf false setzen.
constexpr bool UNDERFLOW_SCHUTZ = true;
constexpr bool OVERFLOW_SCHUTZ  = true;
constexpr bool MUTEX_SCHUTZ     = true;


// Ein Knoten der dynamisch verketteten Queue.
struct Person
{
    const char* name; // Zeiger auf den Namen, hier immer "Heinz"
    int number;       // Fortlaufende Nummer: 0, 1, 2, ...
    Person* nextPtr;  // Adresse des nächsten Knotens
};


struct Queue
{
    Person* mHeadPtr = nullptr; // Erstes Element: Hier wird entnommen
    Person* mTailPtr = nullptr; // Letztes Element: Hier wird angehängt
    int count = 0;              // Momentane Anzahl der Elemente

    void push(const char* name, int number)
    {
        // Einen neuen Knoten im dynamischen Speicher erzeugen.
        // Sein nextPtr zeigt zunächst auf keinen weiteren Knoten.
        Person* new_person = new Person{name, number, nullptr};

        if (mTailPtr == nullptr) // Die Queue war bisher leer
        {
            // Der neue Knoten ist zugleich das erste Element.
            mHeadPtr = new_person;
        }
        else
        {
            // Den bisherigen letzten Knoten mit dem neuen verbinden.
            mTailPtr->nextPtr = new_person;
        }

        // Der neue Knoten ist jetzt in jedem Fall der letzte.
        mTailPtr = new_person;
        count++;
    }

    bool pop(const char*& name, int& number)
    {
        if (mHeadPtr == nullptr) // Die Queue ist leer
        {
            return false;
        }

        // Adresse des ersten Knotens merken.
        // Nach dem Verschieben von mHeadPtr brauchen wir sie für delete.
        Person* old_person = mHeadPtr;

        // Die Werte an den Aufrufer zurückgeben.
        // Durch die Referenzen (&) werden name und number in main() geändert.
        name = old_person->name;
        number = old_person->number;

        // Der bisher zweite Knoten wird zum neuen ersten Knoten.
        mHeadPtr = old_person->nextPtr;

        if (mHeadPtr == nullptr) // Das entfernte Element war das letzte
        {
            // Eine leere Queue hat auch keinen letzten Knoten.
            mTailPtr = nullptr;
        }

        // Nur den entfernten Knoten löschen.
        // Der neue erste Knoten bleibt erhalten.
        delete old_person;
        count--;

        return true;
    }
};


// Dieses Objekt wird von main() und producer() gemeinsam verwendet.
struct Shared
{
    Queue queue;

    // Anzahl der noch verfügbaren Plätze.
    // Startwert: MAX_ELEMENTE.
    sem_t open_spots;

    // Anzahl der zum Lesen gemeldeten Elemente.
    // Startwert: 0, weil die Queue anfangs leer ist.
    sem_t existing_elements;

    // Schützt die gemeinsamen Queue-Daten während push() und pop().
    pthread_mutex_t mutex;

    // Größte während des Tests beobachtete Queue-Größe.
    int peak_count = 0;
};


// Diese Funktion läuft im zusätzlich erzeugten Producer-Thread.
// pthread verlangt als Argument und Rückgabewert jeweils void*.
void* producer(void* argument)
{
    // argument enthält die Adresse des Shared-Objekts aus main().
    // Wir wandeln den allgemeinen void*-Zeiger in Shared* zurück.
    // Es wird dabei keine Kopie des Objekts erstellt.
    Shared* shared = static_cast<Shared*>(argument);

    for (int i = 0; i < ANZAHL_MESSAGES; i++)
    {
        // Im Underflow-Test den Producer etwas bremsen.
        // So versucht der Consumer häufiger, aus einer leeren Queue zu lesen.
        if (!UNDERFLOW_SCHUTZ)
            usleep(100);

        // Overflow-Schutz:
        // Vor push() ein Ticket für einen freien Platz nehmen.
        // Sind alle 6 Plätze belegt, wartet der Producer hier.
        if (OVERFLOW_SCHUTZ)
            sem_wait(&shared->open_spots);

        // Mutual Exclusion:
        // Nur ein Thread darf jetzt die Queue bearbeiten.
        if (MUTEX_SCHUTZ)
            pthread_mutex_lock(&shared->mutex);

        shared->queue.push("Heinz", i);

        // Größte bisher erreichte Queue-Größe merken.
        if (shared->queue.count > shared->peak_count)
            shared->peak_count = shared->queue.count;

        if (MUTEX_SCHUTZ)
            pthread_mutex_unlock(&shared->mutex);

        // Underflow-Schutz:
        // Erst NACH push() melden, dass ein Element gelesen werden kann.
        if (UNDERFLOW_SCHUTZ)
            sem_post(&shared->existing_elements);
    }

    return nullptr; // Producer-Thread ist fertig
}


int main()
{
    // main() ist selbst der Consumer-Thread.
    // Beide Threads verwenden dieses eine Shared-Objekt.
    Shared shared;

    // pshared = 0: Die Semaphore wird zwischen Threads
    // innerhalb desselben Prozesses verwendet.
    //
    // Anfangs sind MAX_ELEMENTE = 6 Plätze frei.
    sem_init(&shared.open_spots, 0, MAX_ELEMENTE);

    // Anfangs liegen 0 Elemente in der Queue.
    sem_init(&shared.existing_elements, 0, 0);

    pthread_mutex_init(&shared.mutex, nullptr);

    pthread_t producerThread;

    // Genau einen zusätzlichen Thread starten.
    // &shared übergibt die Adresse der gemeinsamen Daten.
    if (pthread_create(&producerThread, nullptr, producer, &shared) != 0)
    {
        std::cout << "No thread started" << std::endl;
        return 1;
    }

    int error = 0;       // Messages mit falscher Nummer
    int empty_reads = 0; // pop()-Versuche bei leerer Queue

    // expected wird bewusst NICHT im Schleifenkopf erhöht.
    // Bei fehlgeschlagenem pop() versuchen wir dieselbe Nummer erneut.
    for (int expected = 0; expected < ANZAHL_MESSAGES; )
    {
        const char* name = nullptr;
        int number = -1;

        // Im Overflow-Test den Consumer bremsen.
        // So kann der Producer viele Elemente hintereinander einfügen.
        if (!OVERFLOW_SCHUTZ)
            usleep(100);

        // Underflow-Schutz:
        // Vor pop() ein Ticket für ein vorhandenes Element nehmen.
        // Ist keines vorhanden, wartet der Consumer hier.
        if (UNDERFLOW_SCHUTZ)
            sem_wait(&shared.existing_elements);

        // Exklusiven Zugriff auf die Queue nehmen.
        if (MUTEX_SCHUTZ)
            pthread_mutex_lock(&shared.mutex);

        bool success = shared.queue.pop(name, number);

        if (MUTEX_SCHUTZ)
            pthread_mutex_unlock(&shared.mutex);

        if (!success)
        {
            // Ohne Underflow-Schutz kann pop() die leere Queue antreffen.
            empty_reads++;

            // Beim Test die Wiederholungsrate etwas begrenzen.
            if (!UNDERFLOW_SCHUTZ)
                usleep(10);

            // Zurück zum Schleifenanfang.
            // expected bleibt gleich, weil nichts gelesen wurde.
            continue;
        }

        // Overflow-Schutz:
        // Nach einem ERFOLGREICHEN pop() ist ein Platz frei geworden.
        if (OVERFLOW_SCHUTZ)
            sem_post(&shared.open_spots);

        // Prüfen, ob die Messages in der erwarteten Reihenfolge ankamen.
        if (number != expected)
        {
            error++;

            // Damit der Stresstest nicht Tausende Fehlerzeilen ausgibt.
            if (error <= 5)
            {
                std::cout << "Error. expected: " << expected
                          << " Gotten: " << number << std::endl;
            }
        }

        // Nur die ersten fünf erfolgreich gelesenen Messages anzeigen.
        if (expected < 5)
        {
            std::cout << "Gotten: " << name
                      << " #" << number << std::endl;
        }

        // Erst nach einem erfolgreichen pop() zur nächsten Message gehen.
        expected++;
    }

    // main() wartet, bis der Producer seine Schleife beendet hat.
    // Das steht NACH der Leseschleife, damit main() weiterhin
    // Plätze in der Queue freigeben kann.
    pthread_join(producerThread, nullptr);

    std::cout << "Gepruefte Messages: " << ANZAHL_MESSAGES << '\n';
    std::cout << "Fehler: " << error << '\n';
    std::cout << "Leere Leseversuche: " << empty_reads << '\n';
    std::cout << "Groesste Queue: " << shared.peak_count
              << " (Limit: " << MAX_ELEMENTE << ")\n";
    std::cout << "Elemente am Ende: " << shared.queue.count << '\n';

    // Beide Threads sind fertig: Schutzobjekte freigeben.
    pthread_mutex_destroy(&shared.mutex);
    sem_destroy(&shared.open_spots);
    sem_destroy(&shared.existing_elements);

    return 0;
}
