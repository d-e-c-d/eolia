// Test natif : rejoue les scénarios simulés sans Arduino et vérifie
// les décisions, le déterminisme, la charge de communication et la convergence.
#include "Arduino.h"
uint32_t g_millis = 0; int g_relais = 0;
#include "../firmware/eolia/config.h"
#include "../firmware/eolia/bus.h"
#include "../firmware/eolia/mesures.h"
#include "../firmware/eolia/agents.h"

struct Systeme {
  Bus bus; Acquisition acq; Mesures mes{};
  AgentProduction p; AgentBatterie b; AgentCharge c; AgentSuperviseur s;
  Agent *ag[4] = {&p,&b,&c,&s};
  Print out; uint32_t empreinte = 17, cycle = 0; int toursMax = 0, msgMax = 0;
  void distribuer(){ uint32_t av = bus.total(); int tours=0;
    while(bus.enAttente() && tours<TOURS_MAX_BUS){ tours++; uint8_t n=bus.enAttente();
      for(uint8_t k=0;k<n;k++){ Message m; bus.retirer(m);
        if(m.type==MSG_DECISION) empreinte = empreinte*31 + cycle*2 + (uint32_t)m.valeur;
        for(int i=0;i<4;i++) if((m.vers==AG_TOUS&&i!=m.de)||m.vers==i) ag[i]->recevoir(m,bus);}}
    toursMax=std::max(toursMax,tours); msgMax=std::max<int>(msgMax,bus.total()-av);}
  void init(uint8_t sc, bool autoMode, bool verbeux){ acq.choisirScenario(sc); b.capaciteWh = CAPACITE_SIM_WH; s.consignes.modeAuto=autoMode; s.journal = verbeux? &out : nullptr;
    bus.publier(AG_SUPERVISEUR,AG_CHARGE,MSG_CONSIGNE,1); bus.publier(AG_SUPERVISEUR,AG_BATTERIE,MSG_CONSIGNE,40); distribuer();}
  void pas(){ g_millis+=200; acq.lire(mes,200,s.relaisFerme); for(auto a:ag) a->percevoir(mes);
    c.cumuler(200/3600000.0); p.agir(bus,200/3600000.0); distribuer(); cycle++; }
};

