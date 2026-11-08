# Charte graphique Eolia

Version en ligne (couleurs et logos affichés) : https://claude.ai/artifact/SMJ1ypaNRhAhv29ci2wEbB

Une mini-éolienne qui négocie elle-même l'usage de son énergie.

Cette charte s'applique à tous les supports du projet Eolia (Smart Realization X5, UCAC-ICAM) : rapports, documentation technique, diapositives, poster, courbes et captures de l'outil de supervision.

## Le logo

Le logo associe un **symbole** et un **mot-symbole**.

- **Le symbole** : un rotor à trois pales profilées en `alize`, un moyeu `nuit-fixe` dont le centre `courant` évoque la LED d'état de l'Arduino. Un arc suit la trajectoire d'une pale et se termine sur un point : la mesure que prennent les capteurs à chaque tour. Le vent, la rotation, la donnée.
- **Le mot-symbole** : « eolia » toujours en minuscules, dessiné en traits de même épaisseur à extrémités arrondies. Le point du i est en `courant`, en écho à la LED du moyeu. Ce n'est pas une police : utiliser toujours les fichiers fournis, ne jamais retaper le nom dans une police.

### Versions

| Fichier | Usage |
| --- | --- |
| `eolia_logo_couleur.svg` | Version principale, sur `fond-page` blanc ou `brume`. |
| `eolia_logo_blanc.svg` | Sur `nuit-fixe` ou sur une photo sombre. Les pales et le texte passent en blanc, les points restent en `courant`. |
| `eolia_logo_noir.svg` | Impression monochrome, tampon, gravure, découpe laser sur la nacelle. |
| `eolia_symbole_*.svg` | Le symbole seul : avatar, en-tête de diapositive, coin de page, autocollant. |
| `eolia_favicon.svg` | Moins de 32 px : le symbole sans l'arc, qui disparaîtrait. |

### Zone de protection et taille minimale

- Laisser autour du logo un espace vide égal à la hauteur du moyeu multipliée par deux (environ un sixième de la hauteur du symbole). Aucun texte, filet ou bord de page dans cette zone.
- Taille minimale du logo horizontal : 30 mm de large à l'impression, 120 px à l'écran. En dessous, utiliser le symbole seul (minimum 8 mm ou 24 px), puis le favicon.

### À ne pas faire

- Ne pas changer les couleurs des pales ni mettre les points en `alerte`.
- Ne pas déformer, incliner, ombrer ni entourer le logo d'un contour.
- Ne pas poser la version couleur sur `nuit-fixe` ou sur une photo chargée : prendre la version blanche.
- Ne pas faire tourner le symbole, sauf animation d'attente à l'écran (rotation lente, sens horaire, arc fixe).
- Ne pas écrire « EOLIA » ni « Éolia » dans le logo. Dans le texte courant, écrire **Eolia** avec une majuscule, sans accent.

## Couleurs

Trois couleurs portent l'identité, les autres servent la lecture.

- `nuit` est l'encre : titres et texte courant. `nuit-fixe` reste bleu nuit dans les deux thèmes, pour les bandeaux et le poster.
- `alize` est la couleur signature des pales : aplats, filets épais, pictogrammes, première série de courbe. Pour du texte en couleur de marque, prendre `alize-texte` (meilleur contraste sur blanc).
- `courant` est l'accent énergie. Une seule touche par page : le chiffre clé, la valeur mesurée mise en avant, la courbe de tension. Jamais de texte `courant` sur fond blanc ; du texte `nuit-fixe` posé sur `courant` est lisible.
- `brume` sert de fond aux encadrés, aux lignes paires des tableaux et aux extraits de trame série.
- `ardoise` pour tout ce qui est secondaire : légendes, sources, en-têtes et pieds de page.
- `alerte` est réservé aux sécurités du firmware (`SURCHAUFFE`, `BAT_BASSE`, `SURTENSION_GEN`) et aux erreurs. Toujours accompagné du mot, jamais la couleur seule.

Proportions sur une page de rapport : environ 70 % de blanc, 20 % de `nuit`, 8 % d'`alize`, 2 % de `courant`.

### Courbes et graphiques

Les figures produites par `outils/courbes.py` et `outils/supervision.py` suivent le même ordre de séries :

1. Puissance du générateur : `alize`, trait plein.
2. Tension (générateur ou batterie) : `courant`, trait plein.
3. Courant ou état de charge : `nuit`, trait pointillé.
4. Seuils de `config.h` : ligne horizontale `ardoise` fine, étiquetée.

