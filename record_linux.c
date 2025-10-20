#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <time.h>
#include <string.h>

// ATTENTION: Remplacez par le chemin de votre clavier (Ex: /dev/input/event3)
#define EVENT_DEVICE "/dev/input/eventX" 
#define OUTPUT_FILENAME "macro.txt"

// Touches à surveiller (Codes clavier Linux, e.g., KEY_1=2, KEY_2=3, KEY_3=4, KEY_5=6)
// Ces codes proviennent de <linux/input.h>
#define TARGET_KEY_COUNT 4
const unsigned short TARGET_CODES[] = {KEY_1, KEY_2, KEY_3, KEY_5}; 

// Structure pour stocker l'état d'une touche pressée
typedef struct {
    unsigned short code;
    double pressTime;
} KeyState;

KeyState pressedKeys[TARGET_KEY_COUNT] = {0};

// Fonction pour obtenir le temps en secondes (double, CLOCK_MONOTONIC pour l'offset de temps)
double get_current_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

// Fonctions utilitaires find/insert (simples pour C)
int find_key(unsigned short code) {
    for (int i = 0; i < TARGET_KEY_COUNT; i++) {
        if (pressedKeys[i].code == code) return i;
    }
    return -1;
}

int insert_key(unsigned short code, double pressTime) {
    for (int i = 0; i < TARGET_KEY_COUNT; i++) {
        if (pressedKeys[i].code == 0) { 
            pressedKeys[i].code = code;
            pressedKeys[i].pressTime = pressTime;
            return i;
        }
    }
    return -1; 
}

int main(int argc, char *argv[]) {
    int fd;
    struct input_event ev;
    FILE *fp;
    char device_path[256] = EVENT_DEVICE;

    if (argc > 1) {
        strncpy(device_path, argv[1], sizeof(device_path) - 1);
        device_path[sizeof(device_path) - 1] = '\0';
    } else if (strcmp(device_path, EVENT_DEVICE) == 0) {
        printf("ATTENTION: Le chemin du périphérique n'a pas été spécifié. Utilisation de la valeur par défaut: %s\n", EVENT_DEVICE);
        printf("Veuillez trouver votre clavier dans /dev/input/ et passer le chemin en argument (ex: ./record_linux /dev/input/event3).\n");
        // return 1; // Décommenter ceci si vous voulez forcer l'utilisateur à spécifier le chemin
    }
    
    // Créer ou vider le fichier macro
    fp = fopen(OUTPUT_FILENAME, "w");
    if (fp == NULL) {
        perror("Erreur: Impossible de créer le fichier macro.txt");
        return 1;
    }
    fclose(fp);

    // Ouvrir le périphérique d'entrée en lecture seule
    fd = open(device_path, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Erreur: Impossible d'ouvrir %s.\n", device_path);
        fprintf(stderr, "Assurez-vous d'avoir les permissions (Ex: Exécuter avec 'sudo').\n");
        return 1;
    }

    printf("Début de l'enregistrement (Linux EVDEV). Périphérique: %s\n", device_path);
    printf("Appuyez sur la touche 'Échap' (code %u) pour arrêter.\n", KEY_ESC);
    
    // Boucle de lecture des événements
    while (1) {
        if (read(fd, &ev, sizeof(struct input_event)) == -1) {
            perror("Erreur de lecture du périphérique d'entrée");
            break;
        }

        // Seuls les événements de clavier (EV_KEY) nous intéressent
        if (ev.type == EV_KEY) {
            unsigned short code = ev.code;
            int is_target = 0;

            // Vérifier si la touche est une cible
            for (int i = 0; i < TARGET_KEY_COUNT; i++) {
                if (code == TARGET_CODES[i]) {
                    is_target = 1;
                    break;
                }
            }
            
            double currentTime = get_current_time();
            int index = find_key(code);
            
            if (is_target) {
                if (ev.value == 1) { // Touche Pressée
                    if (index == -1) {
                        insert_key(code, currentTime);
                        printf("Touche code %u pressée.\n", code);
                    }
                } else if (ev.value == 0) { // Touche Relâchée
                    if (index != -1) {
                        double pressTime = pressedKeys[index].pressTime;
                        double duration = currentTime - pressTime;
                        
                        printf("Touche code %u relâchée après %.4f secondes.\n", code, duration);
                        
                        // Enregistrer dans le fichier (mode "a" pour append)
                        fp = fopen(OUTPUT_FILENAME, "a");
                        if (fp != NULL) {
                            // Format: scancode_linux,temps_debut,temps_fin
                            fprintf(fp, "%u,%.9f,%.9f\n", code, pressTime, currentTime);
                            fclose(fp);
                        } else {
                            perror("Erreur d'écriture dans le fichier");
                        }
                        
                        // Réinitialiser l'état
                        pressedKeys[index].code = 0;
                    }
                }
            }

            // Arrêt sur Échap (touche Échap = KEY_ESC)
            if (code == KEY_ESC && ev.value == 1) {
                printf("Arrêt demandé (Échap).\n");
                break;
            }
        }
    }

    close(fd);
    printf("Enregistrement terminé.\n");
    return 0;
}