# Calibration des INA219 et de la thermistance

Les INA219 et la NTC sortent d'usine avec des tolérances : shunt de 0,1 Ω à 1 %, NTC à 1 ou 5 %, résistance série à 5 %. La calibration compare leurs mesures à celles d'un multimètre et calcule des coefficients de correction, rangés dans `firmware/eolia/config.h`.

## Principe

Chaque grandeur électrique est corrigée par une droite :

```
valeur corrigée = gain × valeur brute + décalage
```

| Voie | Coefficients | Référence |
| --- | --- | --- |
| Tension générateur (INA219 n°1) | `CAL_VGEN_GAIN`, `CAL_VGEN_DECALAGE` | voltmètre entre Vin+ et la masse |
| Courant générateur (INA219 n°1) | `CAL_IGEN_GAIN`, `CAL_IGEN_DECALAGE` | ampèremètre en série, calibre 10 A |
| Tension batterie (INA219 n°2) | `CAL_VBAT_GAIN`, `CAL_VBAT_DECALAGE` | voltmètre aux bornes de la batterie |
| Courant lampe (INA219 n°2) | `CAL_ILAMPE_GAIN`, `CAL_ILAMPE_DECALAGE` | ampèremètre en série avec la lampe |

Avec un seul point, seul le gain est calculé (décalage nul). Avec deux points ou plus, gain et décalage sont ajustés par moindres carrés ; des points bien écartés donnent un meilleur résultat.

La NTC suit le modèle bêta : `1/T = 1/298,15 + ln(R/R0)/β`. On mesure d'abord la résistance série hors circuit (`NTC_RSERIE`), puis on relève la NTC face à un thermomètre :

- un point : `NTC_R0` est recalculée, `NTC_BETA` garde la valeur du constructeur (3950) ;
- deux points éloignés d'au moins 15 °C : `NTC_BETA` et `NTC_R0` sont ajustés.

La mesure de la NTC est un rapport de tensions : elle ne dépend pas de la tension exacte du 5 V de l'Arduino.

## Matériel

- Multimètre (voltmètre et ampèremètre calibre 10 A), cordons, pinces crocodile.
- Pour la tension générateur : une alimentation réglable (0 à 25 V au plus) ou le générateur entraîné à plusieurs régimes.
- Pour la NTC : un thermomètre, un verre d'eau chaude (environ 50 °C) et un petit sachet étanche pour la NTC.

## Procédure

1. Téléverser le firmware à jour (`pio run -t upload`), capteurs branchés comme dans [cablage.md](cablage.md).
2. Lancer l'assistant : `python outils/supervision.py --port COM3 --calib`.
3. Pour chaque voie, régler la grandeur, attendre que le multimètre soit stable, taper sa valeur puis Entrée. L'assistant envoie alors `CAL` et relève la valeur brute correspondante (moyenne de 16 lectures). Entrée seule passe à la voie suivante ; une voie sans point garde ses coefficients.
   - **Courant générateur** : premier point générateur à l'arrêt (taper `0`), puis un ou deux points en charge.
   - **Courant lampe** : l'assistant prend lui-même le point lampe éteinte, puis passe en mode manuel et allume la lampe (`FORCER 1`). Si la lampe reste éteinte, une sécurité est active (batterie basse, surchauffe) : la recharger d'abord. L'assistant éteint la lampe et revient en mode autonome à la fin.
4. NTC : taper la résistance série mesurée hors circuit, puis la température du thermomètre au contact de la NTC (ambiante, puis eau chaude).
5. L'assistant affiche les coefficients, l'écart maximal restant et propose de les écrire dans `config.h`. Recompiler et téléverser.
6. Vérifier : relancer la supervision normale et comparer quelques mesures au multimètre.

Les points relevés sont enregistrés dans un fichier `calibration_AAAAMMJJ_HHMMSS.csv` (horodatage, voie, valeur brute, référence), à joindre au rapport.

## Valeurs attendues

| Coefficient | Plage normale | Si hors plage |
| --- | --- | --- |
| Gains INA219 | 0,95 à 1,05 | calibre du multimètre, contact des cordons, Vin+ et Vin- inversés |
| Décalage tension | quelques dizaines de mV | masse commune absente |
| Décalage courant | quelques mA | bruit : refaire le point à courant nul |
| `NTC_BETA` | 3500 à 4300 K | mauvais contact NTC et thermomètre, température pas stabilisée |
| `NTC_R0` | 9 à 11 kΩ | `NTC_RSERIE` mal mesurée |

L'INA219 utilise sa configuration par défaut (32 V, shunt 0,1 Ω, environ 3,2 A au plus). Si le générateur dépasse 3 A, il faut changer le shunt ou le réglage de l'INA219 avant de calibrer.
