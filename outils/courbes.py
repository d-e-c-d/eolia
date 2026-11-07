#!/usr/bin/env python3
"""Eolia : courbes puissance et tension en fonction de la vitesse de rotation, pour le poster.

Usages :
  python outils/courbes.py mesures_*.csv                  # figures dans figures/
  python outils/courbes.py mesures_*.csv --sortie poster  # autre dossier
  python outils/courbes.py mesures_*.csv --simulation     # garder aussi les trames simulées (scénario non nul)

Entrée : un ou plusieurs CSV enregistrés par supervision.py (une ligne par trame D).
Sortie : puissance_rotation.png / .svg et tension_rotation.png / .svg, plus un bilan dans le terminal.
Dépendances : pip install matplotlib numpy
"""
import argparse
import csv
import glob
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from supervision import CHAMPS  # même contrat que la trame D

# Palette de référence (thème clair, impression) : une seule série, en bleu ; l'ajustement en encre secondaire
BLEU = "#2a78d6"
BLEU_CLAIR = "#86b6ef"
ENCRE = "#0b0b0b"
ENCRE_2 = "#52514e"
GRILLE = "#e4e3df"
FOND = "#ffffff"


def lire(fichiers, simulation=False):
    """Renvoie (rpm, p_gen, v_gen) des trames retenues : capteurs réels seulement, sauf --simulation."""
    rpm, p, v = [], [], []
    for nom in fichiers:
        with open(nom, newline="", encoding="utf-8") as f:
            for ligne in csv.DictReader(f):
                try:
                    if not simulation and int(ligne["scenario"]) != 0:
                        continue
                    n, pg, vg = float(ligne["rpm"]), float(ligne["p_gen"]), float(ligne["v_gen"])
                except (KeyError, ValueError, TypeError):
                    continue        # ligne incomplète ou fichier d'un autre format
                if n > 0:           # rotor à l'arrêt : aucune information sur la courbe
                    rpm.append(n); p.append(pg); v.append(vg)
    return np.array(rpm), np.array(p), np.array(v)


def par_tranche(x, y, pas, min_points=3):
    """Moyenne et écart type de y par tranche de x de largeur pas (tranches trop peu remplies ignorées)."""
    bords = np.arange(0, x.max() + pas, pas)
    centres, moy, ecart = [], [], []
    for a, b in zip(bords[:-1], bords[1:]):
        sel = (x >= a) & (x < b)
        if sel.sum() >= min_points:
            centres.append((a + b) / 2); moy.append(y[sel].mean()); ecart.append(y[sel].std())
    return np.array(centres), np.array(moy), np.array(ecart)


def ajuster_cube(n, p):
    """P = a * N^3 par moindres carrés, renvoie (a, R²)."""
    a = np.sum(p * n ** 3) / np.sum(n ** 6)
    return a, 1 - np.sum((p - a * n ** 3) ** 2) / np.sum((p - p.mean()) ** 2)


def ajuster_droite(n, v):
    """V = k * N + b, renvoie (k, b, R²)."""
    k, b = np.polyfit(n, v, 1)
    return k, b, 1 - np.sum((v - (k * n + b)) ** 2) / np.sum((v - v.mean()) ** 2)


def preparer_axes(titre, xlabel, ylabel):
    fig, ax = plt.subplots(figsize=(8, 5), dpi=100)
    fig.patch.set_facecolor(FOND)
    ax.set_facecolor(FOND)
    ax.set_title(titre, loc="left", fontsize=15, color=ENCRE, pad=14, fontweight="bold")
    ax.set_xlabel(xlabel, fontsize=12, color=ENCRE_2)
    ax.set_ylabel(ylabel, fontsize=12, color=ENCRE_2)
    ax.grid(True, color=GRILLE, linewidth=0.8)
    ax.set_axisbelow(True)
    for cote in ("top", "right"):
        ax.spines[cote].set_visible(False)
    for cote in ("left", "bottom"):
        ax.spines[cote].set_color(GRILLE)
    ax.tick_params(colors=ENCRE_2, labelsize=11, length=0)
    return fig, ax


def legende(ax):
    ax.set_xlim(left=0)       # bornes fixées après le tracé, sinon l'échelle automatique reste figée
    ax.set_ylim(bottom=0)
    lg = ax.legend(loc="upper left", frameon=False, fontsize=11)
    for t in lg.get_texts():
        t.set_color(ENCRE)


