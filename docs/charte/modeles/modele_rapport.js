// Modèle de rapport Word aux couleurs de la charte Eolia.
const fs = require("fs");
const path = require("path");
const {
  Document, Packer, Paragraph, TextRun, ImageRun, Header, Footer, AlignmentType,
  HorizontalPositionRelativeFrom, VerticalPositionRelativeFrom, TextWrappingType,
  BorderStyle, Table, TableRow, TableCell, WidthType, ShadingType, PageNumber,
  PageBreak, TableOfContents, LevelFormat, PositionalTab, PositionalTabAlignment,
  PositionalTabRelativeTo, PositionalTabLeader, HeadingLevel, TableLayoutType,
} = require("docx");

const ICI = __dirname;
const SORTIE = process.argv[2] || path.join(ICI, "eolia_modele_rapport.docx");

// Jetons de la charte (thème clair)
const NUIT = "0F2E3D", ALIZE = "1C8C86", ALIZE_TEXTE = "167570", COURANT = "F2A93B";
const BRUME = "EEF3F4", ARDOISE = "4D626C", FILET = "C9D5D9", ALERTE = "B3261E";
const TITRE = "Lexend", TEXTE = "IBM Plex Sans", MESURE = "IBM Plex Mono";

const mm = (v) => Math.round(v * 56.6929); // millimètres vers twips (dxa)
const emu = (v) => Math.round(v * 36000);  // millimètres vers EMU
const PAGE_L = 11906, MARGE_LR = mm(22), LARGEUR = PAGE_L - 2 * MARGE_LR;

const bandeau = fs.readFileSync(path.join(ICI, "bandeau.png"));
const symbole = fs.readFileSync(path.join(ICI, "symbole.png"));

const filetBas = { bottom: { style: BorderStyle.SINGLE, size: 4, color: FILET, space: 6 } };
const filetHaut = { top: { style: BorderStyle.SINGLE, size: 4, color: FILET, space: 6 } };

// ---------- Utilitaires de contenu ----------
const p = (texte, opts = {}) => new Paragraph({ ...opts, children: [new TextRun(texte)] });
const runs = (...parts) => parts.map((x) => (typeof x === "string" ? new TextRun(x) : x));
const code = (t) => new TextRun({ text: t, font: MESURE, size: 19, color: NUIT });
const puce = (...parts) => new Paragraph({ numbering: { reference: "puces", level: 0 }, children: runs(...parts) });

function encadre(titre, couleurTitre, ...lignes) {
  return [
    new Paragraph({ style: "Encadre", children: [new TextRun({ text: titre, bold: true, color: couleurTitre })] }),
    ...lignes.map((l, i) => new Paragraph({ style: "Encadre", spacing: { after: i === lignes.length - 1 ? 240 : 0 }, children: runs(...[].concat(l)) })),
  ];
}

function cellule(texte, largeur, { entete = false, droite = false, mono = false, pair = false } = {}) {
  return new TableCell({
    width: { size: largeur, type: WidthType.DXA },
    shading: entete ? { type: ShadingType.CLEAR, fill: BRUME, color: "auto" } : pair ? { type: ShadingType.CLEAR, fill: "F7FAFA", color: "auto" } : undefined,
    margins: { top: 80, bottom: 80, left: 120, right: 120 },
    borders: {
      top: { style: BorderStyle.NONE, size: 0, color: "FFFFFF" },
      left: { style: BorderStyle.NONE, size: 0, color: "FFFFFF" },
      right: { style: BorderStyle.NONE, size: 0, color: "FFFFFF" },
      bottom: { style: BorderStyle.SINGLE, size: entete ? 8 : 4, color: entete ? NUIT : FILET },
    },
    children: [new Paragraph({
      alignment: droite ? AlignmentType.RIGHT : AlignmentType.LEFT,
      spacing: { after: 0, line: 260 },
      children: [new TextRun({ text: texte, bold: entete, font: mono ? MESURE : TEXTE, size: mono ? 18 : 19, color: NUIT })],
    })],
  });
}

function tableau(colonnes, lignes, alignDroite = [], mono = []) {
  const total = colonnes.reduce((a, c) => a + c.l, 0);
  return new Table({
    width: { size: total, type: WidthType.DXA },
    columnWidths: colonnes.map((c) => c.l),
    layout: TableLayoutType.FIXED,
    rows: [
      new TableRow({ tableHeader: true, children: colonnes.map((c, i) => cellule(c.t, c.l, { entete: true, droite: alignDroite.includes(i) })) }),
      ...lignes.map((l, j) => new TableRow({ children: l.map((v, i) => cellule(v, colonnes[i].l, { droite: alignDroite.includes(i), mono: mono.includes(i), pair: j % 2 === 1 })) })),
    ],
  });
}

