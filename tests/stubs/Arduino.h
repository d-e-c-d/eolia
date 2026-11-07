// Arduino.h minimal pour tester la logique des agents sur PC
#pragma once
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <algorithm>
using std::min; using std::max;
#define F(x) (reinterpret_cast<const __FlashStringHelper*>(x))
class __FlashStringHelper;
#define HIGH 1
#define LOW 0
#define INPUT_PULLUP 2
#define OUTPUT 1
#define FALLING 2
#define A0 14
inline long constrain_(long x,long a,long b){return x<a?a:(x>b?b:x);}
template<class T,class A,class B> T constrain(T x,A a,B b){return x<a?(T)a:(x>b?(T)b:x);}
extern uint32_t g_millis;
inline uint32_t millis(){return g_millis;}
inline uint32_t micros(){return g_millis*1000;}
extern int g_relais;
inline void digitalWrite(int p,int v){ if(p==3) g_relais=v; }
inline void pinMode(int,int){}
inline int analogRead(int){return 512;}
inline int digitalPinToInterrupt(int p){return p;}
inline void attachInterrupt(int,void(*)(),int){}
inline void noInterrupts(){} inline void interrupts(){}
class Print {
 public:
  void print(const char*s){fputs(s,stdout);} void print(const __FlashStringHelper*s){fputs((const char*)s,stdout);}
  void print(char c){putchar(c);} void print(int v){printf("%d",v);} void print(unsigned v){printf("%u",v);}
  void print(long v){printf("%ld",v);} void print(unsigned long v){printf("%lu",v);}
  void print(double v,int d=2){printf("%.*f",d,v);}
  template<class T> void println(T v){print(v);putchar('\n');}
  void println(){putchar('\n');}
};
