// Modèle PowerPoint aux couleurs de la charte Eolia (16:9).
// Usage : node modele_diapos.js [sortie.pptx] [chemin/vers/apply_theme.js]
const path = require("path");
const pptxgen = require("pptxgenjs");

const ICI = __dirname;
const SORTIE = process.argv[2] || path.join(ICI, "eolia_modele_presentation.pptx");
const APPLY_THEME = process.argv[3];
const img = (f) => path.join(ICI, f);

// Thème : couleurs et polices de la charte
const THEME = {
  name: "Eolia",
  headFontFace: "Lexend",
  bodyFontFace: "IBM Plex Sans",
  colors: {
    dk1: "0F2E3D", lt1: "FFFFFF", dk2: "4D626C", lt2: "EEF3F4",
    accent1: "1C8C86", accent2: "F2A93B", accent3: "167570", accent4: "C9D5D9",
    accent5: "B3261E", accent6: "5CC3BC", hlink: "167570", folHlink: "4D626C",
  },
};
const HEX = { nuit: "0F2E3D", alize: "1C8C86", alizeTexte: "167570", courant: "F2A93B", brume: "EEF3F4", ardoise: "4D626C", filet: "C9D5D9", gris: "9DB0B8" };

const pres = new pptxgen();
pres.layout = "LAYOUT_WIDE"; // 13,333 x 7,5 pouces
pres.title = "Modèle de présentation Eolia";
pres.author = "Équipe Eolia";
pres.company = "Smart Realization X5, UCAC-ICAM";
pres.theme = { headFontFace: THEME.headFontFace, bodyFontFace: THEME.bodyFontFace };
const C = pres.SchemeColor;

const W = 13.333, H = 7.5, M = 0.6;
const pied = () => [
  { text: { text: "Eolia, Smart Realization X5", options: { x: M, y: 6.95, w: 6, h: 0.3, fontSize: 10, color: C.text2, margin: 0 } } },
];
const numero = { x: W - M - 1, y: 6.95, w: 1, h: 0.3, fontSize: 10, color: C.text2, align: "right" };

// ---------- Mises en page (layouts) ----------
pres.defineSlideMaster({
  title: "EOLIA_TITRE",
  background: { color: C.text1 },
  objects: [
    { image: { path: img("arcs.png"), x: W - 4.6, y: H - 4.6, w: 4.6, h: 4.6 } },
    { image: { path: img("eolia_logo_blanc.png"), x: M + 0.1, y: 0.7, w: 3.2, h: 1.097 } },
    { placeholder: { options: { name: "title", type: "title", align: "left", x: M + 0.1, y: 2.75, w: 8.4, h: 1.7, fontSize: 40, bold: true, color: C.background1, valign: "bottom", margin: 0 }, text: "Titre de la présentation" } },
    { placeholder: { options: { name: "body", type: "body", align: "left", x: M + 0.1, y: 4.6, w: 8.4, h: 0.9, fontSize: 18, color: C.background2, valign: "top", margin: 0 }, text: "Sous-titre" } },
    { placeholder: { options: { name: "infos", type: "body", align: "left", x: M + 0.1, y: 6.45, w: 7.5, h: 0.6, fontSize: 12, color: C.background2, valign: "bottom", margin: 0 }, text: "Équipe, date" } },
  ],
});

pres.defineSlideMaster({
  title: "EOLIA_INTERTITRE",
  background: { color: C.text1 },
  objects: [
    { image: { path: img("arcs.png"), x: W - 3.4, y: H - 3.4, w: 3.4, h: 3.4 } },
    { placeholder: { options: { name: "numero", type: "body", align: "left", x: M + 0.1, y: 2.0, w: 3, h: 1.3, fontFace: "Lexend", fontSize: 66, bold: true, color: C.accent2, valign: "bottom", margin: 0 }, text: "01" } },
    { placeholder: { options: { name: "title", type: "title", align: "left", x: M + 0.1, y: 3.45, w: 9, h: 1.3, fontSize: 36, bold: true, color: C.background1, valign: "top", margin: 0 }, text: "Titre de partie" } },
  ],
  slideNumber: { ...numero, color: C.background2 },
});