int main(){
  int echecs = 0;
  for (int sc=1; sc<=4; sc++){
    uint32_t emp[3];
    for (int run=0; run<3; run++){
      Systeme S; S.init(sc,true,false); for(int i=0;i<300;i++) S.pas(); emp[run]=S.empreinte;
      if(run==0) printf("Scenario %d : decisions=%u soc=%.0f%% alarmes=%u relais=%d msg_max/cycle=%d tours_max=%d messages=%u perdus=%u Eprod=%.3fWh\n",
        sc,S.s.decisions,S.b.soc,S.s.alarmes,S.s.relaisFerme,S.msgMax,S.toursMax,S.bus.total(),S.bus.perdus(),S.p.energieWh);
      if(run==0 && S.s.decisions>6){ printf("  ECHEC : le relais bat (%u decisions)\n",S.s.decisions); echecs++; }
      if(S.bus.perdus()) { printf("  ECHEC : messages perdus\n"); echecs++; }
      if(S.toursMax>=TOURS_MAX_BUS) { printf("  ECHEC : pas de convergence\n"); echecs++; }
    }
    bool det = emp[0]==emp[1] && emp[1]==emp[2];
    printf("  deterministe sur 3 executions : %s (empreinte %u)\n", det?"oui":"NON", emp[0]);
    if(!det) echecs++;
  }
  // Scénario 4 : la surchauffe doit couper la lampe
  { Systeme S; S.init(4,true,false); for(int i=0;i<300;i++) S.pas();
    if(S.s.relaisFerme || !(S.s.alarmes & AL_SURCHAUFFE)){ printf("ECHEC : surchauffe non geree\n"); echecs++; } else printf("Surchauffe : lampe coupee, OK\n"); }
  // Scénario 3 : la rafale doit déclencher l'alarme de surtension puis la lever
  { Systeme S; S.init(3,true,false); bool vue=false; for(int i=0;i<300;i++){ S.pas(); if(S.s.alarmes & AL_SURTENSION_GEN) vue=true; }
    if(!vue || (S.s.alarmes & AL_SURTENSION_GEN)){ printf("ECHEC : surtension\n"); echecs++; } else printf("Surtension : detectee puis levee, OK\n"); }
  // Mode manuel : une proposition, rien ne bouge sans validation, puis OK l'applique
  { Systeme S; S.init(1,false,true); for(int i=0;i<5;i++) S.pas();
    bool attente = S.s.propositionEnAttente, ferme = S.s.relaisFerme;
    S.s.repondre(true,S.bus); S.distribuer();
    if(!attente || ferme || !S.s.relaisFerme){ printf("ECHEC : mode manuel\n"); echecs++; } else printf("Mode manuel : proposition puis validation, OK\n"); }
  // Mode manuel : un refus ne doit pas provoquer de nouvelle proposition à chaque cycle
  { Systeme S; S.init(2,false,false); for(int i=0;i<5;i++) S.pas(); S.s.repondre(false,S.bus); S.distribuer();
    for(int i=0;i<20;i++) S.pas();
    if(S.s.propositionEnAttente){ printf("ECHEC : propositions repetees apres refus\n"); echecs++; } else printf("Mode manuel : refus respecte, OK\n"); }
  // Etat de charge : le comptage d'énergie suit la simulation, y compris pendant la charge
  // (dérive = écart cumulé entre les variations estimée et simulée, tant qu'aucune des deux ne plafonne)
  for (int sc=1; sc<=4; sc++){ Systeme S; S.init(sc,true,false); float derive=0, ecartMax=0, est=-1, sim=0;
    for(int i=0;i<300;i++){ S.pas(); float e=S.b.soc, s=S.acq.socSimule();
      if(est>0.5 && est<99.5 && e>0.5 && e<99.5 && sim>0.5 && sim<99.5 && s>0.5 && s<99.5) derive += std::fabs((e-est)-(s-sim));
      if(S.b.socInitialise && s>1 && s<99) ecartMax=std::max(ecartMax,std::fabs(e-s));
      est=e; sim=s; }
    bool ok = derive<2 && ecartMax<2;
    printf("Etat de charge scenario %d : estime %.1f%% simule %.1f%% (derive %.2f, ecart max apres recalage %.2f%s) %s\n",
      sc,S.b.soc,S.acq.socSimule(),derive,ecartMax,S.b.socInitialise?"":", jamais hors charge",ok?"OK":"ECHEC"); if(!ok) echecs++; }
  // Capteur Hall : vitesse à partir de la période, sans quantification par pas de 300 tr/min
  { uint32_t per = 0; bool ok = true;
    ok &= rpmHall(3*57143UL, 3, 1000, per) == 1049;        // 3 périodes de 57,143 ms : 1050 tr/min (arrondi inférieur)
    ok &= rpmHall(0, 0, 50000UL, per) == 1049;             // pas de front depuis 50 ms (< période) : dernière période gardée
    ok &= rpmHall(0, 0, 120000UL, per) == 500;             // 120 ms sans front : au plus 500 tr/min
    ok &= rpmHall(0, 0, ARRET_HALL_MS*1000UL, per) == 0 && per == 0;   // arrêt
    ok &= rpmHall(0, 0, 10, per) == 0;                     // pas de période connue
    if(!ok){ printf("ECHEC : vitesse de rotation\n"); echecs++; } else printf("Capteur Hall : vitesse par la periode, OK\n"); }
  // Calibration : correction linéaire et modèle bêta de la NTC
  { bool ok = std::fabs(corriger(12.0f, 1.02f, -0.05f) - 12.19f) < 1e-4;
    float t25 = temperatureNtc(511.5f);                 // pont équilibré : R = RSERIE = R0, donc 25 °C
    float rChaud = NTC_R0 * std::exp(NTC_BETA * (1.0 / 323.15 - 1.0 / 298.15));  // R attendue à 50 °C
    float t50 = temperatureNtc(1023.0 * rChaud / (rChaud + NTC_RSERIE));
    ok = ok && std::fabs(t25 - 25) < 0.05 && std::fabs(t50 - 50) < 0.1
            && temperatureNtc(0) == -99 && temperatureNtc(1023) == -99;
    if(!ok){ printf("ECHEC : calibration (t25=%.2f t50=%.2f)\n", t25, t50); echecs++; } else printf("Calibration : correction et NTC, OK\n"); }
  printf(echecs? "\n%d ECHEC(S)\n" : "\nTOUS LES TESTS PASSENT\n", echecs);
  return echecs;
}
