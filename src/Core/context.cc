
#include "context.h"

#include "error.h"
#include "event_manager.h"
#include "model.h"
#include "prototype.h"
#include "timer.h"

namespace cyclus {

Context::Context(Timer* ti, RecipeLibrary* rl, EventManager* em)
  : ti_(t), rl_(rl), em_(em) { };

void Context::RegisterModel(std::string name, Model* m) {
  models_[name] = m;
};

template <class T>
T* Context::GetModel(std::string name) {
  if (models_.count(m) == 0) {
    throw KeyError("Invalid model name " + name);
  }

  Model* m = models_[name];
  T* casted(NULL);
  casted = dynamic_cast<T*>(m)
  if (casted == NULL) {
    throw CastError("Invalid model cast for model name " + name);
  }
  return casted;
};

void Context::RegisterProto(std::string name, Prototype* p) {
  protos_[name] = p;
}

template <class T>
T* Context::CreateModel(std::string proto_name) {
  if (protos_.count(proto_name) == 0) {
    throw KeyError("Invalid prototype name " + proto_name);
  }

  Prototype* p = protos_[proto_name];
  T* casted(NULL);
  casted = dynamic_cast<T*>(p.clone())
  if (casted == NULL) {
    throw CastError("Invalid prototype cast for prototype " + proto_name);
  }
  return casted;
};

void Context::RegisterRecipe(std::string name, Composition::Ptr c) {
  recipes_[name] = c;
};

Composition::Ptr Context::GetRecipe(std::string name) {
  if (recipes_.count(name) == 0) {
    throw KeyError("Invalid recipe name " + name);
  }
  return recipes_[name];
};

int Context::time() {
  return ti_->time();
};

int Context::start_time() {
  return ti_->StartTime();
};

int Context::sim_dur() {
  return ti_->SimDur();
};

void Context::RegisterTickListener(TimeAgent* ta) {
  ti_->RegisterTickListener(ta);
};

void Context::RegisterTockListener(TimeAgent* ta) {
  ti_->RegisterTockListener(ta);
};

void Context::RegisterResolveListener(MarketModel* mkt) {
  ti_->RegisterResolveListener(mkt);
};

Event* Context::NewEvent(std::string title) {
  return em_->NewEvent(title);
};

} // namespace cyclus
