#!/usr/bin/env python3
"""Eolia : supervision PC de la mini-éolienne.

Usages :
  python supervision.py --port COM3                 # affiche les mesures, enregistre un CSV, accepte des commandes
  python supervision.py --port /dev/ttyACM0 --banc  # banc d'essai : déterminisme et charge de communication
  python supervision.py --port COM3 --calib         # calibration des INA219 et de la NTC au multimètre

Commandes tapées pendant la supervision : MODE AUTO, MODE MANU, LAMPE 0|1, RESERVE n, OK, NON, FORCER 0|1, SIM 0..4, RAZ, STAT, CAL
Dépendance : pip install pyserial
"""
import argparse
import csv
import datetime
import math
import os
import re
import sys
import threading
import time

CHAMPS = ["t_ms", "v_gen", "i_gen", "p_gen", "rpm", "v_bat", "soc", "t_bat", "i_lampe", "p_lampe",
          "e_prod_wh", "e_lampe_wh", "relais", "mode", "alarmes", "msg_cycle", "tours", "cycle_us",
          "ram_libre", "scenario"]
ALARMES = {1: "surtension générateur", 2: "surcharge batterie", 4: "batterie basse", 8: "surchauffe", 16: "capteur"}
MOTIFS = {0: "conditions réunies", 1: "batterie sous la réserve", 2: "batterie basse",
          3: "surchauffe", 4: "lampe non souhaitée"}


def decoder_alarmes(code):
    code = int(code)
    return ", ".join(n for b, n in ALARMES.items() if code & b) or "aucune"


def ouvrir(port, vitesse=115200):
    import serial            # importé ici : les calculs de calibration restent testables sans pyserial
    s = serial.Serial(port, vitesse, timeout=1)
    time.sleep(2)            # l'UNO redémarre à l'ouverture du port
    s.reset_input_buffer()
    return s


def lire_ligne(s):
    return s.readline().decode("utf-8", errors="replace").strip()


def superviser(s, fichier_csv):
    arret = threading.Event()

    def clavier():
        for ligne in sys.stdin:
            if ligne.strip().upper() in ("Q", "QUITTER"):
                arret.set()
                return
            s.write((ligne.strip() + "\n").encode())

    threading.Thread(target=clavier, daemon=True).start()
    with open(fichier_csv, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["horodatage"] + CHAMPS)
        print(f"Enregistrement dans {fichier_csv}. Tapez une commande (AIDE) ou Q pour quitter.")
        while not arret.is_set():
            l = lire_ligne(s)
            if not l:
                continue
            type_, _, reste = l.partition(",")
            if type_ == "D":
                v = reste.split(",")
                if len(v) != len(CHAMPS):
                    continue
                d = dict(zip(CHAMPS, v))
                w.writerow([datetime.datetime.now().isoformat(timespec="seconds")] + v)
                f.flush()
                print(f"Gen {d['v_gen']:>5} V {d['p_gen']:>5} W {d['rpm']:>4} tr/min | "
                      f"Bat {d['v_bat']:>5} V {d['soc']:>3} % {d['t_bat']:>5} °C | "
                      f"Lampe {'ON ' if d['relais'] == '1' else 'OFF'} {d['p_lampe']:>4} W | "
                      f"{d['mode']} | msg {d['msg_cycle']} cycle {d['cycle_us']} µs | alarmes : {decoder_alarmes(d['alarmes'])}")
            elif type_ == "P":
                action, motif = reste.split(",")
                print(f">>> PROPOSITION : {action.lower()} la lampe ({MOTIFS.get(int(motif), motif)}). Répondez OK ou NON.")
            elif type_ == "E":
                print(f"--- événement : {reste}")
            elif type_ == "C":
                print("--- valeurs brutes : " + ", ".join(f"{k}={v}" for k, v in zip(CHAMPS_CAL, reste.split(","))))
            else:
                print(l)


