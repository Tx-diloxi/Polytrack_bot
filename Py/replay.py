# replay.py
import time
from pynput.keyboard import Controller, Key

# Nom du fichier à lire
input_filename = "macro.txt"

# Crée un contrôleur de clavier pour simuler les pressions
keyboard = Controller()

print("Lecture de la macro dans 3 secondes...")
time.sleep(3)

try:
    with open(input_filename, 'r') as f:
        macro_data = f.readlines()
except FileNotFoundError:
    print(f"Erreur : Le fichier '{input_filename}' n'a pas été trouvé.")
    exit()

if not macro_data:
    print("Le fichier macro est vide. Rien à jouer.")
    exit()

# Créer une chronologie de tous les événements (pressions et relâchements)
timeline = []
for line in macro_data:
    parts = line.strip().split(',')
    if len(parts) == 3:
        key, start_time, end_time = parts
        start_time, end_time = float(start_time), float(end_time)
        
        # Ajouter l'événement de pression et de relâchement à la chronologie
        timeline.append({'time': start_time, 'action': 'press', 'key': key})
        timeline.append({'time': end_time, 'action': 'release', 'key': key})

# Trier la chronologie par temps
timeline.sort(key=lambda x: x['time'])

# Obtenir le temps de départ pour synchroniser la lecture
start_offset = timeline[0]['time']
last_event_time = start_offset

print("Début de la lecture de la macro.")

# Parcourir la chronologie et exécuter chaque action
for event in timeline:
    # Calculer le temps à attendre avant le prochain événement
    delay = event['time'] - last_event_time
    time.sleep(delay)
    
    key_to_play = event['key']
    
    # Exécuter l'action
    if event['action'] == 'press':
        print(f"Action : Presser '{key_to_play}'")
        keyboard.press(key_to_play)
    elif event['action'] == 'release':
        print(f"Action : Relâcher '{key_to_play}'")
        keyboard.release(key_to_play)
        
    # Mettre à jour le temps du dernier événement
    last_event_time = event['time']

print("Lecture de la macro terminée.")