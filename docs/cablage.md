# Câblage (Arduino UNO)

## Signaux (fils du kit, breadboard)

| Élément | Broche Arduino | Remarque |
| --- | --- | --- |
| INA219 n°1 (production) SDA / SCL | A4 / A5 | adresse 0x40, VCC 5 V |
| INA219 n°2 (lampe) SDA / SCL | A4 / A5 | adresse 0x41 : souder le pont A0 |
| Capteur Hall A3144 | D2 | aimant collé sur l'axe ; résistance de tirage 10 kΩ vers 5 V |
| Bouton poussoir | D3 | vers GND, pull-up interne |
| L293D 1A (commande du relais K1) | D4 | 1,2EN, VCC1 et VCC2 à 5 V ; 1Y vers la bobine du relais |
| LCD1602 D7, D6, D5, D4 (broches 14, 13, 12, 11) | D5, D6, D9, D10 | RW à GND, contraste par potentiomètre 10 kΩ sur V0 |
| LED rouge | D7 | avec résistance 220 Ω vers GND |
| Buzzer actif | D8 | |
| LCD1602 E / RS (broches 6 et 4) | D11 / D12 | |
| Thermistance NTC | A0 | 5 V, R1 10 kΩ, A0, NTC, GND ; NTC collée sur la batterie |

Ce brochage est celui du schéma électrique (feuille 2). Il est défini dans `firmware/eolia/config.h`.

## Puissance (fils 0,75 à 1,5 mm², bornier)

Générateur, fusible F1, diode Schottky D1, diode TVS D2 en parallèle, **INA219 n°1** (VIN+ puis VIN−), chargeur CC/CV (sortie réglée à 14,4 V au plus) : sa sortie forme le bus 12 V.

Bus 12 V, contact du relais K1, fusible F2, batterie 12 V. K1 isole la batterie en cas de défaut.

Bus 12 V, **INA219 n°2** (VIN+ puis VIN−), lampe LED 12 V, retour au 0 V commun.

Toutes les masses (Arduino, INA219, négatif batterie) sont reliées. La tension du bus mesurée par l'INA219 n°2 sert de tension batterie (égale à celle de la batterie quand K1 est fermé). Ne jamais alimenter l'Arduino depuis la batterie 12 V : USB ou adaptateur 9 V uniquement.

## Ordre de mise en service

1. Mesurer au multimètre la tension à vide du générateur au régime du ventilateur (au moins 12 V attendus, pas plus de 25 V pour l'INA219).
2. Téléverser le firmware sans rien brancher : il passe tout seul en simulation (`SIM 1`).
3. Brancher le LCD, la LED, le buzzer et le bouton ; vérifier l'affichage.
4. Brancher les INA219 avec le petit moteur DC d'essai, puis `SIM 0`.
5. Brancher le vrai générateur, le chargeur et la batterie.
