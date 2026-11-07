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
  void init(uint8_t sc, bool autoMode, bool verbeux){ acq.choisirScenario(sc); s.consignes.modeAuto=autoMode; s.journal = verbeux? &out : nullptr;
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
  printf(echecs? "\n%d ECHEC(S)\n" : "\nTOUS LES TESTS PASSENT\n", echecs);
  return echecs;
}