const contenuBase = (extra) => ({
  background: { color: C.background1 },
  objects: [
    { image: { path: img("eolia_symbole_couleur.png"), x: W - M - 0.55, y: 0.5, w: 0.55, h: 0.55 } },
    { placeholder: { options: { name: "title", type: "title", align: "left", x: M, y: 0.45, w: W - 2 * M - 1, h: 0.75, fontSize: 30, bold: true, color: C.text1, valign: "middle", margin: 0 }, text: "Titre de la diapositive" } },
    ...extra,
    ...pied(),
  ],
  slideNumber: numero,
});

pres.defineSlideMaster({
  title: "EOLIA_CONTENU",
  ...contenuBase([
    { placeholder: { options: { name: "body", type: "body", align: "left", x: M, y: 1.55, w: W - 2 * M, h: 5.1, fontSize: 18, color: C.text1, valign: "top", margin: 0, paraSpaceAfter: 8 }, text: "Texte" } },
  ]),
});

pres.defineSlideMaster({ title: "EOLIA_TITRE_SEUL", ...contenuBase([]) });

const colL = (W - 2 * M - 0.5) / 2;
pres.defineSlideMaster({
  title: "EOLIA_DEUX_COLONNES",
  ...contenuBase([
    { placeholder: { options: { name: "gauche", type: "body", align: "left", x: M, y: 1.55, w: colL, h: 5.1, fontSize: 16, color: C.text1, valign: "top", margin: 0, paraSpaceAfter: 6 }, text: "Colonne de gauche" } },
    { placeholder: { options: { name: "droite", type: "body", align: "left", x: M + colL + 0.5, y: 1.55, w: colL, h: 5.1, fontSize: 16, color: C.text1, valign: "top", margin: 0, paraSpaceAfter: 6 }, text: "Colonne de droite" } },
  ]),
});

pres.defineSlideMaster({
  title: "EOLIA_CHIFFRE_CLE",
  ...contenuBase([
    { placeholder: { options: { name: "chiffre", type: "body", align: "left", x: M, y: 1.9, w: 6.6, h: 2.6, fontFace: "Lexend", fontSize: 120, bold: true, color: C.text1, valign: "bottom", margin: 0 }, text: "84" } },
    { placeholder: { options: { name: "commentaire", type: "body", align: "left", x: M, y: 4.7, w: 6.6, h: 1.6, fontSize: 18, color: C.text2, valign: "top", margin: 0 }, text: "Ce que le chiffre signifie" } },
    { image: { path: img("arcs.png"), x: W - 5.2, y: 1.6, w: 4.4, h: 4.4 } },
  ]),
});

pres.defineSlideMaster({
  title: "EOLIA_CONCLUSION",
  background: { color: C.text1 },
  objects: [
    { image: { path: img("arcs.png"), x: W - 4.6, y: H - 4.6, w: 4.6, h: 4.6 } },
    { image: { path: img("eolia_symbole_blanc.png"), x: M + 0.1, y: 0.7, w: 1.0, h: 1.0 } },
    { placeholder: { options: { name: "title", type: "title", align: "left", x: M + 0.1, y: 2.3, w: 8.4, h: 1.3, fontSize: 40, bold: true, color: C.background1, valign: "bottom", margin: 0 }, text: "Merci" } },
    { placeholder: { options: { name: "body", type: "body", align: "left", x: M + 0.1, y: 3.8, w: 8.0, h: 2.4, fontSize: 18, color: C.background2, valign: "top", margin: 0, paraSpaceAfter: 6 }, text: "Contacts, dépôt, questions" } },
  ],
});

