#ifndef CYCLUS_SRC_TOOLKIT_ONDEMAND_H_
#define CYCLUS_SRC_TOOLKIT_ONDEMAND_H_

#include <list>

namespace cyclus {
namespace toolkit {

/// sample usage for managing resource requests:
///
///   * Create and initialize the Ondemand object inside your archetype.  The
///   ondemand object does not have any of its own mutable state, so you need
///   to create a vector for it with an appropriate state variable pragma.  You
///   will also need to create state variables if you want to allow users to
///   customize any behavior of the ondemand object:
///
///     Class MyArchetype : public cyclus::Facility {
///       ...
///
///       virtual void EnterNotify() {
///        od.Init(&od_history, od_inv_best_guess)
///       }
///       ...
///       Ondemand od;
///
///       #pragma cyclus var {...}
///       double od_inv_best_guess;
///
///       #pragma cyclus var {...}
///       std::list<double> od_history;
///       ...
///     };
///
///   * Whenever the archetype uses resources (e.g. a fab pulls fissile material
///   from its input/inventory fissile stream), it should call UpdateUsage with
///   the quantity in inventory before and after usage respectively:
///
///      Tick() {
///        double before_qty = mybuf.quantity();
///        // use material (e.g. fabricate fresh reactor fuel by combining streams)
///        Material::Ptr m = mybuf.Pop();
///        ...
///        double after_qty = mybuf.quantity();
///        od.UpdateUsage(before_qty, after_qty);
///
///      }
///
///   * When requesting resources to fill the ondemand managed buffer, ask the
///   ondemand object to determine how much should be requested:
///
///      ... GetMatlRequests(...) {
///        double qty_to_request = od.ToMove(mybuf.quantity())
///        ...
///      }
///
class Ondemand {
 public:
  Ondemand();
  virtual ~Ondemand();

  // usage_guess is ignored if qty_used is already initialized with one or
  // more entries.
  Ondemand& Init(std::list<double>* qty_used, double usage_guess);
    
  Ondemand& use_max();
  Ondemand& use_avg();
  // the fraction of usage rate to keep on-hand - this should be at least 1.0.
  Ondemand& usage_buf_frac(double frac);
  Ondemand& empty_thresh(double qty);
  Ondemand& window(int width);

  // the total amount to request/move for this time step given the current
  // quantity in the buffer/inventory being managed.
  double ToMove(double curr_qty);

  // the total amount to be holding in order to maintain an appropriate buffer
  // quantity.  This is complementary to ToMove.  Use one or ther other
  // depending on whether you are deciding how much material to move to/from the
  // buffer or if you are deciding on how big the buffer should be.
  double ToHold(double curr_qty);

  double MovingUsage();
  
  void UpdateUsage(double pre_use_qty, double post_use_qty);

 private:
  double empty_thresh_;
  double usage_buf_frac_;
  bool max_;
  int window_;
  std::list<double>* qty_used_;
};

}  // namespace toolkit
}  // namespace cyclus

#endif
