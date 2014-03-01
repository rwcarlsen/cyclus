// Facilitymodel.cc
// Implements the FacilityModel class

#include "facility_model.h"

#include "timer.h"
#include "query_engine.h"
#include "inst_model.h"
#include "error.h"

#include <stdlib.h>
#include <sstream>
#include "logger.h"
#include <limits>

namespace cyclus {

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FacilityModel::FacilityModel(Context* ctx)
    : Trader(this),
      Model(ctx) {
  model_type_ = "Facility";
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FacilityModel::~FacilityModel() {};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FacilityModel::InitFrom(QueryEngine* qe) {
  Model::InitFrom(qe);

  // get the incommodities
  std::string commod;
  int numInCommod = qe->NElementsMatchingQuery("incommodity");
  for (int i = 0; i < numInCommod; i++) {
    commod = qe->GetElementContent("incommodity", i);
    in_commods_.push_back(commod);
    LOG(LEV_DEBUG2, "none!") << "Facility " << id()
                             << " has just added incommodity" << commod;
  }

  // get the outcommodities
  int numOutCommod = qe->NElementsMatchingQuery("outcommodity");
  for (int i = 0; i < numOutCommod; i++) {
    commod = qe->GetElementContent("outcommodity", i);
    out_commods_.push_back(commod);
    LOG(LEV_DEBUG2, "none!") << "Facility " << id()
                             << " has just added outcommodity" << commod;
  }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FacilityModel::InitFrom(FacilityModel* m) {
  Model::InitFrom(m);
  in_commods_ = m->in_commods_;
  out_commods_ = m->out_commods_;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FacilityModel::Build(Model* parent) {
  Model::Build(parent);
  context()->RegisterTrader(this);
  context()->RegisterTimeListener(this);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string FacilityModel::str() {
  std::stringstream ss("");
  ss << Model::str() << " with: "
     << " lifetime: " << lifetime()
     << " build date: " << birthtime()
     << " decommission date: " << birthtime() + lifetime();
  return ss.str();
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FacilityModel::Decommission() {
  if (!CheckDecommissionCondition()) {
    throw Error("Cannot decommission " + name());
  }

  context()->UnregisterTrader(this);
  context()->UnregisterTimeListener(this);
  Model::Decommission();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FacilityModel::CheckDecommissionCondition() {
  return true;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::vector<std::string> FacilityModel::InputCommodities() {
  return in_commods_;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::vector<std::string> FacilityModel::OutputCommodities() {
  return out_commods_;
}

} // namespace cyclus