// ---------- Diapositives d'exemple ----------
pres.addSection({ title: "Ouverture" });
let s = pres.addSlide({ masterName: "EOLIA_TITRE", sectionTitle: "Ouverture" });
s.addText("[Titre de la présentation]", { placeholder: "title" });
s.addText("[Sous-titre : ce que la présentation démontre, en une phrase]", { placeholder: "body" });
s.addText("[Prénom Nom], [Prénom Nom], [Prénom Nom]\nSmart Realization X5, UCAC-ICAM, [8 novembre 2026]", { placeholder: "infos" });
s.addNotes("Diapositive de titre. Remplacer les textes entre crochets.");

s = pres.addSlide({ masterName: "EOLIA_CONTENU", sectionTitle: "Ouverture" });
s.addText("Utiliser ce modèle", { placeholder: "title" });
s.addText([
  { text: "Nouvelle diapositive : onglet Accueil, Nouvelle diapositive, puis choisir une mise en page EOLIA_.", options: { bullet: true, breakLine: true } },
  { text: "Une idée par diapositive, un titre qui l'énonce sans point final.", options: { bullet: true, breakLine: true } },
  { text: "Une seule touche d'ambre par diapositive : le chiffre ou la valeur à retenir.", options: { bullet: true, breakLine: true } },
  { text: "Nombres à la française dans le texte : 12,6 V, 3,2 W, 45 °C.", options: { bullet: true, breakLine: true } },
  { text: "Installer Lexend et IBM Plex Sans (Google Fonts) sur le poste qui présente.", options: { bullet: true } },
], { placeholder: "body" });
s.addNotes("Supprimer cette diapositive dans la présentation finale.");

pres.addSection({ title: "Le système" });
s = pres.addSlide({ masterName: "EOLIA_INTERTITRE", sectionTitle: "Le système" });
s.addText("01", { placeholder: "numero" });
s.addText("Le système", { placeholder: "title" });

// Schéma des quatre agents autour du bus (formes, modifiables)
s = pres.addSlide({ masterName: "EOLIA_TITRE_SEUL", sectionTitle: "Le système" });
s.addText("Quatre agents négocient par un bus de messages", { placeholder: "title" });
const agents = [
  ["Production", "Mesure le générateur, publie la puissance, détecte la surtension"],
  ["Batterie", "Estime l'état de charge, accorde ou refuse la décharge"],
  ["Charge", "Porte l'exigence de confort et négocie l'allumage"],
  ["Superviseur", "Mode autonome ou manuel, alarmes, écran, relais"],
];
const cw = 2.85, gap = 0.2, x0 = M, yC = 1.75, hC = 2.1;
agents.forEach(([nom, role], i) => {
  const x = x0 + i * (cw + gap);
  s.addShape(pres.shapes.ROUNDED_RECTANGLE, { x, y: yC, w: cw, h: hC, rectRadius: 0.08, fill: { color: C.background2 }, line: { color: C.background2 }, objectName: `Agent ${nom}` });
  s.addText(nom, { x: x + 0.25, y: yC + 0.25, w: cw - 0.5, h: 0.45, fontFace: "Lexend", fontSize: 18, bold: true, color: C.text1, margin: 0, isTextBox: true, objectName: `Nom ${nom}` });
  s.addText(role, { x: x + 0.25, y: yC + 0.8, w: cw - 0.5, h: 1.15, fontSize: 13, color: C.text2, margin: 0, valign: "top", isTextBox: true, objectName: `Rôle ${nom}` });
  s.addShape(pres.shapes.LINE, { x: x + cw / 2, y: yC + hC, w: 0, h: 0.75, line: { color: C.accent1, width: 2 }, objectName: `Liaison ${nom}` });
  s.addShape(pres.shapes.OVAL, { x: x + cw / 2 - 0.09, y: yC + hC + 0.66, w: 0.18, h: 0.18, fill: { color: C.accent1 }, line: { color: C.accent1 }, objectName: `Noeud ${nom}` });
});
const busW = 4 * cw + 3 * gap;
s.addShape(pres.shapes.ROUNDED_RECTANGLE, { x: x0, y: yC + hC + 0.75, w: busW, h: 0.62, rectRadius: 0.31, fill: { color: C.accent1 }, line: { color: C.accent1 }, objectName: "Bus" });
s.addText("Bus de messages (bus.h)", { x: x0, y: yC + hC + 0.75, w: busW, h: 0.62, fontSize: 15, bold: true, color: C.background1, align: "center", valign: "middle", margin: 0, isTextBox: true, objectName: "Texte bus" });
s.addText("Les agents ne s'appellent jamais directement : chaque décision passe par un message.", { x: x0, y: 5.6, w: busW, h: 0.5, fontSize: 14, color: C.text2, margin: 0, isTextBox: true, objectName: "Commentaire" });