def stat(s):
    s.write(b"STAT\n")
    fin = time.time() + 3
    while time.time() < fin:
        l = lire_ligne(s)
        if l.startswith("S,"):
            return dict(kv.split("=") for kv in l[2:].split(","))
    raise RuntimeError("pas de réponse STAT")


def banc(s, repetitions=3, duree=20):
    """Critères de la maquette : temps de convergence, messages échangés, déterminisme, mémoire."""
    print(f"Banc d'essai : 4 scénarios x {repetitions} exécutions de {duree} s")
    bilan = []
    for sc in range(1, 5):
        empreintes = []
        for r in range(repetitions):
            s.write(f"SIM {sc}\n".encode())
            time.sleep(duree)
            st = stat(s)
            empreintes.append(st["empreinte"])
            print(f"  scénario {sc} essai {r + 1} : {st}")
        # Remarque : l'horloge réelle peut décaler d'un cycle le moment de la commande SIM ;
        # l'empreinte compte les décisions par numéro de cycle depuis SIM, donc elle doit rester identique.
        bilan.append((sc, len(set(empreintes)) == 1, st))
    s.write(b"SIM 0\n")
    print("\nBilan")
    for sc, det, st in bilan:
        print(f"  scénario {sc} : déterministe = {'oui' if det else 'NON'}, "
              f"messages max par cycle = {st['msg_max_cycle']}, durée max d'un cycle = {int(st['duree_max_us']) / 1000:.2f} ms, "
              f"messages perdus = {st['perdus']}, RAM libre = {st['ram_libre']} octets")


# ---------- Calibration (voir docs/calibration.md) ----------

CHAMPS_CAL = ["v_gen", "i_gen", "v_bat", "i_lampe", "adc_ntc", "ina_prod", "ina_lampe"]
CONFIG_H = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "firmware", "eolia", "config.h")

# (préfixe dans config.h, champ de la trame C, unité, libellé, consigne de mesure)
VOIES = [
    ("CAL_VGEN", "v_gen", "V", "tension générateur (INA219 n°1)",
     "Multimètre en voltmètre entre Vin+ de l'INA219 n°1 et la masse. Faites varier la tension "
     "(alimentation de labo ou générateur à plusieurs régimes)."),
    ("CAL_IGEN", "i_gen", "A", "courant générateur (INA219 n°1)",
     "Multimètre en ampèremètre (calibre 10 A) en série avec l'INA219 n°1. "
     "Premier point conseillé : générateur à l'arrêt, tapez 0."),
    ("CAL_VBAT", "v_bat", "V", "tension batterie (INA219 n°2)",
     "Multimètre en voltmètre aux bornes de la batterie (Vin- de l'INA219 n°2 et la masse)."),
    ("CAL_ILAMPE", "i_lampe", "A", "courant lampe (INA219 n°2)",
     "Multimètre en ampèremètre en série avec la lampe. Le premier point (lampe éteinte, 0 A) est pris "
     "automatiquement ; la lampe est ensuite allumée par FORCER 1."),
]


def droite(points):
    """Moindres carrés reel = gain * brut + decalage. Un seul point : gain seul, décalage nul."""
    if len(points) == 1:
        b, r = points[0]
        return (r / b if abs(b) > 1e-9 else 1.0), 0.0
    n = len(points)
    mb = sum(b for b, _ in points) / n
    mr = sum(r for _, r in points) / n
    sbb = sum((b - mb) ** 2 for b, _ in points)
    if sbb < 1e-12:
        raise ValueError("points trop proches : faites varier la grandeur mesurée")
    gain = sum((b - mb) * (r - mr) for b, r in points) / sbb
    return gain, mr - gain * mb


def r_ntc(adc, r_serie):
    """Résistance de la NTC (pont 5 V, RSERIE, A0, NTC, GND), même formule que le firmware."""
    return r_serie * adc / (1023.0 - adc)