Fond `fond-page`, grille `filet`, texte des axes `ardoise`, titre de figure aligné à gauche en `nuit`. Toujours indiquer l'unité entre parenthèses : « Puissance (W) », « Vitesse de rotation (tr/min) », « Température (°C) ».

## Typographie

Trois familles, toutes gratuites sur Google Fonts (à installer sur les postes pour Word et PowerPoint).

- **Lexend** (`titre`) pour les titres : géométrique et arrondie, elle prolonge le dessin du mot-symbole. Graisses 500 et 600 uniquement.
- **IBM Plex Sans** (`texte`) pour le texte courant, les tableaux et les légendes.
- **IBM Plex Mono** (`mesure`) pour les valeurs mesurées alignées, les noms de broches, de fichiers, de variables et les trames série : `A0`, `config.h`, `T_BAT_MAX`, `$EOL,12.4,0.35,...`.

Échelle pour un rapport A4 (en points pour Word, entre parenthèses en pixels écran) :

| Style | Famille | Taille |
| --- | --- | --- |
| `couverture` | Lexend 600 | 30 pt (40 px) |
| `titre-1` | Lexend 600 | 20 pt (26 px), en `nuit` |
| `titre-2` | Lexend 500 | 14 pt (19 px), en `nuit` |
| `titre-3` | Lexend 500 | 11 pt (15 px), en `alize-texte` |
| `corps` | IBM Plex Sans 400 | 10,5 pt, interligne 1,5 |
| `legende` | IBM Plex Sans 400 | 9 pt, en `ardoise` |
| `mesure` | IBM Plex Mono 400 | 9,5 pt |

Si les polices ne sont pas disponibles : Century Gothic pour les titres, Arial pour le texte, Consolas pour les mesures.

## Ton et écriture

- Tout est rédigé en français, au présent, à la première personne du pluriel pour l'équipe (« nous avons calibré les INA219 »).
- Phrases courtes et factuelles. Chaque affirmation chiffrée donne sa valeur, son unité et sa source (mesure, fiche technique, calcul).
- Nombres à la française : virgule décimale et espace avant l'unité (« 12,6 V », « 3,2 W »), sauf dans le code et les trames série où le point reste.
- Pas de tiret long : utiliser la virgule, les deux-points ou les parenthèses.
- Pas d'émoji dans les documents.
- Les noms d'agents prennent une majuscule : l'agent Production, l'agent Batterie, l'agent Charge, le Superviseur.

## Gabarits de documents

### Page de garde (rapport, cahier des charges, compte rendu)

- Bandeau `nuit-fixe` en haut de page, sur un tiers de la hauteur, logo blanc à gauche avec sa zone de protection.
- Sous le bandeau : type de document en `legende` majuscules `alize-texte` (« RAPPORT TECHNIQUE »), titre en `couverture` `nuit`, sous-titre en `titre-2` `ardoise`.
- En bas : noms de l'équipe, encadrant, « Smart Realization X5, UCAC-ICAM », date au format « 8 novembre 2026 », numéro de version.
- Un seul point `courant` sur la page : le point du i du logo.

### Pages intérieures

- Marges : 25 mm en haut et en bas, 22 mm à gauche et à droite.
- En-tête : symbole couleur de 6 mm à gauche, titre court du document à droite en `legende` `ardoise`, filet `filet` de 0,5 pt en dessous.
- Pied de page : « Eolia, Smart Realization X5 » à gauche, « page 3 / 18 » à droite, en `legende` `ardoise`.
- Un `titre-1` ouvre chaque partie, surmonté d'un filet `alize` de 2 pt sur toute la largeur.
- Encadrés (remarque, sécurité) : fond `brume`, coins `rayon-1`, marge interne `pas-2`. Un encadré de sécurité porte le mot « Sécurité » en `alerte` gras en tête.
- Tableaux : en-tête en `texte` gras `nuit` sur `brume`, filets horizontaux `filet` seulement, valeurs numériques alignées à droite en `mesure`.
- Figures : numérotées (« Figure 4 : puissance en fonction de la vitesse de rotation »), légende en `legende` `ardoise` sous la figure.

### Diapositives (16:9)

- Diapositive de titre : fond `nuit-fixe`, logo blanc, titre en Lexend 600 blanc.
- Diapositives de contenu : fond `fond-page`, symbole couleur en haut à droite, titre `titre-1` aligné à gauche, une idée par diapositive.
- Mettre le chiffre clé en très grand, Lexend 600 `nuit`, avec son unité en `alize-texte`.

