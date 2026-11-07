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

// ---------- Lecture du capteur Hall ----------
volatile uint16_t g_impulsions = 0;
void isrHall() { g_impulsions++; }

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

  // scenario = 0 : capteurs réels ; 1..4 : simulation
  void choisirScenario(uint8_t s) { scenario_ = s; pas_ = 0; socSim_ = 60; tSim_ = 25; }
  uint8_t scenario() const { return scenario_; }

  // dt_ms : durée écoulée depuis la lecture précédente ; relaisFerme : lampe alimentée
  void lire(Mesures &m, uint16_t dt_ms, bool relaisFerme) {
    if (scenario_ == 0) lireReel(m, dt_ms);
    else simuler(m, relaisFerme);
  }

 private:
  Adafruit_INA219 inaProd_{ADR_INA_PROD};
  Adafruit_INA219 inaCharge_{ADR_INA_CHARGE};
  bool okProd_ = false, okCharge_ = false;
  uint8_t scenario_ = 0;
  uint16_t pas_ = 0;
  float socSim_ = 60, tSim_ = 25;

  void lireReel(Mesures &m, uint16_t dt_ms) {
    m.vGen = okProd_ ? inaProd_.getBusVoltage_V() + inaProd_.getShuntVoltage_mV() / 1000.0 : 0;
    m.iGen = okProd_ ? max(0.0f, inaProd_.getCurrent_mA() / 1000.0f) : 0;
    m.pGen = m.vGen * m.iGen;
    m.vBat = okCharge_ ? inaCharge_.getBusVoltage_V() : 0;
    m.iLampe = okCharge_ ? max(0.0f, inaCharge_.getCurrent_mA() / 1000.0f) : 0;
    m.pLampe = m.vBat * m.iLampe;
    m.tBat = lireNtc();
    noInterrupts();
    uint16_t n = g_impulsions; g_impulsions = 0;
    interrupts();
    m.rpm = dt_ms ? (uint32_t)n * 60000UL / ((uint32_t)dt_ms * AIMANTS_PAR_TOUR) : 0;
  }

  float lireNtc() {
    int adc = analogRead(PIN_NTC);
    if (adc <= 0 || adc >= 1023) return -99;            // capteur absent ou en court-circuit
    float r = NTC_RSERIE * adc / (1023.0 - adc);
    float invT = 1.0 / 298.15 + log(r / NTC_R0) / NTC_BETA;
    return 1.0 / invT - 273.15;
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
    socSim_ += (m.pGen * 0.85 - m.pLampe) * 0.25;                          // capacité réduite pour la démo
    socSim_ = constrain(socSim_, 0, 100);
    m.vBat = tensionDepuisSoc(socSim_) + (m.pGen > 0.5 ? 0.4 : 0);        // tension relevée pendant la charge
    if (scenario_ == 3 && m.vGen > 25) m.vBat = 14.8;
    m.iLampe = m.pLampe / m.vBat;
    m.tBat = tSim_;
    pas_++;
  }
};
