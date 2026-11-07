// config.h : brochage et seuils de la mini-éolienne Eolia
// Toutes les valeurs à calibrer sont regroupées ici.
#pragma once
#include <Arduino.h>

// ---------- Brochage Arduino UNO ----------
// I2C : A4 = SDA, A5 = SCL (INA219 n°1 et n°2)
const uint8_t PIN_HALL     = 2;   // A3144, sortie vers D2 (interruption INT0), pull-up interne
const uint8_t PIN_RELAIS   = 3;   // entrée L293D (IN1), la sortie OUT1 pilote la bobine du relais
const uint8_t PIN_LCD_D7   = 4;
const uint8_t PIN_LCD_D6   = 5;
const uint8_t PIN_LCD_D5   = 6;
const uint8_t PIN_LCD_D4   = 7;
const uint8_t PIN_LED      = 8;   // LED rouge + résistance 220 ohms
const uint8_t PIN_BUZZER   = 9;   // buzzer actif
const uint8_t PIN_BOUTON   = 10;  // bouton vers GND, pull-up interne
const uint8_t PIN_LCD_E    = 11;
const uint8_t PIN_LCD_RS   = 12;
const uint8_t PIN_NTC      = A0;  // 5 V -> 10 k -> A0 -> NTC 10 k -> GND

// ---------- Capteurs ----------
const uint8_t ADR_INA_PROD   = 0x40; // INA219 n°1 : sortie générateur
const uint8_t ADR_INA_CHARGE = 0x41; // INA219 n°2 : batterie vers lampe (pont A0 soudé)
const uint8_t AIMANTS_PAR_TOUR = 1;  // nombre d'aimants collés sur l'axe
const uint16_t ANTI_REBOND_HALL_US = 2000;  // fronts plus rapprochés ignorés (30 000 tr/min avec un aimant)
const uint16_t ARRET_HALL_MS = 2000;        // sans front pendant ce délai, le rotor est considéré à l'arrêt

// ---------- Calibration (python outils/supervision.py --calib, voir docs/calibration.md) ----------
// Valeur corrigée = gain * valeur brute + décalage. Valeurs neutres tant que rien n'est calibré.
const float CAL_VGEN_GAIN       = 1.0;
const float CAL_VGEN_DECALAGE   = 0.0;    // V
const float CAL_IGEN_GAIN       = 1.0;
const float CAL_IGEN_DECALAGE   = 0.0;    // A
const float CAL_VBAT_GAIN       = 1.0;
const float CAL_VBAT_DECALAGE   = 0.0;    // V
const float CAL_ILAMPE_GAIN     = 1.0;
const float CAL_ILAMPE_DECALAGE = 0.0;    // A
// Thermistance : modèle bêta, R0 ramenée à 25 °C, RSERIE mesurée hors circuit au multimètre
const float NTC_BETA   = 3950.0;
const float NTC_R0     = 10000.0;    // ohms à 25 °C
const float NTC_RSERIE = 10000.0;    // ohms

// ---------- Seuils de protection ----------
const float V_GEN_MAX      = 25.0;  // V : l'INA219 accepte 26 V au maximum
const float V_BAT_MAX      = 14.6;  // V : surcharge batterie plomb
const float V_BAT_COUPURE  = 11.8;  // V : coupure de la lampe (décharge profonde)
const float V_BAT_REARME   = 12.2;  // V : réarmement après coupure (hystérésis)
const float T_BAT_MAX      = 45.0;  // °C : surchauffe

// ---------- Estimation de l'état de charge (agent Batterie) ----------
// Comptage d'énergie : SOC += 100 * (RENDEMENT_CHARGE * P_gen - P_lampe) * dt / CAPACITE_BAT_WH,
// recalé lentement sur la courbe tension / état de charge quand la batterie ne charge pas.
const float CAPACITE_BAT_WH  = 84.0;   // Wh : 12 V x 7 Ah, à adapter à la batterie utilisée
const float RENDEMENT_CHARGE = 0.85;   // part de l'énergie produite réellement stockée (chargeur et batterie)
const float P_CHARGE_MIN     = 0.5;    // W : en dessous, la batterie est considérée hors charge
const float R_INTERNE_BAT    = 0.05;   // ohms : compense la chute de tension due au courant de la lampe
const float TAU_RECALAGE_S   = 600.0;  // s : constante de temps du recalage sur la tension
const float CAPACITE_SIM_WH  = 0.0222222; // Wh : capacité réduite de la simulation accélérée (un cycle vaut environ une minute)

// ---------- Exigences de confort par défaut (modifiables par l'utilisateur) ----------
const uint8_t RESERVE_SOC_DEFAUT = 40;  // % de charge à garder en réserve
const uint8_t HYST_RESERVE       = 10;  // points de % à regagner avant de rallumer

// ---------- Cadencement ----------
const uint16_t PERIODE_CYCLE_MS  = 200;   // un cycle de négociation des agents
const uint16_t PERIODE_TRAME_MS  = 1000;  // envoi d'une trame de mesures au PC
const uint16_t PERIODE_LCD_MS    = 500;
const uint8_t  TOURS_MAX_BUS     = 8;     // tours maximum de messages par cycle