// ---------- Page de garde ----------
const pageDeGarde = [
  new Paragraph({
    spacing: { before: 0, after: 0 },
    children: [new ImageRun({
      type: "png", data: bandeau,
      transformation: { width: 794, height: 374 }, // 210 x 99 mm à 96 ppp
      floating: {
        horizontalPosition: { relative: HorizontalPositionRelativeFrom.PAGE, offset: 0 },
        verticalPosition: { relative: VerticalPositionRelativeFrom.PAGE, offset: 0 },
        behindDocument: true, allowOverlap: true,
        wrap: { type: TextWrappingType.NONE },
      },
      altText: { title: "Bandeau Eolia", description: "Bandeau bleu nuit avec le logo Eolia", name: "bandeau" },
    })],
  }),
  new Paragraph({ spacing: { before: mm(92), after: 160 }, children: [new TextRun({ text: "RAPPORT TECHNIQUE", font: TEXTE, bold: true, size: 18, color: ALIZE_TEXTE, characterSpacing: 40 })] }),
  new Paragraph({ style: "Title", children: [new TextRun("[Titre du document]")] }),
  new Paragraph({ style: "Subtitle", children: [new TextRun("[Sous-titre : ce que le document établit, en une phrase]")] }),
  new Paragraph({ spacing: { before: 600, after: 60 }, children: [new TextRun({ text: "Équipe", font: TEXTE, bold: true, size: 18, color: ARDOISE, characterSpacing: 20 })] }),
  p("[Prénom Nom], [Prénom Nom], [Prénom Nom], [Prénom Nom]", { spacing: { after: 200 } }),
  new Paragraph({ spacing: { after: 60 }, children: [new TextRun({ text: "Encadrant", font: TEXTE, bold: true, size: 18, color: ARDOISE, characterSpacing: 20 })] }),
  p("[Prénom Nom]"),
];

const piedGarde = new Footer({
  children: [
    new Paragraph({
      border: filetHaut,
      spacing: { before: 0, after: 0 },
      children: [
        new TextRun({ text: "Smart Realization X5, UCAC-ICAM", font: TEXTE, size: 18, color: NUIT, bold: true }),
        new TextRun({ children: [new PositionalTab({ alignment: PositionalTabAlignment.RIGHT, relativeTo: PositionalTabRelativeTo.MARGIN, leader: PositionalTabLeader.NONE }), "[8 novembre 2026], version [1.0]"], font: TEXTE, size: 18, color: ARDOISE }),
      ],
    }),
  ],
});

// ---------- En-tête et pied des pages intérieures ----------
const entete = new Header({
  children: [new Paragraph({
    border: filetBas,
    children: [
      new ImageRun({ type: "png", data: symbole, transformation: { width: 23, height: 23 }, altText: { title: "Symbole Eolia", description: "Symbole Eolia", name: "symbole" } }),
      new TextRun({ children: [new PositionalTab({ alignment: PositionalTabAlignment.RIGHT, relativeTo: PositionalTabRelativeTo.MARGIN, leader: PositionalTabLeader.NONE }), "[Titre court du document]"], font: TEXTE, size: 18, color: ARDOISE }),
    ],
  })],
});

const pied = new Footer({
  children: [new Paragraph({
    children: [
      new TextRun({ text: "Eolia, Smart Realization X5", font: TEXTE, size: 18, color: ARDOISE }),
      new TextRun({ children: [new PositionalTab({ alignment: PositionalTabAlignment.RIGHT, relativeTo: PositionalTabRelativeTo.MARGIN, leader: PositionalTabLeader.NONE }), "page ", PageNumber.CURRENT, " / ", PageNumber.TOTAL_PAGES], font: TEXTE, size: 18, color: ARDOISE }),
    ],
  })],
});

