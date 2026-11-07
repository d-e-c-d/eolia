# Protocole série Eolia (115200 bauds, lignes terminées par retour à la ligne)

## De l'Arduino vers le PC

| Préfixe | Contenu |
| --- | --- |
| `D,` | trame de mesures, une par seconde (20 champs, voir ci-dessous) |
| `P,ACTION,MOTIF` | mode manuel : proposition d'action (`ALLUMER` ou `ETEINDRE`) |
| `E,t_ms,NOM,valeur` | événement : `RELAIS`, `ALARME`, `PROPOSITION_ACCEPTEE`, `PROPOSITION_REFUSEE` |
| `S,cle=valeur,...` | statistiques (réponse à `STAT`) |
| `C,v_gen,i_gen,v_bat,i_lampe,adc_ntc,ina_prod,ina_lampe` | valeurs brutes sans calibration, moyennées sur 16 lectures (réponse à `CAL`) : V, A, V, A, ADC 0 à 1023, puis 1 si l'INA219 n°1 ou n°2 est détecté |
| `I,texte` | information |

Champs de la trame `D` : `t_ms, v_gen, i_gen, p_gen, rpm, v_bat, soc, t_bat, i_lampe, p_lampe, e_prod_wh, e_lampe_wh, relais, mode, alarmes, msg_cycle, tours, cycle_us, ram_libre, scenario`

Code d'alarme (somme de bits) : 1 surtension générateur, 2 surcharge batterie, 4 batterie basse, 8 surchauffe, 16 capteur de température.

Motif d'une proposition : 0 conditions réunies, 1 batterie sous la réserve, 2 batterie basse, 3 surchauffe, 4 lampe non souhaitée.

## Du PC vers l'Arduino

| Commande | Effet |
| --- | --- |
| `MODE AUTO` / `MODE MANU` | autonome : les agents appliquent leurs décisions ; manuel : ils proposent, l'utilisateur décide |
| `LAMPE 1` / `LAMPE 0` | exigence de confort : l'utilisateur veut ou ne veut pas de lumière |
| `RESERVE n` | pourcentage de charge à garder en réserve (0 à 100) |
| `OK` / `NON` | accepter ou refuser la proposition en attente (le bouton physique vaut `OK`) |
| `FORCER 1` / `FORCER 0` | mode manuel seulement : commande directe de la lampe (les sécurités restent actives) |
| `SIM n` | 0 capteurs réels ; 1 vent fort puis calme ; 2 vent faible ; 3 rafale (surtension) ; 4 surchauffe |
| `RAZ` | remise à zéro des agents et des compteurs |
| `STAT` | statistiques : cycles, messages, durée maximale d'un cycle, empreinte des décisions, RAM libre |
| `CAL` | valeurs brutes des capteurs pour la calibration (trame `C`, voir [calibration.md](calibration.md)) |
