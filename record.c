#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

// Touches à surveiller (Virtual Key Codes pour '1', '2', '3', '5')
// VK_NUMPAD1 (0x61), VK_NUMPAD2 (0x62), VK_NUMPAD3 (0x63), VK_NUMPAD5 (0x65)
// VK_1 (0x31), VK_2 (0x32), VK_3 (0x33), VK_5 (0x35)
#define TARGET_KEY_COUNT 4
const int TARGET_VKS[] = {0x31, 0x32, 0x33, 0x35}; // Virtual Key Codes for '1', '2', '3', '5'

// Structure pour stocker l'état d'une touche pressée
typedef struct {
    int vkCode;
    double pressTime;
} KeyState;

// Dictionnaire simple pour stocker l'état (max 4 touches pour simplifier)
KeyState pressedKeys[TARGET_KEY_COUNT] = {0};
const char* OUTPUT_FILENAME = "macro.txt";

// Fonction utilitaire pour obtenir le temps en secondes (double)
double get_current_time() {
    // Utiliser GetTickCount64 ou une haute résolution pour un meilleur timing
    return (double)GetTickCount64() / 1000.0;
}

// Fonction utilitaire pour trouver une touche dans le tableau
int find_key(int vkCode) {
    for (int i = 0; i < TARGET_KEY_COUNT; i++) {
        if (pressedKeys[i].vkCode == vkCode) {
            return i; // Trouvé
        }
    }
    return -1; // Non trouvé
}

// Fonction utilitaire pour insérer une nouvelle touche
int insert_key(int vkCode, double pressTime) {
    for (int i = 0; i < TARGET_KEY_COUNT; i++) {
        if (pressedKeys[i].vkCode == 0) { // Emplacement vide
            pressedKeys[i].vkCode = vkCode;
            pressedKeys[i].pressTime = pressTime;
            return i;
        }
    }
    return -1; // Plein
}

// Procédure de la fonction de hook (appelée par le système)
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        KBDLLHOOKSTRUCT *pKeyStruct = (KBDLLHOOKSTRUCT *)lParam;
        int vkCode = pKeyStruct->vkCode;
        int isTarget = 0;
        
        // Vérifier si c'est une touche cible
        for (int i = 0; i < TARGET_KEY_COUNT; i++) {
            if (vkCode == TARGET_VKS[i]) {
                isTarget = 1;
                break;
            }
        }
        
        if (isTarget) {
            double currentTime = get_current_time();
            int index = find_key(vkCode);

            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                // Touche PRESSÉE
                if (index == -1) { // Si la touche n'est pas déjà enregistrée comme pressée
                    insert_key(vkCode, currentTime);
                    printf("Touche '%d' pressée.\n", vkCode);
                }
            } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
                // Touche RELÂCHÉE
                if (index != -1) { // Si la touche est enregistrée comme pressée
                    double pressTime = pressedKeys[index].pressTime;
                    double duration = currentTime - pressTime;
                    
                    // Récupère le caractère pour l'affichage (optionnel, mais pratique)
                    char key_char = (char)vkCode;
                    if (vkCode >= 0x30 && vkCode <= 0x39) { // Nombres 0-9
                        key_char = vkCode;
                    } else {
                        key_char = '?';
                    }

                    printf("Touche '%c' (VK:%d) relâchée après %.4f secondes.\n", key_char, vkCode, duration);
                    
                    // Enregistrer dans le fichier (mode "a" pour append)
                    FILE *fp = fopen(OUTPUT_FILENAME, "a");
                    if (fp != NULL) {
                        // Format: vkCode,temps_debut,temps_fin
                        fprintf(fp, "%d,%.8f,%.8f\n", vkCode, pressTime, currentTime);
                        fclose(fp);
                    }
                    
                    // Réinitialiser l'état
                    pressedKeys[index].vkCode = 0;
                    pressedKeys[index].pressTime = 0.0;
                }
            }
        }
        
        // Gérer la touche Échap pour arrêter l'enregistrement
        if (vkCode == VK_ESCAPE && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
            printf("Touche 'Échap' pressée. Arrêt de l'enregistrement.\n");
            PostQuitMessage(0); // Signal pour arrêter la boucle de message
        }
    }
    
    // Passer l'événement au prochain hook de la chaîne
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

int main() {
    // Créer ou vider le fichier macro au début
    FILE *fp = fopen(OUTPUT_FILENAME, "w");
    if (fp != NULL) {
        fclose(fp);
    } else {
        fprintf(stderr, "Erreur: Impossible de créer le fichier %s\n", OUTPUT_FILENAME);
        return 1;
    }

    printf("Début de l'enregistrement (Windows)... Appuyez sur 'Échap' pour arrêter.\n");
    printf("Les touches surveillées sont : 1, 2, 3, 5.\n");

    // Installer le hook clavier
    HHOOK hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);

    if (hHook == NULL) {
        fprintf(stderr, "Erreur: Impossible d'installer le hook clavier.\n");
        return 1;
    }

    // Boucle de message pour maintenir le hook actif
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Désinstaller le hook à l'arrêt
    UnhookWindowsHookEx(hHook);
    printf("Enregistrement terminé.\n");

    return 0;
}