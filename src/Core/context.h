// context.h
#ifndef CONTEXT_H_
#define CONTEXT_H_

#include <map>
#include <string>

#include "composition.h"

namespace cyclus {

class Prototype;
class EventManager;
class Event;
class MarketModel;
class Model;
class Timer;
class TimeAgent;

class Context {
 public:
  Context(Timer* ti, EventManager* em);

  void RegisterModel(std::string name, Model* m);

  template <class T>
  T* GetModel(std::string name);

  void RegisterProto(std::string name, Prototype* p);

  template <class T>
  T* CreateModel(std::string proto_name);

  void RegisterRecipe(std::string name, Composition::Ptr c);
  Composition::Ptr GetRecipe(std::string name);

  void RegisterTickListener(TimeAgent* ta);
  void RegisterTockListener(TimeAgent* ta);
  void RegisterResolveListener(MarketModel* mkt);

  int time();
  int start_time();
  int sim_dur();

  Event* NewEvent(std::string title);

 private:
  std::map<std::string, Model*> models_;
  std::map<std::string, Prototype*> protos_;
  std::map<std::string, Composition::Ptr> recipes_;

  Timer* ti_;
  EventManager* em_;
};

} // namespace cyclus

#endif
