#ifndef CYCLUS_SRC_TOOLKIT_MATL_BUY_POLICY_H_
#define CYCLUS_SRC_TOOLKIT_MATL_BUY_POLICY_H_

#include <string>

#include "composition.h"
#include "material.h"
#include "res_buf.h"
#include "trader.h"

namespace cyclus {
namespace toolkit {

/// MatlBuyOndemand performs semi-automatic inventory management of a material
/// buffer by making requests and accepting materials in an attempt to fill the
/// buffer fully every time step according to an (s, S) inventory policy (see
/// [1]).
///
/// For simple behavior, policies virtually eliminate the need to write any code
/// for resource exchange. Just assign a few policies to work with a few buffers
/// and focus on writing the physics and other behvavior of your agent.  Typical
/// usage goes something like this:
///
/// @code
/// class YourAgent : public cyclus::Facility {
///  public:
///   ...
///
///   void EnterNotify() {
///     cyclus::Facility::EnterNotify(); // always do this first
///
///     policy_.Init(this, &inbuf_, "inbuf-label").Set(incommod, comp).Start();
///   }
///   ...
///
///  private:
///   MatlBuyOndemand policy_;
///   ResBuf<Material> inbuf_;
///    ...
/// }
/// @endcode
///
/// The policy needs to be initialized with its owning agent and the material
/// buffer that is is managing. It also needs to be activated by calling the
/// Start function for it to begin participation in resource exchange.  And
/// don't forget to add some commodities to request by calling Set.  All policy
/// configuration should usually occur in the agent's EnterNotify member
/// function.
///
/// [1] Zheng, Yu-Sheng. "A simple proof for optimality of (s, S) policies in
/// infinite-horizon inventory systems." Journal of Applied Probability
/// (1991): 802-810.
///
/// @warn When a policy's managing agent is deallocated, you MUST either
/// call the policy's Stop function or delete the policy. Otherwise SEGFAULT.
class MatlBuyOndemand : public Trader {
 public:
  /// Creates an uninitialized policy.  The Init function MUST be called before
  /// anything else is done with the policy.
  MatlBuyOndemand();

  virtual ~MatlBuyOndemand();

  MatlBuyOndemand& Init(Agent* manager, ResBuf<Material>* buf, std::vector<double>* qty_used, std::string name);
    
  /// Instructs the policy to fill its buffer with requests on the given
  /// commodity of composition c and the given preference.  This must be called
  /// at least once or the policy will do nothing.  The policy can request on an
  /// arbitrary number of commodities by calling Set multiple times.  Re-calling
  /// Set to modify the composition or preference of a commodity that has been
  /// set previously is allowed.
  /// @param commod the commodity name
  /// @param c the composition to request for the given commodity
  /// @param pref the preference value for the commodity
  /// @{
  MatlBuyOndemand& Set(std::string commod);
  MatlBuyOndemand& Set(std::string commod, Composition::Ptr c);
  MatlBuyOndemand& Set(std::string commod, Composition::Ptr c, double pref);
  /// @}

  /// Registers this policy as a trader in the current simulation.  This
  /// function must be called for the policy to begin participating in resource
  /// exchange. Init MUST be called prior to calling this function.  Start is
  /// idempotent.
  void Start();
#ifndef CYCLUS_SRC_TOOLKIT_MATL_BUY_POLICY_H_
#define CYCLUS_SRC_TOOLKIT_MATL_BUY_POLICY_H_

#include <string>

#include "composition.h"
#include "material.h"
#include "res_buf.h"
#include "trader.h"

namespace cyclus {
namespace toolkit {

/// MatlBuyOndemand performs semi-automatic inventory management of a material
/// buffer by making requests and accepting materials in an attempt to fill the
/// buffer fully every time step according to an (s, S) inventory policy (see
/// [1]).
///
/// For simple behavior, policies virtually eliminate the need to write any code
/// for resource exchange. Just assign a few policies to work with a few buffers
/// and focus on writing the physics and other behvavior of your agent.  Typical
/// usage goes something like this:
///
/// @code
/// class YourAgent : public cyclus::Facility {
///  public:
///   ...
///
///   void EnterNotify() {
///     cyclus::Facility::EnterNotify(); // always do this first
///
///     policy_.Init(this, &inbuf_, "inbuf-label").Set(incommod, comp).Start();
///   }
///   ...
///
///  private:
///   MatlBuyOndemand policy_;
///   ResBuf<Material> inbuf_;
///    ...
/// }
/// @endcode
///
/// The policy needs to be initialized with its owning agent and the material
/// buffer that is is managing. It also needs to be activated by calling the
/// Start function for it to begin participation in resource exchange.  And
/// don't forget to add some commodities to request by calling Set.  All policy
/// configuration should usually occur in the agent's EnterNotify member
/// function.
///
/// [1] Zheng, Yu-Sheng. "A simple proof for optimality of (s, S) policies in
/// infinite-horizon inventory systems." Journal of Applied Probability
/// (1991): 802-810.
///
/// @warn When a policy's managing agent is deallocated, you MUST either
/// call the policy's Stop function or delete the policy. Otherwise SEGFAULT.
class MatlBuyOndemand : public Trader {
 public:
  /// Creates an uninitialized policy.  The Init function MUST be called before
  /// anything else is done with the policy.
  MatlBuyOndemand();

