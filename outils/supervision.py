#!/usr/bin/env python3
"""Eolia : supervision PC de la mini-éolienne.

Usages :
  python supervision.py --port COM3                 # affiche les mesures, enregistre un CSV, accepte des commandes
  python supervision.py --port /dev/ttyACM0 --banc  # banc d'essai : déterminisme et charge de communication

Commandes tapées pendant la supervision : MODE AUTO, MODE MANU, LAMPE 0|1, RESERVE n, OK, NON, FORCER 0|1, SIM 0..4, RAZ, STAT
Dépendance : pip install pyserial
"""
import argparse
import csv
import datetime
import sys
import threading
import time

import serial

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


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--port", required=True, help="port série de l'Arduino (COM3, /dev/ttyACM0...)")
    p.add_argument("--banc", action="store_true", help="lancer le banc d'essai multi-agents")
    p.add_argument("--csv", default=f"mesures_{datetime.datetime.now():%Y%m%d_%H%M%S}.csv")
    a = p.parse_args()
    s = ouvrir(a.port)
    try:
        banc(s) if a.banc else superviser(s, a.csv)
    finally:
        s.close()


if __name__ == "__main__":
    main()
