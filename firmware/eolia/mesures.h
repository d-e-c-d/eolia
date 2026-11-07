// mesures.h : acquisition réelle (capteurs) ou simulée (scénarios reproductibles)
#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include "config.h"

struct Mesures {
  float vGen, iGen, pGen;      // V, A, W côté générateur
  float vBat;                  // V batterie (tension bus de l'INA219 n°2)
  float iLampe, pLampe;        // A, W côté lampe
  float tBat;                  // °C
  uint16_t rpm;
};

// Valeurs brutes des capteurs, avant toute correction de calibration
struct Brutes {
  float vGen, iGen;            // V, A (INA219 n°1)
  float vBat, iLampe;          // V, A (INA219 n°2)
  float adcNtc;                // 0..1023
};

// ---------- Corrections de calibration (coefficients dans config.h) ----------
inline float corriger(float brut, float gain, float decalage) { return gain * brut + decalage; }

// Pont 5 V, RSERIE, A0, NTC, GND. Renvoie -99 si le capteur est absent ou en court-circuit.
float temperatureNtc(float adc) {
  if (adc < 1 || adc > 1022) return -99;
  float r = NTC_RSERIE * adc / (1023.0 - adc);
  float invT = 1.0 / 298.15 + log(r / NTC_R0) / NTC_BETA;
  return 1.0 / invT - 273.15;
}

// ---------- Lecture du capteur Hall ----------
// On mesure la période entre deux fronts (résolution de quelques µs) plutôt que de compter
// les impulsions sur un cycle de 200 ms, qui ne donnerait que des multiples de 300 tr/min.
volatile uint32_t g_dernierFrontUs = 0;   // instant du dernier front retenu
volatile uint32_t g_sommePeriodesUs = 0;  // somme des périodes mesurées depuis la dernière lecture
volatile uint16_t g_nbPeriodes = 0;
void isrHall() {
  uint32_t t = micros();
  uint32_t d = t - g_dernierFrontUs;
  if (d < ANTI_REBOND_HALL_US) return;
  if (d < ARRET_HALL_MS * 1000UL) { g_sommePeriodesUs += d; g_nbPeriodes++; }  // le premier front après un arrêt ne donne pas de période
  g_dernierFrontUs = t;
}

// Vitesse de rotation à partir des périodes mesurées pendant le cycle.
// Sans nouveau front, la dernière période est gardée, mais la vitesse ne peut pas dépasser
// ce qu'indique le temps écoulé depuis le dernier front : elle décroît donc quand le rotor ralentit.
uint16_t rpmHall(uint32_t sommeUs, uint16_t nb, uint32_t depuisDernierUs, uint32_t &periodeUs) {
  if (nb) periodeUs = sommeUs / nb;
  if (periodeUs == 0 || depuisDernierUs >= ARRET_HALL_MS * 1000UL) { periodeUs = 0; return 0; }
  uint32_t p = max(periodeUs, depuisDernierUs);
  return 60000000UL / ((uint32_t)p * AIMANTS_PAR_TOUR);
}

// ---------- Courbe tension / état de charge d'une batterie plomb 12 V au repos ----------
float socDepuisTension(float v) {
  const float tv[]  = {11.8, 12.0, 12.2, 12.4, 12.6, 12.7};
  const float tsoc[] = {0,   25,   50,   75,   95,   100};
  if (v <= tv[0]) return 0;
  for (uint8_t i = 1; i < 6; i++)
    if (v <= tv[i]) return tsoc[i-1] + (v - tv[i-1]) * (tsoc[i] - tsoc[i-1]) / (tv[i] - tv[i-1]);
  return 100;
}
float tensionDepuisSoc(float soc) {
  const float tv[]  = {11.8, 12.0, 12.2, 12.4, 12.6, 12.7};
  const float tsoc[] = {0,   25,   50,   75,   95,   100};
  if (soc <= 0) return tv[0];
  for (uint8_t i = 1; i < 6; i++)
    if (soc <= tsoc[i]) return tv[i-1] + (soc - tsoc[i-1]) * (tv[i] - tv[i-1]) / (tsoc[i] - tsoc[i-1]);
  return tv[5];
}