s = pres.addSlide({ masterName: "EOLIA_DEUX_COLONNES", sectionTitle: "Le système" });
s.addText("Deux modes de pilotage", { placeholder: "title" });
s.addText([
  { text: "Mode autonome", options: { bold: true, fontFace: "Lexend", fontSize: 20, color: C.accent3, breakLine: true } },
  { text: "Les agents appliquent eux-mêmes leurs décisions.", options: { bullet: true, breakLine: true } },
  { text: "La lampe s'allume si la batterie est au-dessus de la réserve.", options: { bullet: true, breakLine: true } },
  { text: "Commande série : MODE AUTO", options: { bullet: true } },
], { placeholder: "gauche" });
s.addText([
  { text: "Mode manuel", options: { bold: true, fontFace: "Lexend", fontSize: 20, color: C.accent3, breakLine: true } },
  { text: "Les agents proposent, l'utilisateur accepte ou refuse.", options: { bullet: true, breakLine: true } },
  { text: "Surchauffe et batterie basse restent prioritaires.", options: { bullet: true, breakLine: true } },
  { text: "Commande série : MODE MANU", options: { bullet: true } },
], { placeholder: "droite" });

pres.addSection({ title: "Résultats" });
s = pres.addSlide({ masterName: "EOLIA_INTERTITRE", sectionTitle: "Résultats" });
s.addText("02", { placeholder: "numero" });
s.addText("Résultats", { placeholder: "title" });

s = pres.addSlide({ masterName: "EOLIA_CHIFFRE_CLE", sectionTitle: "Résultats" });
s.addText("Capacité de stockage de la batterie", { placeholder: "title" });
s.addText([{ text: "84" }, { text: " Wh", options: { fontSize: 48, color: C.accent3 } }], { placeholder: "chiffre" });
s.addText("Batterie plomb 12 V, 7 Ah (CAPACITE_BAT_WH dans config.h). Remplacer par le résultat à retenir.", { placeholder: "commentaire" });

