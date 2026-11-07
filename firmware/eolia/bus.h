// bus.h : bus de messages entre agents (file circulaire, sans allocation dynamique)
#pragma once
#include <Arduino.h>

enum IdAgent : uint8_t { AG_PRODUCTION = 0, AG_BATTERIE, AG_CHARGE, AG_SUPERVISEUR, AG_TOUS, NB_AGENTS = 4 };

enum TypeMsg : uint8_t {
  MSG_MESURE_PROD,     // valeur = puissance produite (W)
  MSG_ETAT_BATTERIE,   // valeur = état de charge (%), drapeau = autorisation de décharge
  MSG_DEMANDE_LAMPE,   // valeur = 1 allumer / 0 éteindre
  MSG_REPONSE_LAMPE,   // drapeau = 1 accord / 0 refus, valeur = motif
  MSG_PROPOSITION,     // mode manuel : action proposée à l'utilisateur
  MSG_DECISION,        // action appliquée
  MSG_ALARME,          // valeur = code d'alarme
  MSG_CONSIGNE         // consigne utilisateur transmise aux agents
};

struct Message {
  IdAgent de;
  IdAgent vers;
  TypeMsg type;
  uint8_t drapeau;
  float   valeur;
};

class Bus {
 public:
  static const uint8_t CAPACITE = 16;

  bool publier(IdAgent de, IdAgent vers, TypeMsg type, float valeur, uint8_t drapeau = 0) {
    if (nb_ >= CAPACITE) { perdus_++; return false; }
    Message &m = file_[(tete_ + nb_) % CAPACITE];
    m.de = de; m.vers = vers; m.type = type; m.valeur = valeur; m.drapeau = drapeau;
    nb_++;
    total_++;
    return true;
  }
  bool retirer(Message &m) {
    if (nb_ == 0) return false;
    m = file_[tete_];
    tete_ = (tete_ + 1) % CAPACITE;
    nb_--;
    return true;
  }
  uint8_t enAttente() const { return nb_; }
  uint32_t total() const { return total_; }
  uint16_t perdus() const { return perdus_; }
  void razCompteurs() { total_ = 0; perdus_ = 0; }

 private:
  Message file_[CAPACITE];
  uint8_t tete_ = 0, nb_ = 0;
  uint32_t total_ = 0;
  uint16_t perdus_ = 0;
};
