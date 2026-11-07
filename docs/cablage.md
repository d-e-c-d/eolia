# Câblage (Arduino UNO)

## Signaux (fils du kit, breadboard)

| Élément | Broche Arduino | Remarque |
| --- | --- | --- |
| INA219 n°1 (production) SDA / SCL | A4 / A5 | adresse 0x40, VCC 5 V |
| INA219 n°2 (lampe) SDA / SCL | A4 / A5 | adresse 0x41 : souder le pont A0 |
| Capteur Hall A3144 | D2 | aimant collé sur l'axe ; pull-up interne activé |
| L293D IN1 (commande du relais) | D3 | EN1 à 5 V, OUT1 vers la bobine du relais, VS à 5 V |
| LCD1602 D7, D6, D5, D4 | D4, D5, D6, D7 | RW à GND, contraste par potentiomètre 10 kΩ sur V0 |
| LCD1602 E / RS | D11 / D12 | |
| LED rouge | D8 | avec résistance 220 Ω vers GND |
| Buzzer actif | D9 | |
| Bouton poussoir | D10 | vers GND, pull-up interne |
| Thermistance NTC | A0 | 5 V, résistance 10 kΩ, A0, NTC, GND ; NTC collée sur la batterie |

## Puissance (fils 0,75 à 1,5 mm², bornier)

Générateur, fusible 1, diode Schottky, diode TVS en parallèle, **INA219 n°1** (Vin+ puis Vin-), chargeur CC/CV (sortie réglée à 14,4 V au plus), batterie 12 V.

Batterie, fusible 2, **INA219 n°2** (Vin+ puis Vin-), contact du relais, lampe LED 12 V, retour au négatif de la batterie.

Toutes les masses (Arduino, INA219, négatif batterie) sont reliées. La tension mesurée par l'INA219 n°2 sert de tension batterie.

## Ordre de mise en service

1. Mesurer au multimètre la tension à vide du générateur au régime du ventilateur (au moins 12 V attendus, pas plus de 25 V pour l'INA219).
2. Téléverser le firmware sans rien brancher : il passe tout seul en simulation (`SIM 1`).
3. Brancher le LCD, la LED, le buzzer et le bouton ; vérifier l'affichage.
4. Brancher les INA219 avec le petit moteur DC d'essai, puis `SIM 0`.
5. Brancher le vrai générateur, le chargeur et la batterie.
