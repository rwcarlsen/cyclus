#include "matl_buy_policy.h"

#include <sstream>

#include "error.h"

#define LG(X) LOG(LEV_##X, "buypol")
#define LGH(X)                                                    \
  LOG(LEV_##X, "buypol") << "policy " << name_ << " (agent "      \
                         << Trader::manager()->prototype() << "-" \
                         << Trader::manager()->id() << "): "

namespace cyclus {
namespace toolkit {

MatlBuyOndemand::MatlBuyOndemand() :
    Trader(NULL),
    name_(""),
    buf_(NULL),
    throughput_(std::numeric_limits<double>::max()),
    quantize_(-1),
    max_(false),
    window_(12),
    usage_buf_frac_(1.5),
    qty_used(NULL) {
  Warn<EXPERIMENTAL_WARNING>(
      "MatlBuyOndemand is experimental and its API may be subject to change");
}

MatlBuyOndemand::~MatlBuyOndemand() {
  if (manager() != NULL) 
    manager()->context()->UnregisterTrader(this);
}

MatlBuyOndemand& MatlBuyOndemand::Init(Agent* manager, ResBuf<Material>* buf, std::vector<double>* qty_used, std::string name);
  Trader::manager_ = manager;
  buf_ = buf;
  name_ = name;
  qty_used_ = qty_used,
  qty_used_->push_back(buf->capacity());
  qty_used_->push_back(0);
  return *this;
}

MatlBuyOndemand& MatlBuyOndemand::Set(std::string commod) {
  CompMap c;
  c[10010000] = 1e-100;
  return Set(commod, Composition::CreateFromMass(c), 1.0);
}

MatlBuyOndemand& MatlBuyOndemand::Set(std::string commod, Composition::Ptr c) {
  return Set(commod, c, 1.0);
}

MatlBuyOndemand& MatlBuyOndemand::Set(std::string commod, Composition::Ptr c,
                                  double pref) {
  CommodDetail d;
  d.comp = c;
  d.pref = pref;
  commod_details_[commod] = d;
  return *this;
}

void MatlBuyOndemand::Start() {
  if (manager() == NULL) {
    std::stringstream ss;
    ss << "No manager set on Buy Policy " << name_;
    throw ValueError(ss.str());
  }
  manager()->context()->RegisterTrader(this);
}

void MatlBuyOndemand::Stop() {
  if (manager() == NULL) {
    std::stringstream ss;
    ss << "No manager set on Buy Policy " << name_;
    throw ValueError(ss.str());
  }
  manager()->context()->UnregisterTrader(this);
}

std::set<RequestPortfolio<Material>::Ptr> MatlBuyOndemand::GetMatlRequests() {
  rsrc_commods_.clear();
  std::set<RequestPortfolio<Material>::Ptr> ports;
  bool make_req = buf_->quantity() < req_when_under_ * buf_->capacity();
  double amt = TotalQty();
  if (!make_req || amt < eps())
    return ports;

  bool excl = exclusive();
  double req_amt = ReqQty();
  int n_req = NReq();
  LGH(INFO3) << "requesting " << amt << " kg via " << n_req << " request(s)";

  // one portfolio for each request
  for (int i = 0; i != n_req; i++) {
    RequestPortfolio<Material>::Ptr port(new RequestPortfolio<Material>());
    std::map<int, std::vector<Request<Material>*> > grps;
    // one request for each commodity
    std::map<std::string, CommodDetail>::iterator it;
    for (it = commod_details_.begin(); it != commod_details_.end(); ++it) {
      std::string commod = it->first;
      CommodDetail d = it->second;
      LG(INFO3) << "  - one " << amt << " kg request of " << commod;
      Material::Ptr m = Material::CreateUntracked(req_amt, d.comp);
      grps[i].push_back(port->AddRequest(m, this, commod, d.pref, excl));
    }

    // if there's more than one commodity, then make them mutual
    if (grps.size() > 1) {
      std::map<int, std::vector<Request<Material>*> >::iterator grpit;
      for (grpit = grps.begin(); grpit != grps.end(); ++grpit) {
        port->AddMutualReqs(grpit->second);
      }
    }
    ports.insert(port);
  }
  
  return ports;
}

double MatlBuyOndemand::Usage() {
  if (qty_used_->size() < 2) {
    return 0;
  }

  std::list<double>::iterator = it;
  if (max_) {
    double max = 0;
    for (it = qty_used_->begin(); it + 1 != qty_used_->end(); ++it) {
      if (*it > max) {
        max = *it;
      }
    }
    return max;
  } else { // avg
    double tot = 0;
    for (it = qty_used_->begin(); it + 1 != qty_used_->end(); ++it) {
      tot += *it;
    }
    return tot / std::max(1e-100, qty_used_->size() - 1);
  }
}

void MatlBuyOndemand::AcceptMatlTrades(
    const std::vector<std::pair<Trade<Material>, Material::Ptr> >& resps) {
  std::vector<std::pair<Trade<Material>, Material::Ptr> >::const_iterator it;
  rsrc_commods_.clear();
  for (it = resps.begin(); it != resps.end(); ++it) {
    rsrc_commods_[it->second] = it->first.request->commodity();
    LGH(INFO3) << "got " << it->second->quantity() << " kg of "
               << it->first.request->commodity();
    buf_->Push(it->second);
  }
  UpdateUsage();
}

void MatlBuyOndemand::UpdateUsage() {
  double prev_qty = qty_used_->back();
  double used = 0.0
  if (prev_qty < std::max(cyclus::eps(), quantize_)) {

    // demand was probably larger than available inventory so assume
    // demand/virtual-usage is equal to usage plus safety buffer
    used = Usage() * usage_buf_frac_;
  } else {
    used = buf_->quantity() - prev_qty;
  }

  qty_used_->pop_back();
  qty_used_->push_back(used);
  qty_used_->push_back(buf_->quantity());

  if (qty_used_->size() > window_) {
    qty_used_->pop_front();
  }
}

}  // namespace toolkit
}  // namespace cyclus
