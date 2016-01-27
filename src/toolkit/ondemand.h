#ifndef CYCLUS_SRC_TOOLKIT_ONDEMAND_H_
#define CYCLUS_SRC_TOOLKIT_ONDEMAND_H_

#include <vector>

namespace cyclus {
namespace toolkit {

class Ondemand {
 public:
  Ondemand() : usage_buf_frac_(1.5), max_(false), window_(12), empty_thresh_(1e-6) {};
  virtual ~Ondemand() {};

  Ondemand& Init(std::list<double>* qty_used, double usage_guess) {
    qty_used_ = qty_used;
    if (qty_used_->size() < 2) {
      qty_used_->clear();
      qty_used_->push_back(usage_guess);
      qty_used_->push_back(0);
    }
  };
    
  // the total amount to request or fill in for the time step
  double ToMove() const {
    double curr_qty = qty_used_.back();
    return std::max(0.0, MovingUsage() * usage_buf_frac_ - curr_qty);
  }

  void use_max() {max_ = true;};
  void use_avg() {max_ = false;};
  void usage_buf_frac(double frac) {usage_buf_frac_ = frac;};
  void empty_thresh(double qty) {empty_thresh_ = qty;};
  void window(int width) {window_ = width;};

  double MovingUsage() {
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

  void UpdateUsage(double qty_available) {
    double prev_qty = qty_used_->back();
    double used = 0.0
    if (prev_qty < empty_thresh_) {
      // demand was probably larger than available inventory so assume
      // demand/virtual-usage is equal to usage plus safety buffer
      used = MovingUsage() * usage_buf_frac_;
    } else {
      used = qty_available - prev_qty;
    }

    qty_used_->pop_back();
    qty_used_->push_back(used);
    qty_used_->push_back(qty_available);

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
