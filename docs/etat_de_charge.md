# Estimation de l'état de charge

L'agent Batterie estime l'état de charge (SOC, en %) à chaque cycle de 200 ms. Il le publie aux autres agents (`MSG_ETAT_BATTERIE`) et il apparaît dans le champ `soc` de la trame `D`.

## Pourquoi pas la tension seule

La tension d'une batterie plomb au repos donne une bonne idée de sa charge : 11,8 V vide, 12,7 V pleine. Mais pendant la charge, le chargeur relève la tension jusqu'à 14,4 V et elle ne dit plus rien. L'ancienne estimation gelait donc le SOC dès que l'éolienne produisait : par vent régulier, il restait bloqué sur sa valeur de départ.

## Méthode : comptage d'énergie et recalage

À chaque cycle :

```
SOC += 100 × (RENDEMENT_CHARGE × P_gen − P_lampe) × dt / CAPACITE_BAT_WH
```

- `P_gen` est mesurée par l'INA219 n°1 en entrée du chargeur, `P_lampe` par l'INA219 n°2.
- `RENDEMENT_CHARGE` est la part de l'énergie produite qui est vraiment stockée (pertes du chargeur et de la batterie).

Le comptage seul dérive : erreurs de mesure, rendement approximatif, autodécharge. Il est donc **recalé sur la tension** quand la batterie ne charge pas (`P_gen` < `P_CHARGE_MIN`) :

```
SOC += (SOC_tension − SOC) × dt / TAU_RECALAGE_S
```

- `SOC_tension` vient de la courbe tension / état de charge (`socDepuisTension`, mesures.h), avec la tension corrigée de la chute due à la lampe (`R_INTERNE_BAT × I_lampe`).
- La constante de temps de 10 minutes laisse retomber la tension de surface après une charge avant que le recalage ne pèse.
- Au démarrage, le SOC vaut 50 % ; il est remplacé par `SOC_tension` à la première mesure hors charge.

## Paramètres (config.h)

| Constante | Valeur | À faire |
| --- | --- | --- |
| `CAPACITE_BAT_WH` | 84 Wh (12 V × 7 Ah) | Mettre la capacité de la batterie utilisée (Ah inscrits × 12) |
| `RENDEMENT_CHARGE` | 0,85 | Mesurer (voir ci-dessous), en général entre 0,65 et 0,85 |
| `P_CHARGE_MIN` | 0,5 W | En dessous, la batterie est considérée hors charge |
| `R_INTERNE_BAT` | 0,05 Ω | Ordre de grandeur pour une batterie 7 Ah ; influence faible avec une lampe de 3 W |
| `TAU_RECALAGE_S` | 600 s | Plus grand si le SOC remonte trop vite après une charge |
| `CAPACITE_SIM_WH` | 0,022 Wh | Capacité réduite de la simulation accélérée, ne pas modifier |

## Mesurer le rendement de charge

1. Laisser la batterie au repos au moins une heure, noter `soc` (trame `D`), soit SOC₁.
2. `LAMPE 0`, puis charger avec l'éolienne pendant quelques heures, sans lampe. Relever `e_prod_wh` au début et à la fin (CSV de la supervision) : E_prod.
3. Arrêter le ventilateur, laisser reposer une heure, noter SOC₂.
4. `RENDEMENT_CHARGE = (SOC₂ − SOC₁) / 100 × CAPACITE_BAT_WH / E_prod`.

Plus l'écart entre SOC₁ et SOC₂ est grand, plus le résultat est fiable. Partir d'une batterie à moitié chargée donne un bon écart sans entrer dans la phase finale de charge, où le rendement chute.

## Validation

`tests/test_natif.cpp` compare l'estimation à l'état de charge réel de la simulation dans les quatre scénarios. La dérive cumulée reste inférieure à 0,1 point de SOC sur 300 cycles, et l'écart après recalage inférieur à 1 point.