class Acquisition {
 public:
  bool begin() {
    pinMode(PIN_HALL, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_HALL), isrHall, FALLING);
    okProd_ = inaProd_.begin();
    okCharge_ = inaCharge_.begin();
    return okProd_ && okCharge_;
  }
  bool capteursOk() const { return okProd_ && okCharge_; }
  bool inaProdOk() const { return okProd_; }
  bool inaChargeOk() const { return okCharge_; }

  // Valeurs brutes moyennées sur n lectures (n >= 1), utilisées par la commande CAL
  void lireBrutes(Brutes &b, uint8_t n) {
    b = Brutes{};
    for (uint8_t k = 0; k < n; k++) {
      if (okProd_) {
        b.vGen += inaProd_.getBusVoltage_V() + inaProd_.getShuntVoltage_mV() / 1000.0;
        b.iGen += inaProd_.getCurrent_mA() / 1000.0;
      }
      if (okCharge_) {
        b.vBat += inaCharge_.getBusVoltage_V();
        b.iLampe += inaCharge_.getCurrent_mA() / 1000.0;
      }
      b.adcNtc += analogRead(PIN_NTC);
    }
    b.vGen /= n; b.iGen /= n; b.vBat /= n; b.iLampe /= n; b.adcNtc /= n;
  }

  // scenario = 0 : capteurs réels ; 1..4 : simulation
  void choisirScenario(uint8_t s) { scenario_ = s; pas_ = 0; socSim_ = 60; tSim_ = 25; }
  uint8_t scenario() const { return scenario_; }
  float socSimule() const { return socSim_; }   // état de charge réel de la simulation, pour les tests

  // dt_ms : durée du cycle (inutilisée depuis la mesure par période) ; relaisFerme : lampe alimentée
  void lire(Mesures &m, uint16_t /* dt_ms */, bool relaisFerme) {
    if (scenario_ == 0) lireReel(m);
    else simuler(m, relaisFerme);
  }

 private:
  Adafruit_INA219 inaProd_{ADR_INA_PROD};
  Adafruit_INA219 inaCharge_{ADR_INA_CHARGE};
  bool okProd_ = false, okCharge_ = false;
  uint8_t scenario_ = 0;
  uint16_t pas_ = 0;
  float socSim_ = 60, tSim_ = 25;
  uint32_t periodeHallUs_ = 0;

  void lireReel(Mesures &m) {
    Brutes b;
    lireBrutes(b, 1);
    m.vGen = okProd_ ? corriger(b.vGen, CAL_VGEN_GAIN, CAL_VGEN_DECALAGE) : 0;
    m.iGen = okProd_ ? max(0.0f, corriger(b.iGen, CAL_IGEN_GAIN, CAL_IGEN_DECALAGE)) : 0;
    m.pGen = m.vGen * m.iGen;
    m.vBat = okCharge_ ? corriger(b.vBat, CAL_VBAT_GAIN, CAL_VBAT_DECALAGE) : 0;
    m.iLampe = okCharge_ ? max(0.0f, corriger(b.iLampe, CAL_ILAMPE_GAIN, CAL_ILAMPE_DECALAGE)) : 0;
    m.pLampe = m.vBat * m.iLampe;
    m.tBat = temperatureNtc(b.adcNtc);
    noInterrupts();
    uint32_t somme = g_sommePeriodesUs, dernier = g_dernierFrontUs; uint16_t nb = g_nbPeriodes;
    g_sommePeriodesUs = 0; g_nbPeriodes = 0;
    interrupts();
    m.rpm = rpmHall(somme, nb, micros() - dernier, periodeHallUs_);
  }

  // Simulation déterministe : même scénario => mêmes mesures, cycle par cycle.
  // Accélérée : un cycle de 200 ms représente environ une minute réelle.
  void simuler(Mesures &m, bool relaisFerme) {
    float vent;  // m/s
    uint16_t p = pas_;
    switch (scenario_) {
      case 1:  vent = (p < 100) ? 6.0 : (p < 200 ? 1.0 : 4.0); break;     // vent fort, calme, reprise
      case 2:  vent = 2.0; break;                                         // vent faible permanent
      case 3:  vent = (p > 50 && p < 70) ? 14.0 : 5.0; break;             // rafale : surtension
      case 4:  vent = 5.0; tSim_ = min(60.0f, 25.0f + p * 0.2f); break;   // échauffement batterie
      default: vent = 0;
    }
    m.vGen = min(30.0f, vent * 2.0f);                                    // ~2 V par m/s
    m.pGen = (vent < 1.5) ? 0 : 0.03 * vent * vent * vent;                // P ~ v^3
    m.iGen = m.vGen > 0.1 ? m.pGen / m.vGen : 0;
    m.rpm = (uint16_t)(vent * 120);
    m.pLampe = relaisFerme ? 3.0 : 0;                                    // lampe LED 3 W
    socSim_ += (m.pGen * RENDEMENT_CHARGE - m.pLampe) * (PERIODE_CYCLE_MS / 3600000.0) * 100 / CAPACITE_SIM_WH;
    socSim_ = constrain(socSim_, 0, 100);
    m.vBat = tensionDepuisSoc(socSim_) + (m.pGen > 0.5 ? 0.4 : 0);        // tension relevée pendant la charge
    if (scenario_ == 3 && m.vGen > 25) m.vBat = 14.8;
    m.iLampe = m.pLampe / m.vBat;
    m.tBat = tSim_;
    pas_++;
  }
};
