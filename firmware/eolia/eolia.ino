// Eolia : supervision multi-agents d'une mini-éolienne (Arduino UNO)
// Projet Smart Realization X5
//
// Chaque cycle (200 ms) :
//   1. acquisition des mesures (capteurs réels ou scénario simulé)
//   2. chaque agent perçoit les mesures
//   3. l'agent Production lance la négociation ; les messages circulent jusqu'à ce que le bus soit vide
//   4. on mesure : durée du cycle, nombre de messages, nombre de tours, mémoire libre
//
// Protocole série (115200 bauds) : voir docs/protocole_serie.md

#include <LiquidCrystal.h>
#include "config.h"
#include "bus.h"
#include "mesures.h"
#include "agents.h"

LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_E, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);
Bus bus;
Acquisition acq;
Mesures mes;
AgentProduction  agProd;
AgentBatterie    agBat;
AgentCharge      agCharge;
AgentSuperviseur agSup;
Agent *agents[NB_AGENTS] = {&agProd, &agBat, &agCharge, &agSup};

// statistiques du dernier cycle
uint32_t numCycle = 0;
uint16_t msgsCycle = 0, msgsMax = 0;
uint8_t  toursCycle = 0, toursMax = 0;
uint32_t dureeCycleUs = 0, dureeMaxUs = 0;
uint32_t empreinte = 17;      // résumé des décisions, pour tester le déterminisme
uint8_t  pageLcd = 0;

uint32_t tCycle = 0, tTrame = 0, tLcd = 0;
char ligne[32];
uint8_t lgLigne = 0;

