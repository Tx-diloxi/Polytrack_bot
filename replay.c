#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

const char* INPUT_FILENAME = "macro.txt";

// Structure pour un événement clavier (pression ou relâchement)
typedef struct {
    double time;
    int vkCode;
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
    if (seconds <= 0) return;
    
    // Conversion en millisecondes pour Sleep()
    DWORD ms = (DWORD)(seconds * 1000.0);
    
    // Tenter d'améliorer la résolution de la minuterie (optionnel, mais utile)
    // timeBeginPeriod(1);
    
    Sleep(ms);
    
    // timeEndPeriod(1);
}

// Fonction utilitaire pour envoyer un événement clavier
void send_key_event(int vkCode, int is_press) {
    INPUT ip;
    ip.type = INPUT_KEYBOARD;
    ip.ki.wScan = 0; // Pas de scan code
    ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0;
    
    ip.ki.wVk = (WORD)vkCode; // Virtual Key Code
    
    // Définir le flag pour la pression ou le relâchement
    ip.ki.dwFlags = is_press ? 0 : KEYEVENTF_KEYUP;
    
    SendInput(1, &ip, sizeof(INPUT));
}

int main() {
    FILE *fp;
    char line[100];
    KeyEvent *timeline = NULL;
    int timeline_count = 0;
    int timeline_capacity = 10;
    
    printf("Lecture de la macro (Windows) dans 3 secondes...\n");
    sleep_precise(3.0);
    
    // Allocation initiale
    timeline = (KeyEvent *)malloc(timeline_capacity * sizeof(KeyEvent));
    if (timeline == NULL) {
        perror("Erreur d'allocation mémoire");
        return 1;
    }

    // Ouvrir le fichier en lecture
    fp = fopen(INPUT_FILENAME, "r");
    if (fp == NULL) {
        fprintf(stderr, "Erreur : Le fichier '%s' n'a pas été trouvé.\n", INPUT_FILENAME);
        free(timeline);
        return 1;
    }

    // Lire le fichier ligne par ligne
    while (fgets(line, sizeof(line), fp) != NULL) {
        int vkCode;
        double start_time, end_time;
        
        // Tenter d'analyser la ligne: vkCode,temps_debut,temps_fin
        if (sscanf(line, "%d,%lf,%lf", &vkCode, &start_time, &end_time) == 3) {
            // S'assurer qu'il y a assez d'espace pour 2 événements
            if (timeline_count + 2 > timeline_capacity) {
                timeline_capacity *= 2;
                KeyEvent *temp = (KeyEvent *)realloc(timeline, timeline_capacity * sizeof(KeyEvent));
                if (temp == NULL) {
                    perror("Erreur de réallocation mémoire");
                    fclose(fp);
                    free(timeline);
                    return 1;
                }
                timeline = temp;
            }
            
            // Ajouter l'événement de pression
            timeline[timeline_count].time = start_time;
            timeline[timeline_count].vkCode = vkCode;
            timeline[timeline_count].action = PRESS;
            timeline_count++;
            
            // Ajouter l'événement de relâchement
            timeline[timeline_count].time = end_time;
            timeline[timeline_count].vkCode = vkCode;
            timeline[timeline_count].action = RELEASE;
            timeline_count++;
        }
    }

    fclose(fp);

    if (timeline_count == 0) {
        printf("Le fichier macro est vide ou illisible. Rien à jouer.\n");
        free(timeline);
        return 0;
    }

    // Trier la chronologie par temps
    qsort(timeline, timeline_count, sizeof(KeyEvent), compare_events);

    // Synchronisation de la relecture
    double start_offset = timeline[0].time;
    double last_event_time = start_offset;

    printf("Début de la lecture de la macro.\n");

    // Parcourir la chronologie et exécuter chaque action
    for (int i = 0; i < timeline_count; i++) {
        KeyEvent event = timeline[i];
        
        // Calculer le temps à attendre (en secondes)
        double delay = event.time - last_event_time;
        
        // Attendre l'instant précis
        sleep_precise(delay);
        
        // Exécuter l'action
        if (event.action == PRESS) {
            printf("Action : Presser VK:%d\n", event.vkCode);
            send_key_event(event.vkCode, 1); // 1 pour presser
        } else if (event.action == RELEASE) {
            printf("Action : Relâcher VK:%d\n", event.vkCode);
            send_key_event(event.vkCode, 0); // 0 pour relâcher
        }
        
        // Mettre à jour le temps du dernier événement
        last_event_time = event.time;
    }

    printf("Lecture de la macro terminée.\n");

    free(timeline);
    return 0;
}