// ---------- Pages intérieures : exemples de chaque style ----------
const corps = [
  new Paragraph({ style: "TitreSommaire", children: [new TextRun("Sommaire")] }),
  new TableOfContents("Sommaire", { hyperlink: true, headingStyleRange: "1-3" }),
  new Paragraph({ children: [new PageBreak()] }),

  new Paragraph({ heading: HeadingLevel.HEADING_1, children: [new TextRun("1  Utiliser ce modèle")] }),
  p("Ce modèle applique la charte graphique Eolia. Remplacer les textes entre crochets, puis supprimer les exemples des pages suivantes. Pour que la mise en forme reste cohérente, n'appliquer que les styles du modèle (onglet Accueil, galerie des styles) plutôt qu'une mise en forme manuelle."),
  new Paragraph({ heading: HeadingLevel.HEADING_2, children: [new TextRun("1.1  Styles disponibles")] }),
  puce(new TextRun({ text: "Titre 1, Titre 2, Titre 3 : ", bold: true }), "parties, sous-parties et intertitres. Ils alimentent le sommaire."),
  puce(new TextRun({ text: "Normal : ", bold: true }), "texte courant en IBM Plex Sans 10,5 pt, interligne 1,5."),
  puce(new TextRun({ text: "Légende : ", bold: true }), "sous les figures et au-dessus des tableaux."),
  puce(new TextRun({ text: "Encadré : ", bold: true }), "remarque ou point de sécurité, sur fond brume."),
  puce(new TextRun({ text: "Trame : ", bold: true }), "extraits de code, de trame série ou de commande, en IBM Plex Mono."),
  new Paragraph({ heading: HeadingLevel.HEADING_2, children: [new TextRun("1.2  Mettre à jour le sommaire et les numéros de page")] }),
  p("Le sommaire se met à jour par un clic droit sur le sommaire, puis Mettre à jour les champs. Le nombre total de pages du pied de page se met à jour de la même manière, ou à l'impression."),
  new Paragraph({ heading: HeadingLevel.HEADING_3, children: [new TextRun("Règles d'écriture")] }),
  p("Nombres à la française dans le texte : virgule décimale et espace avant l'unité (12,6 V ; 3,2 W ; 45 °C). Dans le code et les trames série, le point décimal reste. Pas de tiret long : utiliser la virgule, les deux-points ou les parenthèses."),

  new Paragraph({ heading: HeadingLevel.HEADING_1, children: [new TextRun("2  Exemples de mise en forme")] }),
  p("Les éléments ci-dessous montrent l'aspect attendu des tableaux, encadrés, trames et figures. Les valeurs du tableau 1 sont celles de config.h au 8 novembre 2026."),
  new Paragraph({ style: "Caption", children: [new TextRun("Tableau 1 : seuils de sécurité du firmware (config.h)")] }),
  tableau(
    [{ t: "Constante", l: 2600 }, { t: "Rôle", l: 4612 }, { t: "Valeur", l: 1100 }, { t: "Unité", l: 1100 }],
    [
      ["V_GEN_MAX", "Surtension générateur (limite de l'INA219 : 26 V)", "25,0", "V"],
      ["V_BAT_MAX", "Surcharge de la batterie plomb", "14,6", "V"],
      ["V_BAT_COUPURE", "Coupure de la lampe (décharge profonde)", "11,8", "V"],
      ["V_BAT_REARME", "Réarmement après coupure (hystérésis)", "12,2", "V"],
      ["T_BAT_MAX", "Surchauffe de la batterie", "45,0", "°C"],
    ],
    [2], [0],
  ),
  new Paragraph({ spacing: { after: 120 }, children: [] }),
  ...encadre("Remarque", ALIZE_TEXTE, "Les coefficients de calibration des INA219 et de la NTC sont mesurés au multimètre ; la procédure est décrite dans docs/calibration.md."),
  ...encadre("Sécurité", ALERTE, "En mode manuel, les sécurités surchauffe et batterie basse restent prioritaires : la commande FORCER 1 n'allume pas la lampe si l'une d'elles est active."),
  p("Exemple de trame série (commande puis réponse) :"),
  new Paragraph({ style: "Trame", children: [new TextRun("> MODE MANU")] }),
  new Paragraph({ style: "Trame", children: [new TextRun("< P,ALLUMER,0")] }),
  new Paragraph({ style: "Trame", spacing: { after: 240 }, children: [new TextRun("< E,48210,RELAIS,1")] }),

  new Paragraph({
    alignment: AlignmentType.CENTER,
    shading: { type: ShadingType.CLEAR, fill: BRUME, color: "auto" },
    spacing: { before: 120, after: 0, line: 240 },
    border: { top: { style: BorderStyle.SINGLE, size: 4, color: FILET, space: 40 }, bottom: { style: BorderStyle.SINGLE, size: 4, color: FILET, space: 40 } },
    children: [new TextRun({ text: "[Insérer la figure : clic droit sur ce cadre, puis Insérer une image]", color: ARDOISE, size: 19 })],
  }),
  new Paragraph({ style: "Caption", spacing: { before: 120, after: 240 }, children: [new TextRun("Figure 1 : puissance du générateur en fonction de la vitesse de rotation (source : mesures du [date], outils/courbes.py)")] }),

  new Paragraph({ heading: HeadingLevel.HEADING_1, children: [new TextRun("3  [Titre de partie]")] }),
  p("[Texte de la partie.]"),
];