  virtual ~MatlBuyOndemand();

  MatlBuyOndemand& Init(Agent* manager, ResBuf<Material>* buf, std::vector<double>* qty_used, std::string name);
    
  /// Instructs the policy to fill its buffer with requests on the given
  /// commodity of composition c and the given preference.  This must be called
  /// at least once or the policy will do nothing.  The policy can request on an
  /// arbitrary number of commodities by calling Set multiple times.  Re-calling
  /// Set to modify the composition or preference of a commodity that has been
  /// set previously is allowed.
  /// @param commod the commodity name
  /// @param c the composition to request for the given commodity
  /// @param pref the preference value for the commodity
  /// @{
  MatlBuyOndemand& Set(std::string commod);
  MatlBuyOndemand& Set(std::string commod, Composition::Ptr c);
  MatlBuyOndemand& Set(std::string commod, Composition::Ptr c, double pref);
  /// @}

  /// Registers this policy as a trader in the current simulation.  This
  /// function must be called for the policy to begin participating in resource
  /// exchange. Init MUST be called prior to calling this function.  Start is
  /// idempotent.
  void Start();

  /// Unregisters this policy as a trader in the current simulation. This
  /// function need only be called if a policy is to be stopped *during* a
  /// simulation (it is not required to be called explicitly at the end). Stop
  /// is idempotent.
  void Stop();

  /// the total amount being requested for the time step
  inline double TotalQty() const {
    return std::min(buf_->space(), std::max(0.0, Usage() - buf_->quantity());
  }

  /// whether trades will be denoted as exclusive or not
  inline bool exclusive() const { return quantize_ > 0; }
  
  /// Returns corresponding commodities from which each material object
  /// was received for the current time step. The data returned by this function
  /// are ONLY valid during the Tock phase of a time step.
  inline const std::map<Material::Ptr, std::string>& rsrc_commods() {
      return rsrc_commods_;
  };

  virtual std::set<RequestPortfolio<Material>::Ptr> GetMatlRequests();
  virtual void AcceptMatlTrades(
      const std::vector<std::pair<Trade<Material>, Material::Ptr> >& resps);

  void throughput(double val) {throughput_ = val;};
  void use_max() {max_ = true;};
  void use_avg() {max_ = false;};
  void usage_buf_frac(double frac) {usage_buf_frac_ = frac;};
  void quantize(double quanta) {quantize_ = quanta;};

 private:
  struct CommodDetail {
    Composition::Ptr comp;
    double pref;
  };

  /// the number of requests made per each commodity
  inline int NReq() const {
    return exclusive() ? static_cast<int>(TotalQty() / quantize_) : 1;
  }
  
  /// the current amount requested per each request
  inline double ReqQty() const {
    return Excl() ? quantize_ : TotalQty();
  }

  ResBuf<Material>* buf_;
  std::string name_;
  double quantize_;
  std::map<Material::Ptr, std::string> rsrc_commods_;
  std::map<std::string, CommodDetail> commod_details_;

  double usage_buf_frac_;
  bool max_;
  double window_;
  std::list<double>* qty_used_;
  double throughput_;

};

}  // namespace toolkit
}  // namespace cyclus

#endif
