// agents.h : les quatre agents du système et leurs règles de décision
//
//  Production  : surveille le générateur, publie la puissance, signale la surtension.
//  Batterie    : estime l'état de charge, protège la batterie, accorde ou refuse la décharge.
//  Charge      : porte l'exigence de confort (lampe voulue) et négocie avec la batterie.
//  Superviseur : interface utilisateur, mode AUTO / MANUEL, alarmes, application des décisions.
#pragma once
#include "bus.h"
#include "mesures.h"

enum CodeAlarme : uint8_t {
  AL_AUCUNE = 0, AL_SURTENSION_GEN = 1, AL_SURCHARGE_BAT = 2, AL_BAT_BASSE = 4, AL_SURCHAUFFE = 8, AL_CAPTEUR = 16
};
enum Motif : uint8_t { MOTIF_OK = 0, MOTIF_RESERVE, MOTIF_BAT_BASSE, MOTIF_SURCHAUFFE, MOTIF_NON_VOULU };

struct Consignes {          // exigences de confort fixées par l'utilisateur
  bool    lampeVoulue = true;
  uint8_t reserveSoc  = RESERVE_SOC_DEFAUT;
  bool    modeAuto    = true;
};

class Agent {
 public:
  explicit Agent(IdAgent id) : id_(id) {}
  IdAgent id() const { return id_; }
  virtual void percevoir(const Mesures &) {}
  virtual void recevoir(const Message &, Bus &) {}
 protected:
  IdAgent id_;
};

// ------------------------------------------------------------------
class AgentProduction : public Agent {
 public:
  AgentProduction() : Agent(AG_PRODUCTION) {}
  float energieWh = 0;
  float pGen = 0;
  bool alarme = false;

  void percevoir(const Mesures &m) override { m_ = m; }
  void agir(Bus &bus, float dt_h) {
    pGen = m_.pGen;
    energieWh += pGen * dt_h;
    bus.publier(id_, AG_BATTERIE, MSG_MESURE_PROD, pGen);
    bool surtension = m_.vGen > V_GEN_MAX;
    if (surtension != alarme) {                 // on ne publie qu'au changement d'état
      alarme = surtension;
      bus.publier(id_, AG_SUPERVISEUR, MSG_ALARME, AL_SURTENSION_GEN, surtension);
    }
  }
 private:
  Mesures m_{};
};

// ------------------------------------------------------------------
class AgentBatterie : public Agent {
 public:
  AgentBatterie() : Agent(AG_BATTERIE) {}
  float soc = 50;
  uint8_t alarmes = AL_AUCUNE;
  bool coupureBasse = false;

  void percevoir(const Mesures &m) override { m_ = m; }

  void recevoir(const Message &msg, Bus &bus) override {
    if (msg.type == MSG_MESURE_PROD) {
      pProd_ = msg.valeur;
      evaluer(bus);
    } else if (msg.type == MSG_CONSIGNE) {
      reserve_ = (uint8_t)msg.valeur;
    } else if (msg.type == MSG_DEMANDE_LAMPE) {
      uint8_t motif = MOTIF_OK;
      bool accord = true;
      if (msg.valeur > 0.5) {
        if (alarmes & AL_SURCHAUFFE)      { accord = false; motif = MOTIF_SURCHAUFFE; }
        else if (coupureBasse)            { accord = false; motif = MOTIF_BAT_BASSE; }
        // sous la réserve, on n'accepte que si le vent couvre la consommation de la lampe ;
        // après un refus, il faut remonter de HYST_RESERVE points pour accepter (évite le battement du relais)
        else {
          float seuil = sousReserve_ ? reserve_ + HYST_RESERVE : reserve_;
          sousReserve_ = (soc < seuil && pProd_ < 3.0);
          if (sousReserve_) { accord = false; motif = MOTIF_RESERVE; }
        }
      }
      bus.publier(id_, AG_CHARGE, MSG_REPONSE_LAMPE, motif, accord);
    }
  }

 private:
  Mesures m_{};
  float pProd_ = 0;
  uint8_t reserve_ = RESERVE_SOC_DEFAUT;
  bool sousReserve_ = false;

  void evaluer(Bus &bus) {
    // au repos (pas de charge), la tension donne l'état de charge ; sinon on garde la dernière estimation
    if (pProd_ < 0.5) soc = 0.8 * soc + 0.2 * socDepuisTension(m_.vBat);

    if (m_.vBat < V_BAT_COUPURE) coupureBasse = true;
    else if (m_.vBat > V_BAT_REARME) coupureBasse = false;

    uint8_t a = AL_AUCUNE;
    if (m_.vBat > V_BAT_MAX)  a |= AL_SURCHARGE_BAT;
    if (coupureBasse)         a |= AL_BAT_BASSE;
    if (m_.tBat > T_BAT_MAX)  a |= AL_SURCHAUFFE;
    if (m_.tBat < -50)        a |= AL_CAPTEUR;
    if (a != alarmes) {
      alarmes = a;
      bus.publier(id_, AG_SUPERVISEUR, MSG_ALARME, a, 1);
    }
    bus.publier(id_, AG_CHARGE, MSG_ETAT_BATTERIE, soc, !coupureBasse);
  }
};

