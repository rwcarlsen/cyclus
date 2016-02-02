#ifndef CYCLUS_SRC_TOOLKIT_ONDEMAND_H_
#define CYCLUS_SRC_TOOLKIT_ONDEMAND_H_

#include <list>
#include <iostream>

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
  Ondemand() : qty_used_(nullptr), usage_buf_frac_(2.0), max_(true), window_(24), empty_thresh_(1e-6) {};
  virtual ~Ondemand() {};

  // usage_guess is ignored if qty_used is already initialized with one or
  // more entries.
  Ondemand& Init(std::list<double>* qty_used, double usage_guess) {
    qty_used_ = qty_used;
    if (qty_used_->size() < 1) {
      qty_used_->clear();
      qty_used_->push_back(usage_guess);
    }
    return *this;
  };
    
  // the total amount to request/move for this time step given the current
  // quantity in the buffer/inventory being managed.
  double ToMove(double curr_qty) {
    return std::max(0.0, MovingUsage() * usage_buf_frac_ - curr_qty);
  }

  Ondemand& use_max() {max_ = true; return *this;};
  Ondemand& use_avg() {max_ = false; return *this;};
  Ondemand& usage_buf_frac(double frac) {usage_buf_frac_ = frac; return *this;};
  Ondemand& empty_thresh(double qty) {empty_thresh_ = qty; return *this;};
  Ondemand& window(int width) {window_ = width; return *this;};

  double MovingUsage() {
    if (qty_used_->size() < 1) {
      return 0;
    }

    std::list<double>::iterator it;
    if (max_) {
      double max = 0;
      for (it = qty_used_->begin(); it != qty_used_->end(); ++it) {
        max = std::max(max, *it);
      }
      return max;
    } else { // avg
      double tot = 0;
      for (it = qty_used_->begin(); it != qty_used_->end(); ++it) {
        tot += *it;
      }
      return tot / std::max(1e-100, (double)qty_used_->size());
    }
  }
  
  void UpdateUsage(double pre_use_qty, double post_use_qty) {
    double used = pre_use_qty - post_use_qty; 
    if (pre_use_qty <= empty_thresh_) {
      // didn't have enough initial inventory to satisfy any amount of demand
      // and so we don't actually know what demand was - just keep it same
      used = MovingUsage();
    } else if (pre_use_qty >= MovingUsage() && post_use_qty <= empty_thresh_) {
      // demand was greater than supply and also greater than current avg/max
      // usage.
      used = MovingUsage() * usage_buf_frac_;
    }

    qty_used_->push_back(used);
    if (qty_used_->size() > window_) {
      qty_used_->pop_front();
    }
  }

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