def nuage(ax, x, y):
    # nombreux points : petits et transparents pour montrer la densité sans masquer la moyenne
    ax.scatter(x, y, s=10, color=BLEU_CLAIR, alpha=0.5, linewidths=0, label=f"Mesures ({len(x)} trames)")


def moyenne(ax, c, m, e, unite):
    ax.fill_between(c, m - e, m + e, color=BLEU, alpha=0.12, linewidth=0, label="± écart type")
    ax.plot(c, m, color=BLEU, linewidth=2, marker="o", markersize=6,
            markeredgecolor=FOND, markeredgewidth=2, zorder=4, label=f"Moyenne par tranche ({unite})")


def enregistrer(fig, dossier, nom):
    fig.tight_layout()
    chemins = []
    for ext in ("png", "svg"):
        chemin = os.path.join(dossier, f"{nom}.{ext}")
        fig.savefig(chemin, dpi=300, facecolor=FOND)
        chemins.append(chemin)
    plt.close(fig)
    return chemins


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("csv", nargs="+", help="fichiers CSV de supervision.py (motifs acceptés : mesures_*.csv)")
    p.add_argument("--sortie", default="figures", help="dossier des figures (créé si besoin)")
    p.add_argument("--pas", type=float, default=50, help="largeur des tranches de vitesse, en tr/min")
    p.add_argument("--simulation", action="store_true", help="inclure les trames simulées (scénario 1 à 4)")
    a = p.parse_args()

    fichiers = sorted({f for motif in a.csv for f in (glob.glob(motif) or [motif])})
    manquants = [f for f in fichiers if not os.path.isfile(f)]
    if manquants:
        sys.exit(f"fichier introuvable : {', '.join(manquants)}")
    n, pw, v = lire(fichiers, a.simulation)
    if len(n) < 10:
        sys.exit(f"{len(n)} trame(s) exploitable(s) avec le rotor en rotation : il en faut au moins 10"
                 + ("" if a.simulation else " (les trames simulées sont exclues, voir --simulation)"))

    global plt
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    os.makedirs(a.sortie, exist_ok=True)
    unite = f"{a.pas:g} tr/min"
    c, mp, ep = par_tranche(n, pw, a.pas)
    _, mv, ev = par_tranche(n, v, a.pas)
    coef, r2p = ajuster_cube(n, pw)
    k, b, r2v = ajuster_droite(n, v)
    xs = np.linspace(0, n.max(), 200)

    # Puissance
    fig, ax = preparer_axes("Puissance électrique produite selon la vitesse de rotation",
                            "Vitesse de rotation (tr/min)", "Puissance du générateur (W)")
    nuage(ax, n, pw)
    if len(c):
        moyenne(ax, c, mp, ep, unite)
    ax.plot(xs, coef * xs ** 3, color=ENCRE_2, linewidth=1.5, linestyle="--", zorder=3,
            label=f"Ajustement P = a·N³ (R² = {r2p:.2f})")
    legende(ax)
    sorties = enregistrer(fig, a.sortie, "puissance_rotation")

    # Tension
    fig, ax = preparer_axes("Tension du générateur selon la vitesse de rotation",
                            "Vitesse de rotation (tr/min)", "Tension du générateur (V)")
    nuage(ax, n, v)
    if len(c):
        moyenne(ax, c, mv, ev, unite)
    ax.plot(xs, k * xs + b, color=ENCRE_2, linewidth=1.5, linestyle="--", zorder=3,
            label=f"Droite V = k·N + b, k = {k * 1000:.1f} mV/(tr/min) (R² = {r2v:.2f})")
    legende(ax)
    sorties += enregistrer(fig, a.sortie, "tension_rotation")

    i = int(np.argmax(mp)) if len(mp) else None
    print(f"{len(n)} trames retenues sur {len(fichiers)} fichier(s), de {n.min():.0f} à {n.max():.0f} tr/min")
    if i is not None:
        print(f"Puissance moyenne maximale : {mp[i]:.2f} W vers {c[i]:.0f} tr/min ; maximum instantané {pw.max():.2f} W")
    print(f"Ajustement cubique : P = {coef:.3e} × N³ (R² = {r2p:.3f})")
    print(f"Constante du générateur : {k * 1000:.2f} mV par tr/min, décalage {b:+.2f} V (R² = {r2v:.3f})")
    print("Figures : " + ", ".join(sorties))


if __name__ == "__main__":
    main()