def coefficients_ntc(points, beta=3950.0):
    """points = [(résistance NTC en ohms, température en °C)]. Renvoie (beta, R0 ramenée à 25 °C).
    Un point : le bêta du constructeur est gardé. Deux points ou plus : bêta ajusté (moindres carrés
    sur ln R en fonction de 1/T)."""
    if len(points) >= 2:
        x = [1.0 / (t + 273.15) for _, t in points]
        y = [math.log(r) for r, _ in points]
        if max(x) - min(x) < 1e-5:
            raise ValueError("températures trop proches : il faut au moins 15 °C d'écart")
        beta, ln_r0_inf = droite(list(zip(x, y)))
        # ln R = beta / T + c, donc R0 = exp(beta / 298.15 + c)
        return beta, math.exp(beta / 298.15 + ln_r0_inf)
    r, t = points[0]
    return beta, r * math.exp(beta * (1.0 / 298.15 - 1.0 / (t + 273.15)))


def ecrire_config(valeurs, chemin=CONFIG_H):
    """Remplace la valeur des constantes 'const float NOM = ...;' dans config.h, commentaires conservés."""
    with open(chemin, encoding="utf-8") as f:
        texte = f.read()
    for nom, v in valeurs.items():
        texte, n = re.subn(rf"^(const float\s+{nom}\s*=\s*)[-+0-9.eE]+", rf"\g<1>{v:.6g}", texte, flags=re.M)
        if n != 1:
            raise ValueError(f"{nom} introuvable dans {chemin}")
    with open(chemin, "w", encoding="utf-8") as f:
        f.write(texte)


def lire_cal(s):
    s.write(b"CAL\n")
    fin = time.time() + 3
    while time.time() < fin:
        l = lire_ligne(s)
        if l.startswith("C,"):
            v = l[2:].split(",")
            if len(v) == len(CHAMPS_CAL):
                return {k: float(x) for k, x in zip(CHAMPS_CAL, v)}
    raise RuntimeError("pas de réponse CAL (firmware à jour ?)")


def commander(s, cmd):
    s.write((cmd + "\n").encode())
    time.sleep(0.5)
    s.reset_input_buffer()


def saisir(question):
    """Renvoie un nombre, ou None si l'utilisateur appuie sur Entrée sans rien taper."""
    while True:
        t = input(question).strip().replace(",", ".")
        if not t:
            return None
        try:
            return float(t)
        except ValueError:
            print("  nombre attendu (exemple : 12.47)")


def calibrer_voie(s, prefixe, champ, unite, libelle, consigne, journal):
    print(f"\n=== {libelle} ===\n{consigne}")
    points = []
    if prefixe == "CAL_ILAMPE":
        commander(s, "MODE MANU")
        commander(s, "FORCER 0")
        points.append((lire_cal(s)[champ], 0.0))
        print(f"  point lampe éteinte : brut {points[0][0]:.4f} {unite}")
        commander(s, "FORCER 1")
        print("  Lampe allumée (si elle reste éteinte, une sécurité est active : vérifiez la batterie).")
    try:
        while True:
            reel = saisir(f"  valeur lue au multimètre en {unite} (Entrée pour terminer cette voie) : ")
            if reel is None:
                break
            brut = lire_cal(s)[champ]
            points.append((brut, reel))
            journal.writerow([datetime.datetime.now().isoformat(timespec="seconds"), prefixe, brut, reel])
            print(f"  point {len(points)} : brut {brut:.4f} {unite}, multimètre {reel:.4f} {unite}")
    finally:
        if prefixe == "CAL_ILAMPE":
            commander(s, "FORCER 0")
            commander(s, "MODE AUTO")
    if not points or (prefixe == "CAL_ILAMPE" and len(points) < 2):
        print("  voie ignorée : coefficients inchangés")
        return {}
    gain, decalage = droite(points)
    ecart = max(abs(gain * b + decalage - r) for b, r in points)
    print(f"  gain = {gain:.5f}, décalage = {decalage:+.4f} {unite}, écart maximal restant = {ecart:.4f} {unite}")
    if not 0.8 < gain < 1.2:
        print("  ATTENTION : gain éloigné de 1, vérifiez le câblage et le calibre du multimètre")
    return {f"{prefixe}_GAIN": gain, f"{prefixe}_DECALAGE": decalage}