// ------------------------------------------------------------------
class AgentCharge : public Agent {
 public:
  AgentCharge() : Agent(AG_CHARGE) {}
  float energieWh = 0;
  bool etatVoulu = false;     // dernière décision de l'agent
  uint8_t dernierMotif = MOTIF_OK;

  void percevoir(const Mesures &m) override { pLampe_ = m.pLampe; }
  void cumuler(float dt_h) { energieWh += pLampe_ * dt_h; }

  void recevoir(const Message &msg, Bus &bus) override {
    if (msg.type == MSG_CONSIGNE) {
      lampeVoulue_ = msg.valeur > 0.5;
    } else if (msg.type == MSG_ETAT_BATTERIE) {
      // à chaque nouvel état batterie, l'agent redemande ce que veut l'utilisateur
      bus.publier(id_, AG_BATTERIE, MSG_DEMANDE_LAMPE, lampeVoulue_ ? 1 : 0);
    } else if (msg.type == MSG_DECISION) {
      relais_ = msg.valeur > 0.5;
    } else if (msg.type == MSG_REPONSE_LAMPE) {
      bool cible = lampeVoulue_ && msg.drapeau;
      etatVoulu = cible;
      dernierMotif = lampeVoulue_ ? (uint8_t)msg.valeur : MOTIF_NON_VOULU;
      if (cible == relais_) {
        dejaPropose_ = relais_;               // situation conforme : rien à proposer
      } else if (cible != dejaPropose_) {     // une seule proposition par changement de situation
        dejaPropose_ = cible;
        bus.publier(id_, AG_SUPERVISEUR, MSG_PROPOSITION, cible ? 1 : 0, dernierMotif);
      }
    }
  }
 private:
  float pLampe_ = 0;
  bool lampeVoulue_ = true;
  bool relais_ = false;
  bool dejaPropose_ = false;
};

// ------------------------------------------------------------------
class AgentSuperviseur : public Agent {
 public:
  AgentSuperviseur() : Agent(AG_SUPERVISEUR) {}
  Consignes consignes;
  bool relaisFerme = false;
  bool propositionEnAttente = false;
  bool propositionValeur = false;
  uint8_t propositionMotif = 0;
  uint8_t alarmes = 0;
  uint32_t decisions = 0;
  Print *journal = nullptr;   // sortie série pour les événements

  void recevoir(const Message &msg, Bus &bus) override {
    if (msg.type == MSG_ALARME) {
      if (msg.de == AG_PRODUCTION) alarmes = msg.drapeau ? (alarmes | AL_SURTENSION_GEN) : (alarmes & ~AL_SURTENSION_GEN);
      else alarmes = (alarmes & AL_SURTENSION_GEN) | (uint8_t)msg.valeur;
      evenement(F("ALARME"), alarmes);
      // sécurité : surchauffe ou batterie basse coupent la lampe, même en mode manuel
      if (alarmes & (AL_SURCHAUFFE | AL_BAT_BASSE)) appliquer(false, bus);
    } else if (msg.type == MSG_PROPOSITION) {
      bool v = msg.valeur > 0.5;
      bool securite = !v && (msg.drapeau == MOTIF_SURCHAUFFE || msg.drapeau == MOTIF_BAT_BASSE);
      if (consignes.modeAuto || securite) {
        appliquer(v, bus);
      } else {
        propositionEnAttente = true;
        propositionValeur = v;
        propositionMotif = msg.drapeau;
        if (journal) {
          journal->print(F("P,")); journal->print(v ? F("ALLUMER") : F("ETEINDRE"));
          journal->print(','); journal->println(msg.drapeau);
        }
      }
    }
  }

  // réponse de l'utilisateur en mode manuel : libre à lui de suivre ou non la proposition
  void repondre(bool accepte, Bus &bus) {
    if (!propositionEnAttente) return;
    propositionEnAttente = false;
    evenement(accepte ? F("PROPOSITION_ACCEPTEE") : F("PROPOSITION_REFUSEE"), propositionValeur);
    if (accepte) appliquer(propositionValeur, bus);
  }

  void forcer(bool ferme, Bus &bus) { appliquer(ferme, bus); }

  void appliquer(bool ferme, Bus &bus) {
    if (ferme && (alarmes & (AL_SURCHAUFFE | AL_BAT_BASSE))) ferme = false;  // interdit
    if (ferme == relaisFerme) return;
    relaisFerme = ferme;
    digitalWrite(PIN_RELAIS, ferme ? HIGH : LOW);
    decisions++;
    bus.publier(id_, AG_TOUS, MSG_DECISION, ferme ? 1 : 0);
    evenement(F("RELAIS"), ferme);
  }

 private:
  void evenement(const __FlashStringHelper *nom, uint32_t v) {
    if (!journal) return;
    journal->print(F("E,")); journal->print(millis()); journal->print(',');
    journal->print(nom); journal->print(','); journal->println(v);
  }
};