int ramLibre() {
  extern int __heap_start, *__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

// ---------- Distribution des messages ----------
void distribuer() {
  uint32_t avant = bus.total();
  toursCycle = 0;
  while (bus.enAttente() && toursCycle < TOURS_MAX_BUS) {
    toursCycle++;
    uint8_t n = bus.enAttente();             // on traite un "tour" : les messages présents au début du tour
    for (uint8_t k = 0; k < n; k++) {
      Message m;
      if (!bus.retirer(m)) break;
      if (m.type == MSG_DECISION) empreinte = empreinte * 31 + numCycle * 2 + (uint32_t)m.valeur;
      for (uint8_t i = 0; i < NB_AGENTS; i++)
        if ((m.vers == AG_TOUS && i != m.de) || m.vers == i) agents[i]->recevoir(m, bus);
    }
  }
  msgsCycle = bus.total() - avant;
}

void cycle() {
  uint32_t t0 = micros();
  acq.lire(mes, PERIODE_CYCLE_MS, agSup.relaisFerme);
  for (uint8_t i = 0; i < NB_AGENTS; i++) agents[i]->percevoir(mes);
  const float dt_h = PERIODE_CYCLE_MS / 3600000.0;
  agCharge.cumuler(dt_h);
  agProd.agir(bus, dt_h);
  distribuer();
  dureeCycleUs = micros() - t0;
  numCycle++;
  if (dureeCycleUs > dureeMaxUs) dureeMaxUs = dureeCycleUs;
  if (msgsCycle > msgsMax) msgsMax = msgsCycle;
  if (toursCycle > toursMax) toursMax = toursCycle;
}

// ---------- Alarmes locales ----------
void signaler() {
  bool alarme = agSup.alarmes != 0;
  bool clignote = (millis() / 250) % 2;
  digitalWrite(PIN_LED, alarme ? clignote : (agSup.propositionEnAttente ? clignote : (mes.pGen > 0.5)));
  digitalWrite(PIN_BUZZER, alarme && (agSup.alarmes & (AL_SURTENSION_GEN | AL_SURCHARGE_BAT | AL_SURCHAUFFE)) && clignote);
}

// ---------- Trame de mesures vers le PC ----------
void envoyerTrame() {
  Serial.print(F("D,"));
  Serial.print(millis());           Serial.print(',');
  Serial.print(mes.vGen, 2);        Serial.print(',');
  Serial.print(mes.iGen, 3);        Serial.print(',');
  Serial.print(mes.pGen, 2);        Serial.print(',');
  Serial.print(mes.rpm);            Serial.print(',');
  Serial.print(mes.vBat, 2);        Serial.print(',');
  Serial.print(agBat.soc, 0);       Serial.print(',');
  Serial.print(mes.tBat, 1);        Serial.print(',');
  Serial.print(mes.iLampe, 3);      Serial.print(',');
  Serial.print(mes.pLampe, 2);      Serial.print(',');
  Serial.print(agProd.energieWh, 3);   Serial.print(',');
  Serial.print(agCharge.energieWh, 3); Serial.print(',');
  Serial.print(agSup.relaisFerme);  Serial.print(',');
  Serial.print(agSup.consignes.modeAuto ? F("AUTO") : F("MANU")); Serial.print(',');
  Serial.print(agSup.alarmes);      Serial.print(',');
  Serial.print(msgsCycle);          Serial.print(',');
  Serial.print(toursCycle);         Serial.print(',');
  Serial.print(dureeCycleUs);       Serial.print(',');
  Serial.print(ramLibre());         Serial.print(',');
  Serial.println(acq.scenario());
}

void envoyerStat() {
  Serial.print(F("S,cycles=")); Serial.print(numCycle);
  Serial.print(F(",messages=")); Serial.print(bus.total());
  Serial.print(F(",perdus=")); Serial.print(bus.perdus());
  Serial.print(F(",msg_max_cycle=")); Serial.print(msgsMax);
  Serial.print(F(",tours_max=")); Serial.print(toursMax);
  Serial.print(F(",duree_max_us=")); Serial.print(dureeMaxUs);
  Serial.print(F(",decisions=")); Serial.print(agSup.decisions);
  Serial.print(F(",empreinte=")); Serial.print(empreinte);
  Serial.print(F(",ram_libre=")); Serial.println(ramLibre());
}

// ---------- Affichage LCD (3 pages) ----------
void afficher() {
  lcd.clear();
  if (agSup.propositionEnAttente) {
    lcd.print(F("Proposition:"));
    lcd.setCursor(0, 1);
    lcd.print(agSup.propositionValeur ? F("ALLUMER? bouton") : F("ETEINDRE? bouton"));
    return;
  }
  switch (pageLcd) {
    case 0:
      lcd.print(F("G ")); lcd.print(mes.vGen, 1); lcd.print(F("V ")); lcd.print(mes.pGen, 1); lcd.print('W');
      lcd.setCursor(0, 1);
      lcd.print(F("B ")); lcd.print(mes.vBat, 1); lcd.print(F("V ")); lcd.print(agBat.soc, 0); lcd.print('%');
      break;
    case 1:
      lcd.print(F("Lampe ")); lcd.print(agSup.relaisFerme ? F("ON ") : F("OFF")); lcd.print(' ');
      lcd.print(mes.pLampe, 1); lcd.print('W');
      lcd.setCursor(0, 1);
      lcd.print(F("T ")); lcd.print(mes.tBat, 0); lcd.print(F("C ")); lcd.print(mes.rpm); lcd.print(F("tr"));
      break;
    default:
      lcd.print(F("Prod ")); lcd.print(agProd.energieWh, 2); lcd.print(F("Wh"));
      lcd.setCursor(0, 1);
      lcd.print(agSup.consignes.modeAuto ? F("AUTO ") : F("MANU "));
      lcd.print(F("al:")); lcd.print(agSup.alarmes);
  }
}

// ---------- Bouton : appui court = page ou validation, appui long = remise à zéro ----------
void lireBouton() {
  static bool avant = HIGH;
  static uint32_t tAppui = 0;
  bool etat = digitalRead(PIN_BOUTON);
  if (avant == HIGH && etat == LOW) tAppui = millis();
  if (avant == LOW && etat == HIGH) {
    uint32_t d = millis() - tAppui;
    if (d > 2000) { agProd.energieWh = 0; agCharge.energieWh = 0; }
    else if (d > 40) {
      if (agSup.propositionEnAttente) agSup.repondre(true, bus);
      else pageLcd = (pageLcd + 1) % 3;
    }
  }
  avant = etat;
}

// ---------- Commandes série ----------
void envoyerConsignes() {
  bus.publier(AG_SUPERVISEUR, AG_CHARGE, MSG_CONSIGNE, agSup.consignes.lampeVoulue ? 1 : 0);
  bus.publier(AG_SUPERVISEUR, AG_BATTERIE, MSG_CONSIGNE, agSup.consignes.reserveSoc);
}

void reinitialiser() {
  agProd = AgentProduction(); agBat = AgentBatterie(); agCharge = AgentCharge();
  bool autoMode = agSup.consignes.modeAuto;
  Consignes c = agSup.consignes;
  agSup = AgentSuperviseur(); agSup.consignes = c; agSup.consignes.modeAuto = autoMode;
  agSup.journal = &Serial;
  digitalWrite(PIN_RELAIS, LOW);
  numCycle = 0; msgsMax = 0; toursMax = 0; dureeMaxUs = 0; empreinte = 17;
  bus.razCompteurs();
  envoyerConsignes();
}

// Valeurs brutes (sans correction) moyennées sur 16 lectures, pour outils/supervision.py --calib
void envoyerCalibration() {
  Brutes b;
  acq.lireBrutes(b, 16);
  Serial.print(F("C,"));
  Serial.print(b.vGen, 3);         Serial.print(',');
  Serial.print(b.iGen, 4);         Serial.print(',');
  Serial.print(b.vBat, 3);         Serial.print(',');
  Serial.print(b.iLampe, 4);       Serial.print(',');
  Serial.print(b.adcNtc, 1);       Serial.print(',');
  Serial.print(acq.inaProdOk());   Serial.print(',');
  Serial.println(acq.inaChargeOk());
}

void executer(char *c) {
  for (char *p = c; *p; p++) *p = toupper(*p);
  char *arg = strchr(c, ' ');
  if (arg) { *arg = 0; arg++; }
  int n = arg ? atoi(arg) : 0;

  if (!strcmp(c, "MODE") && arg) {
    agSup.consignes.modeAuto = (arg[0] == 'A');
    agSup.propositionEnAttente = false;
  } else if (!strcmp(c, "LAMPE")) {
    agSup.consignes.lampeVoulue = n != 0;
    envoyerConsignes();
  } else if (!strcmp(c, "RESERVE")) {
    agSup.consignes.reserveSoc = constrain(n, 0, 100);
    envoyerConsignes();
  } else if (!strcmp(c, "OK"))  { agSup.repondre(true, bus);
  } else if (!strcmp(c, "NON")) { agSup.repondre(false, bus);
  } else if (!strcmp(c, "FORCER")) {
    if (!agSup.consignes.modeAuto) agSup.forcer(n != 0, bus);
  } else if (!strcmp(c, "SIM")) {
    acq.choisirScenario(constrain(n, 0, 4));
    reinitialiser();
  } else if (!strcmp(c, "RAZ")) { reinitialiser();
  } else if (!strcmp(c, "STAT")) { envoyerStat(); return;
  } else if (!strcmp(c, "CAL")) { envoyerCalibration(); return;
  } else if (!strcmp(c, "AIDE")) {
    Serial.println(F("I,MODE AUTO|MANU  LAMPE 0|1  RESERVE n  OK  NON  FORCER 0|1  SIM 0..4  RAZ  STAT  CAL"));
    return;
  } else {
    Serial.println(F("I,commande inconnue (AIDE)"));
    return;
  }
  distribuer();
  Serial.println(F("I,ok"));
}

void lireSerie() {
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\n' || ch == '\r') {
      if (lgLigne) { ligne[lgLigne] = 0; executer(ligne); lgLigne = 0; }
    } else if (lgLigne < sizeof(ligne) - 1) {
      ligne[lgLigne++] = ch;
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_RELAIS, OUTPUT); digitalWrite(PIN_RELAIS, LOW);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BOUTON, INPUT_PULLUP);
  lcd.begin(16, 2);
  lcd.print(F("Eolia demarre"));
  agSup.journal = &Serial;
  bool ok = acq.begin();
  if (!ok) {
    Serial.println(F("I,INA219 absent : passage en simulation (SIM 1)"));
    acq.choisirScenario(1);
  }
  envoyerConsignes();
  distribuer();
  Serial.println(F("I,Eolia pret (AIDE pour les commandes)"));
}

void loop() {
  uint32_t t = millis();
  lireSerie();
  lireBouton();
  if (t - tCycle >= PERIODE_CYCLE_MS) { tCycle = t; cycle(); }
  if (t - tTrame >= PERIODE_TRAME_MS) { tTrame = t; envoyerTrame(); }
  if (t - tLcd >= PERIODE_LCD_MS)     { tLcd = t; afficher(); }
  signaler();
}