def calibrer_ntc(s, journal):
    print("\n=== thermistance NTC ===")
    r_serie = saisir("Résistance série (10 kΩ) mesurée au multimètre HORS circuit, en ohms "
                     "(Entrée pour ignorer la NTC) : ")
    if r_serie is None:
        print("  NTC ignorée : coefficients inchangés")
        return {}
    print("Placez la NTC et un thermomètre au contact (même température), attendez une minute.\n"
          "Un point garde le bêta du constructeur ; deux points éloignés d'au moins 15 °C (ambiante et "
          "eau chaude dans un sachet étanche par exemple) ajustent aussi le bêta.")
    points = []
    while True:
        t = saisir("  température lue au thermomètre en °C (Entrée pour terminer) : ")
        if t is None:
            break
        adc = lire_cal(s)["adc_ntc"]
        if not 1 <= adc <= 1022:
            print(f"  ADC = {adc:.0f} : NTC absente ou en court-circuit, point ignoré")
            continue
        r = r_ntc(adc, r_serie)
        points.append((r, t))
        journal.writerow([datetime.datetime.now().isoformat(timespec="seconds"), "NTC", adc, t])
        print(f"  point {len(points)} : ADC {adc:.1f}, R = {r:.0f} Ω à {t:.1f} °C")
    if not points:
        return {"NTC_RSERIE": r_serie}
    beta, r0 = coefficients_ntc(points)
    print(f"  bêta = {beta:.0f} K, R0 (25 °C) = {r0:.0f} Ω")
    if not 3000 < beta < 4500:
        print("  ATTENTION : bêta inhabituel, vérifiez le contact entre la NTC et le thermomètre")
    return {"NTC_RSERIE": r_serie, "NTC_BETA": beta, "NTC_R0": r0}


def calibrer(s, fichier_csv):
    """Assistant de calibration : relève les valeurs brutes (commande CAL) face au multimètre."""
    etat = lire_cal(s)
    if not etat["ina_prod"]:
        print("ATTENTION : INA219 n°1 (0x40) non détecté, la voie générateur renverra 0")
    if not etat["ina_lampe"]:
        print("ATTENTION : INA219 n°2 (0x41) non détecté, la voie batterie et lampe renverra 0")
    print("Calibration Eolia. Pour chaque voie, entrez autant de points que vous voulez ; "
          "Entrée seule passe à la suite.")
    valeurs = {}
    with open(fichier_csv, "w", newline="") as f:
        journal = csv.writer(f)
        journal.writerow(["horodatage", "voie", "brut", "reference"])
        for voie in VOIES:
            valeurs.update(calibrer_voie(s, *voie, journal))
        valeurs.update(calibrer_ntc(s, journal))
    print(f"\nPoints enregistrés dans {fichier_csv}")
    if not valeurs:
        print("Aucun coefficient calculé.")
        return
    print("\nCoefficients pour firmware/eolia/config.h :")
    for nom, v in valeurs.items():
        print(f"  const float {nom} = {v:.6g};")
    if input("\nÉcrire ces valeurs dans config.h ? (o/n) ").strip().lower().startswith("o"):
        ecrire_config(valeurs)
        print("config.h mis à jour. Recompilez et téléversez : pio run -t upload")


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--port", required=True, help="port série de l'Arduino (COM3, /dev/ttyACM0...)")
    p.add_argument("--banc", action="store_true", help="lancer le banc d'essai multi-agents")
    p.add_argument("--calib", action="store_true", help="calibrer les INA219 et la NTC au multimètre")
    p.add_argument("--csv", default=f"mesures_{datetime.datetime.now():%Y%m%d_%H%M%S}.csv")
    a = p.parse_args()
    s = ouvrir(a.port)
    try:
        if a.calib:
            calibrer(s, f"calibration_{datetime.datetime.now():%Y%m%d_%H%M%S}.csv")
        elif a.banc:
            banc(s)
        else:
            superviser(s, a.csv)
    finally:
        s.close()


if __name__ == "__main__":
    main()