s = pres.addSlide({ masterName: "EOLIA_TITRE_SEUL", sectionTitle: "Résultats" });
s.addText("[La puissance croît avec la vitesse de rotation]", { placeholder: "title" });
s.addChart(pres.charts.LINE, [
  { name: "Puissance (W)", labels: ["0", "200", "400", "600", "800", "1000", "1200"], values: [0, 0.2, 0.7, 1.5, 2.6, 3.9, 5.4] },
], {
  x: M, y: 1.5, w: 8.4, h: 5.0, objectName: "Courbe puissance",
  chartColors: [THEME.colors.accent1], lineSize: 3, lineDataSymbol: "circle", lineDataSymbolSize: 7,
  showTitle: false, showLegend: false,
  catAxisTitle: "Vitesse de rotation (tr/min)", showCatAxisTitle: true, valAxisTitle: "Puissance (W)", showValAxisTitle: true,
  catAxisTitleColor: THEME.colors.dk2, valAxisTitleColor: THEME.colors.dk2, catAxisTitleFontSize: 12, valAxisTitleFontSize: 12,
  catAxisLabelColor: THEME.colors.dk2, valAxisLabelColor: THEME.colors.dk2, catAxisLabelFontSize: 11, valAxisLabelFontSize: 11,
  catAxisLabelFontFace: "+mn-lt", valAxisLabelFontFace: "+mn-lt", catAxisTitleFontFace: "+mn-lt", valAxisTitleFontFace: "+mn-lt",
  valGridLine: { color: THEME.colors.accent4, size: 0.75 }, catGridLine: { style: "none" },
  catAxisLineColor: THEME.colors.accent4, valAxisLineShow: false,
});
s.addShape(pres.shapes.ROUNDED_RECTANGLE, { x: 9.4, y: 1.5, w: W - M - 9.4, h: 5.0, rectRadius: 0.08, fill: { color: C.background2 }, line: { color: C.background2 }, objectName: "Encadré commentaire" });
s.addText([
  { text: "Données d'exemple", options: { bold: true, color: C.accent3, breakLine: true } },
  { text: "Clic droit sur la courbe, Modifier les données, puis coller les mesures exportées par outils/courbes.py.", options: { breakLine: true } },
  { text: " ", options: { breakLine: true } },
  { text: "Toujours donner l'unité entre parenthèses sur chaque axe.", options: {} },
], { x: 9.65, y: 1.75, w: W - M - 9.9, h: 4.5, fontSize: 14, color: C.text1, valign: "top", margin: 0, isTextBox: true, objectName: "Texte commentaire" });

s = pres.addSlide({ masterName: "EOLIA_TITRE_SEUL", sectionTitle: "Résultats" });
s.addText("Seuils de sécurité du firmware", { placeholder: "title" });
const th = { bold: true, color: C.text1, fill: { color: C.background2 }, fontSize: 14 };
const td = { color: C.text1, fontSize: 14 };
const rows = [
  [{ text: "Constante", options: th }, { text: "Rôle", options: th }, { text: "Valeur", options: { ...th, align: "right" } }],
  ...[
    ["V_GEN_MAX", "Surtension du générateur", "25,0 V"],
    ["V_BAT_MAX", "Surcharge de la batterie", "14,6 V"],
    ["V_BAT_COUPURE", "Coupure de la lampe", "11,8 V"],
    ["V_BAT_REARME", "Réarmement après coupure", "12,2 V"],
    ["T_BAT_MAX", "Surchauffe de la batterie", "45,0 °C"],
  ].map(([a, b, c]) => [{ text: a, options: { ...td, fontFace: "IBM Plex Mono", fontSize: 13 } }, { text: b, options: td }, { text: c, options: { ...td, align: "right" } }]),
];
s.addTable(rows, {
  x: M, y: 1.55, w: 9.0, colW: [2.8, 4.5, 1.7], rowH: 0.5, valign: "middle",
  border: { type: "solid", color: THEME.colors.accent4, pt: 0.75 }, margin: [0, 0.12, 0, 0.12], objectName: "Tableau seuils",
});
s.addText("Source : firmware/eolia/config.h", { x: M, y: 4.85, w: 9, h: 0.35, fontSize: 11, color: C.text2, margin: 0, isTextBox: true, objectName: "Source" });

pres.addSection({ title: "Clôture" });
s = pres.addSlide({ masterName: "EOLIA_CONCLUSION", sectionTitle: "Clôture" });
s.addText("Merci", { placeholder: "title" });
s.addText([
  { text: "Questions ?", options: { breakLine: true } },
  { text: "[Prénom Nom], [adresse électronique]", options: { breakLine: true } },
  { text: "Dépôt du projet : [lien]", options: {} },
], { placeholder: "body" });

pres.writeFile({ fileName: SORTIE }).then(async () => {
  if (APPLY_THEME) await require(APPLY_THEME).applyTheme(SORTIE, THEME);
  console.log("écrit :", SORTIE);
});