### Poster (A1 ou A0, portrait)

- Bandeau supérieur `nuit-fixe` avec logo blanc et titre ; marges `pas-4` à l'échelle (40 mm sur A1).
- Trois colonnes : le besoin, le système (schéma des quatre agents autour du bus), les résultats (courbes puissance / vitesse).
- Le résultat principal en `courant` sur `nuit-fixe`, une seule fois.
- Pied : logos UCAC-ICAM et Eolia, noms de l'équipe, QR code vers le dépôt.

## Fichiers

Les fichiers sources (SVG) et les exports PNG sont dans le dépôt, dossier `docs/charte/`. Toujours partir du SVG pour l'impression ; le PNG sert pour Word, PowerPoint et les réseaux.

## Valeurs des couleurs

| Jeton | Thème clair | Thème sombre | Usage |
| --- | --- | --- | --- |
| `nuit` | #0F2E3D | #E6EEF0 | Encre principale : titres, texte courant, mot-symbole du logo. Sur papier, fond-page et brume (14:1). En sombre, devient le texte clair sur fond-page. |
| `nuit-fixe` | #0F2E3D | #1C4A5E | Bleu nuit de marque : bandeaux de couverture, fond du poster et des diapositives de titre. Porte le logo blanc (blanc dessus : 14:1 en clair, 9:1 en sombre). En thème sombre, légèrement éclairci pour se détacher de fond-page. |
| `alize` | #1C8C86 | #5CC3BC | Couleur signature (les pales). Aplats, filets, pictogrammes et courbe de puissance. Jamais pour du texte courant sur blanc (4,1:1) : utiliser alize-texte. |
| `alize-texte` | #167570 | #5CC3BC | Texte en couleur de marque (intertitres de niveau 3, liens, légendes) sur fond-page et brume (5,4:1 au minimum). |
| `courant` | #F2A93B | #F2A93B | Accent énergie : point du i, LED du moyeu, chiffre clé, courbe de tension. Jamais en texte sur blanc. Texte nuit-fixe posé dessus (7,1:1). |
| `fond-page` | #FFFFFF | #0B1F29 | Fond des pages de rapport et des diapositives. |
| `brume` | #EEF3F4 | #13303F | Fond d'encadré, ligne paire de tableau, zone de code ou de trame série. |
| `ardoise` | #4D626C | #9DB0B8 | Texte secondaire : légendes, en-têtes et pieds de page, sources, métadonnées. Sur fond-page et brume (5,7:1 au minimum). |
| `filet` | #C9D5D9 | #2A4756 | Filets de tableau, séparateurs, bordure d'encadré. |
| `alerte` | #B3261E | #F08A80 | Uniquement pour les sécurités (surchauffe, batterie basse) et les erreurs, toujours accompagné du mot. Sur fond-page (6,5:1). |

Pour régénérer les logos : `python3 docs/charte/generer_logo.py docs/charte`, puis `rsvg-convert` pour les PNG.

## Modèles Word et PowerPoint

Dans `modeles/` :

- `eolia_modele_rapport.dotx` : rapport A4 avec la page de garde (bandeau bleu nuit, logo blanc), les en-têtes et pieds de page, et les styles Titre 1 à 3, Légende, Encadré et Trame. Double-cliquer crée un nouveau document à partir du modèle.
- `eolia_modele_presentation.potx` : présentation 16:9 avec sept mises en page (EOLIA_TITRE, EOLIA_INTERTITRE, EOLIA_CONTENU, EOLIA_TITRE_SEUL, EOLIA_DEUX_COLONNES, EOLIA_CHIFFRE_CLE, EOLIA_CONCLUSION) et dix diapositives d'exemple à garder ou supprimer.
- Les versions `.docx` et `.pptx` contiennent la même chose, à ouvrir directement (Google Docs, Google Slides, LibreOffice).

Installer d'abord les polices Lexend, IBM Plex Sans et IBM Plex Mono (Google Fonts), sinon Word et PowerPoint les remplacent.

Pour régénérer les modèles (dans `docs/charte/modeles/`) : `npm install docx@9.5.0 pptxgenjs@3.12.0`, puis `node modele_rapport.js` et `node modele_diapos.js`. Attention : pptxgenjs n'écrit pas les couleurs du thème ; après régénération, les vérifier dans PowerPoint (Création, Variantes, Couleurs) et saisir celles de la charte si besoin. Le plus simple reste de modifier directement le fichier `.potx`.
