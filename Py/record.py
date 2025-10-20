# record.py
import time
from pynput import keyboard

# Touches à surveiller
TARGET_KEYS = {'1', '2', '3', '5'}

# Dictionnaire pour stocker le moment où une touche est pressée
pressed_keys = {}

# Nom du fichier de sortie
output_filename = "macro.txt"

print("Début de l'enregistrement... Appuyez sur 'Échap' pour arrêter.")

# Créer ou vider le fichier macro au début
with open(output_filename, "w") as f:
    f.write("") # Crée le fichier ou le vide s'il existe déjà

def on_press(key):
    try:
        # Convertir la touche en caractère simple
        key_char = key.char
    except AttributeError:
        # Gérer les touches spéciales (comme Échap)
        if key == keyboard.Key.esc:
            print("Enregistrement terminé.")
            # Arrête l'écouteur
            return False
        return # Ignorer les autres touches spéciales

    # Si la touche est une de celles que nous surveillons et qu'elle n'est pas déjà pressée
    if key_char in TARGET_KEYS and key_char not in pressed_keys:
        # Enregistrer le temps de début
        pressed_keys[key_char] = time.time()
        print(f"Touche '{key_char}' pressée.")

def on_release(key):
    try:
        key_char = key.char
    except AttributeError:
        return # Ignorer les touches spéciales au relâchement

    # Si la touche relâchée est une de celles que nous surveillons
    if key_char in TARGET_KEYS and key_char in pressed_keys:
        start_time = pressed_keys.pop(key_char) # Récupère le temps de début et retire la touche
        end_time = time.time()
        duration = end_time - start_time
        
        print(f"Touche '{key_char}' relâchée après {duration:.4f} secondes.")
        
        # Enregistrer dans le fichier
        with open(output_filename, "a") as f:
            # Format: touche,temps_debut,temps_fin
            f.write(f"{key_char},{start_time},{end_time}\n")

# Démarrer l'écoute des événements clavier
with keyboard.Listener(on_press=on_press, on_release=on_release) as listener:
    listener.join()