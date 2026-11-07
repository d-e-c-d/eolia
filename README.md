<div align="center">

# Eolia

**Une mini-éolienne qui négocie elle-même l'usage de son énergie.**

[![arduino](https://img.shields.io/badge/Arduino-UNO-00979D)](https://docs.arduino.cc/hardware/uno-rev3/)
[![platformio](https://img.shields.io/badge/PlatformIO-atmelavr-F5822A)](https://platformio.org/)
[![python](https://img.shields.io/badge/Python-3-3776AB)](https://www.python.org/)
[![licence](https://img.shields.io/badge/licence-MIT-5B6472)](LICENSE)

</div>

---

Eolia est le projet **Smart Realization X5** (UCAC-ICAM) : une petite
éolienne transforme le vent en électricité, la stocke dans une batterie
12 V et allume une lampe LED. Un **Arduino UNO** supervise l'ensemble
grâce à **quatre agents** qui négocient entre eux par un bus de messages :

| Agent | Rôle |
|---|---|
| **Production** | Mesure le générateur (INA219 n°1, capteur Hall), publie la puissance, détecte la surtension |
| **Batterie** | Estime l'état de charge (comptage d'énergie recalé sur la tension), surveille tension et température, accorde ou refuse la décharge |
| **Charge** | Porte l'exigence de confort (lampe voulue) et négocie l'allumage avec la batterie |
| **Superviseur** | Mode autonome ou manuel, alarmes (LED, buzzer), écran LCD, commande du relais |

Les agents ne s'appellent jamais directement : ils communiquent
uniquement par le `Bus` (`bus.h`).

## Deux modes de fonctionnement

| Mode | Comportement |
|---|---|
| **Autonome** | Les agents appliquent eux-mêmes leurs décisions |
| **Manuel** | Les agents proposent une action (LCD et PC), l'utilisateur l'accepte ou la refuse (commande `OK` / `NON` ou bouton) |

Dans les deux modes, les **sécurités** (surchauffe, batterie basse,
surtension) restent prioritaires.

## Matériel

| | |
|---|---|
| Carte | Arduino UNO (2 Ko de RAM) |
| Mesure électrique | 2 × INA219 (production à 0x40, lampe à 0x41) |
| Vitesse de rotation | Capteur Hall A3144 + aimant sur l'axe |
| Température batterie | Thermistance NTC 10 kΩ |
| Commande de la lampe | Relais piloté par un L293D |
| Interface | LCD1602, LED rouge, buzzer, bouton poussoir |
| Puissance | Générateur, chargeur CC/CV 14,4 V, batterie plomb 12 V, lampe LED 12 V |

Le câblage complet et l'ordre de mise en service sont dans
[`docs/cablage.md`](docs/cablage.md).

## Ce que contient le dépôt

```
firmware/eolia/
  eolia.ino          boucle principale, commandes série, LCD
  agents.h           les quatre agents et leurs règles de décision
  bus.h              bus de messages entre agents (sans allocation dynamique)
  mesures.h          lecture des capteurs et scénarios de simulation
  config.h           brochage, seuils et coefficients de calibration
outils/
  supervision.py     affichage, enregistrement CSV, commandes, banc d'essai, calibration
tests/
  test_natif.cpp     tests de la logique des agents sur PC
  stubs/             substituts d'Arduino.h, Wire.h, Adafruit_INA219.h
docs/
  cablage.md         brochage et circuit de puissance
  calibration.md     calibration des INA219 et de la NTC au multimètre
  etat_de_charge.md  estimation de l'état de charge et réglage de ses paramètres
  protocole_serie.md trames Arduino vers PC et commandes PC vers Arduino
platformio.ini       compilation avec PlatformIO
```

## Démarrage

```bash
# Firmware (PlatformIO)
pio run -t upload
pio device monitor

# Supervision PC
pip install pyserial
python outils/supervision.py --port COM3          # mesures, CSV, commandes
python outils/supervision.py --port COM3 --banc   # banc d'essai automatique
python outils/supervision.py --port COM3 --calib  # calibration au multimètre

# Tests sans matériel
g++ -std=c++17 -Itests/stubs -o test tests/test_natif.cpp && ./test
```

Avec l'IDE Arduino : ouvrir `firmware/eolia/eolia.ino`, installer les
bibliothèques *Adafruit INA219* et *LiquidCrystal*, puis téléverser.

Sans capteurs branchés, le firmware passe tout seul **en simulation**
(`SIM 1` à `SIM 4` : vent fort puis calme, vent faible, rafale,
surchauffe), ce qui permet de travailler le logiciel avant d'avoir le
matériel.

## Critères de la maquette

| Critère | Où il est mesuré |
|---|---|
| Temps de convergence inférieur à 2 s | `cycle_us` dans chaque trame, `duree_max_us` dans `STAT` (de l'ordre de la milliseconde) |
| Charge de communication | `msg_cycle` (messages par cycle), `messages` (total) dans `STAT` |
| Indéterminisme | `SIM n` répété : `empreinte` des décisions identique ; `--banc` le vérifie automatiquement |
| Utilisation de la mémoire | Aucune allocation dynamique, `ram_libre` dans chaque trame |
| Modes autonome et manuel | `MODE AUTO` / `MODE MANU`, propositions `P,...`, réponse `OK` / `NON` ou bouton |

Résultat des tests sur PC (300 cycles par scénario) : 5 messages au plus
par cycle, 6 tours de négociation, aucun message perdu, décisions
identiques sur 3 exécutions pour les 4 scénarios.

Le format des trames est décrit dans
[`docs/protocole_serie.md`](docs/protocole_serie.md).

## État du projet

Projet réalisé en 2026. Le firmware, la supervision PC et les tests sur
PC sont fonctionnels ; la logique des agents est validée en simulation.

Prochaines étapes :

1. Calibrer les INA219 et la NTC au multimètre : l'assistant `--calib` est prêt, la procédure est dans [`docs/calibration.md`](docs/calibration.md).
2. Régler `CAPACITE_BAT_WH` et mesurer `RENDEMENT_CHARGE` sur la vraie batterie (procédure dans [`docs/etat_de_charge.md`](docs/etat_de_charge.md)).
3. Tracer les courbes puissance / vitesse de rotation à partir des CSV pour le poster.

---

<div align="center">
<sub>Par <a href="https://github.com/d-e-c-d">Charles Darryl Dikoume</a></sub>
</div>
