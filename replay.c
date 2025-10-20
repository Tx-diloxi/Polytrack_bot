#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

const char* INPUT_FILENAME = "macro.txt";

// Structure pour un événement clavier (pression ou relâchement)
typedef struct {
    double time;
    int code; // C'est le scancode EVDEV, que Xlib peut mapper
    enum { PRESS, RELEASE } action;
} KeyEvent;

// Fonction de comparaison pour qsort (tri par temps)
int compare_events(const void *a, const void *b) {
    KeyEvent *eventA = (KeyEvent *)a;
    KeyEvent *eventB = (KeyEvent *)b;
    if (eventA->time < eventB->time) return -1;
    if (eventA->time > eventB->time) return 1;
    return 0;
}

// Fonction utilitaire pour dormir avec précision (en secondes)
void sleep_precise(double seconds) {
    if (seconds <= 0.0) return;
    
    struct timespec ts;
    ts.tv_sec = (time_t)seconds;
    ts.tv_nsec = (long)((seconds - (double)ts.tv_sec) * 1000000000.0);
    
    // nanosleep est préférable à usleep ou sleep pour la précision
    nanosleep(&ts, NULL);
}

int main() {
    FILE *fp;
    char line[100];
    KeyEvent *timeline = NULL;
    int timeline_count = 0;
    int timeline_capacity = 10;
    Display *display;

    printf("Lecture de la macro (Linux XTest) dans 3 secondes...\n");
    sleep_precise(3.0);
    
    // --- 1. Initialisation X11 ---
    display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "Erreur: Impossible d'ouvrir l'affichage X11. Assurez-vous d'être sur un environnement X.\n");
        return 1;
    }

    // --- 2. Lecture du fichier ---
    timeline = (KeyEvent *)malloc(timeline_capacity * sizeof(KeyEvent));
    if (timeline == NULL) {
        perror("Erreur d'allocation mémoire");
        XCloseDisplay(display);
        return 1;
    }

    fp = fopen(INPUT_FILENAME, "r");
    if (fp == NULL) {
        fprintf(stderr, "Erreur : Le fichier '%s' n'a pas été trouvé. Lancez record_linux d'abord.\n", INPUT_FILENAME);
        free(timeline);
        XCloseDisplay(display);
        return 1;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        int code;
        double start_time, end_time;
        
        // Format: scancode_linux,temps_debut,temps_fin
        if (sscanf(line, "%d,%lf,%lf", &code, &start_time, &end_time) == 3) {
            if (timeline_count + 2 > timeline_capacity) {
                timeline_capacity *= 2;
                KeyEvent *temp = (KeyEvent *)realloc(timeline, timeline_capacity * sizeof(KeyEvent));
                if (temp == NULL) {
                    perror("Erreur de réallocation mémoire");
                    goto cleanup;
                }
                timeline = temp;
            }
            
            timeline[timeline_count++] = (KeyEvent){start_time, code, PRESS};
            timeline[timeline_count++] = (KeyEvent){end_time, code, RELEASE};
        }
    }

    fclose(fp);

    if (timeline_count == 0) {
        printf("Le fichier macro est vide ou illisible. Rien à jouer.\n");
        goto cleanup;
    }

    qsort(timeline, timeline_count, sizeof(KeyEvent), compare_events);

    // --- 3. Synchronisation et Relecture ---
    double start_offset = timeline[0].time;
    double last_event_time = start_offset;

    printf("Début de la lecture de la macro.\n");

    for (int i = 0; i < timeline_count; i++) {
        KeyEvent event = timeline[i];
        
        // Calculer le temps à attendre (en secondes)
        double delay = event.time - last_event_time;
        sleep_precise(delay);
        
        // Convertir le scancode EVDEV en KeyCode X11 (nécessaire pour XTest)
        // La fonction XKeysymToKeycode() pourrait être utilisée si on enregistrait des Keysyms,
        // mais le scancode EVDEV est très proche du KeyCode X11 pour les touches de base.
        // On utilise la fonction XKeysymToKeycode sur l'équivalent Keysym, ou KeyCodeFromKeySym() si on l'avait.
        // Pour une approche simple, on suppose ici que KeyCode X11 est égal au code EVDEV + 8 (offset commun).
        // C'est une simplification qui marche pour de nombreux claviers!
        // Le code le plus précis serait: XKeysymToKeycode(display, XStringToKeysym("1")) etc.
        KeyCode keycode = XKeysymToKeycode(display, XStringToKeysym(
            (event.code == KEY_1) ? "1" :
            (event.code == KEY_2) ? "2" :
            (event.code == KEY_3) ? "3" :
            (event.code == KEY_5) ? "5" : "Return")); // Fallback to 'Return'

        if (keycode == 0) {
            fprintf(stderr, "Avertissement: Code EVDEV %d non mappé en KeyCode X11. Ignoré.\n", event.code);
            last_event_time = event.time;
            continue;
        }

        // Exécuter l'action
        if (event.action == PRESS) {
            XTestFakeKeyEvent(display, keycode, True, CurrentTime);
            printf("Action : Presser code %d (KeyCode %u)\n", event.code, keycode);
        } else if (event.action == RELEASE) {
            XTestFakeKeyEvent(display, keycode, False, CurrentTime);
            printf("Action : Relâcher code %d (KeyCode %u)\n", event.code, keycode);
        }
        
        XFlush(display); // Assurer l'envoi immédiat de l'événement
        last_event_time = event.time;
    }

    printf("Lecture de la macro terminée.\n");

cleanup:
    free(timeline);
    XCloseDisplay(display);
    return 0;
}