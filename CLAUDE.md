# Contexte pour Claude Code : projet Eolia

Projet étudiant (Smart Realization X5, UCAC-ICAM) : mini-éolienne intelligente supervisée par un Arduino UNO. Lire README.md, docs/cablage.md et docs/protocole_serie.md avant toute modification.

## Règles du projet

- Tout le texte (commentaires, documentation, messages) est en français. Ne jamais utiliser le tiret long.
- Cible : Arduino UNO, 2 Ko de RAM. Pas d'allocation dynamique, pas de `String`, chaînes constantes dans `F()`.
- Architecture multi-agents : les agents ne s'appellent pas entre eux, ils communiquent uniquement par le `Bus` (bus.h). Toute nouvelle règle de décision va dans l'agent concerné (agents.h).
- Les seuils et le brochage sont uniquement dans config.h.
- Les sécurités (surchauffe, batterie basse) doivent rester prioritaires en mode manuel.
- Le protocole série est un contrat avec outils/supervision.py : tout changement de trame se reporte dans les deux fichiers et dans docs/protocole_serie.md.

## Vérifier après chaque modification

```
pio run                                                        # compile pour l'UNO, surveiller le % de RAM
g++ -std=c++17 -Itests/stubs -o /tmp/t tests/test_natif.cpp && /tmp/t   # doit afficher TOUS LES TESTS PASSENT
```

## Prochaines étapes prévues

1. Calibrer les INA219 et la NTC avec le multimètre (coefficients dans config.h).
2. Affiner l'estimation de l'état de charge (comptage d'énergie pendant la charge).
3. Tracer les courbes puissance / vitesse de rotation à partir des CSV pour le poster.