// ---------- Document ----------
const doc = new Document({
  creator: "Équipe Eolia",
  title: "Modèle de rapport Eolia",
  description: "Modèle Word conforme à la charte graphique Eolia",
  features: { updateFields: true },
  styles: {
    default: {
      document: { run: { font: TEXTE, size: 21, color: NUIT }, paragraph: { spacing: { after: 160, line: 360 } } },
      title: { run: { font: TITRE, size: 60, bold: true, color: NUIT }, paragraph: { spacing: { before: 0, after: 200, line: 264 }, keepNext: true } },
      heading1: { run: { font: TITRE, size: 40, bold: true, color: NUIT }, paragraph: { spacing: { before: 480, after: 200, line: 300 }, keepNext: true, keepLines: true, outlineLevel: 0, border: { top: { style: BorderStyle.SINGLE, size: 16, color: ALIZE, space: 10 } } } },
      heading2: { run: { font: TITRE, size: 28, color: NUIT }, paragraph: { spacing: { before: 320, after: 120, line: 300 }, keepNext: true, keepLines: true, outlineLevel: 1 } },
      heading3: { run: { font: TITRE, size: 22, color: ALIZE_TEXTE }, paragraph: { spacing: { before: 240, after: 80, line: 300 }, keepNext: true, keepLines: true, outlineLevel: 2 } },
      hyperlink: { run: { color: ALIZE_TEXTE, underline: {} } },
    },
    paragraphStyles: [
      { id: "Subtitle", name: "Subtitle", basedOn: "Normal", next: "Normal", quickFormat: true, run: { font: TITRE, size: 28, color: ARDOISE }, paragraph: { spacing: { before: 0, after: 200, line: 300 } } },
      { id: "Caption", name: "caption", basedOn: "Normal", next: "Normal", quickFormat: true, run: { font: TEXTE, size: 18, color: ARDOISE }, paragraph: { spacing: { before: 0, after: 80, line: 260 }, keepNext: true } },
      { id: "Encadre", name: "Encadré", basedOn: "Normal", next: "Normal", quickFormat: true, run: { size: 20 },
        paragraph: { shading: { type: ShadingType.CLEAR, fill: BRUME, color: "auto" }, indent: { left: 170, right: 170 }, spacing: { before: 0, after: 0, line: 300 },
          border: { top: { style: BorderStyle.SINGLE, size: 2, color: BRUME, space: 6 }, bottom: { style: BorderStyle.SINGLE, size: 2, color: BRUME, space: 6 } } } },
      { id: "Trame", name: "Trame", basedOn: "Normal", next: "Normal", quickFormat: true, run: { font: MESURE, size: 18, color: NUIT },
        paragraph: { shading: { type: ShadingType.CLEAR, fill: BRUME, color: "auto" }, indent: { left: 170, right: 170 }, spacing: { before: 0, after: 0, line: 260 } } },
      { id: "TitreSommaire", name: "Titre du sommaire", basedOn: "Normal", next: "Normal", run: { font: TITRE, size: 40, bold: true, color: NUIT }, paragraph: { spacing: { before: 0, after: 240 } } },
      { id: "TOC1", name: "toc 1", basedOn: "Normal", next: "Normal", run: { bold: true }, paragraph: { spacing: { before: 120, after: 40 } } },
      { id: "TOC2", name: "toc 2", basedOn: "Normal", next: "Normal", paragraph: { indent: { left: 280 }, spacing: { after: 20 } } },
      { id: "TOC3", name: "toc 3", basedOn: "Normal", next: "Normal", run: { color: ARDOISE }, paragraph: { indent: { left: 560 }, spacing: { after: 20 } } },
    ],
  },
  numbering: {
    config: [{
      reference: "puces",
      levels: [
        { level: 0, format: LevelFormat.BULLET, text: "▪", alignment: AlignmentType.LEFT, style: { run: { color: ALIZE }, paragraph: { indent: { left: 360, hanging: 240 } } } },
        { level: 1, format: LevelFormat.BULLET, text: "–", alignment: AlignmentType.LEFT, style: { run: { color: ARDOISE }, paragraph: { indent: { left: 720, hanging: 240 } } } },
      ],
    }],
  },
  sections: [
    {
      properties: { page: { size: { width: PAGE_L, height: 16838 }, margin: { top: mm(25), bottom: mm(25), left: MARGE_LR, right: MARGE_LR, header: mm(12), footer: mm(14) } } },
      footers: { default: piedGarde },
      headers: { default: new Header({ children: [new Paragraph({ children: [] })] }) },
      children: pageDeGarde,
    },
    {
      properties: { page: { size: { width: PAGE_L, height: 16838 }, margin: { top: mm(25), bottom: mm(25), left: MARGE_LR, right: MARGE_LR, header: mm(12), footer: mm(12) } } },
      headers: { default: entete },
      footers: { default: pied },
      children: corps,
    },
  ],
});

Packer.toBuffer(doc).then((buf) => { fs.writeFileSync(SORTIE, buf); console.log("écrit :", SORTIE); });
