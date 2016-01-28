#ifndef CYCLUS_SRC_TOOLKIT_ONDEMAND_H_
#define CYCLUS_SRC_TOOLKIT_ONDEMAND_H_

#include <list>

namespace cyclus {
namespace toolkit {

class Ondemand {
 public:
  Ondemand() : qty_used_(nullptr), usage_buf_frac_(2.0), max_(false), window_(12), empty_thresh_(1e-6) {};
  virtual ~Ondemand() {};

  Ondemand& Init(std::list<double>* qty_used, double usage_guess) {
    qty_used_ = qty_used;
    if (qty_used_->size() < 2) {
      qty_used_->clear();
      qty_used_->push_back(usage_guess);
      qty_used_->push_back(0);
    }
    return *this;
  };
    
  // the total amount to request or fill in for the time step
  double ToMove(double curr_qty) {
    return std::max(0.0, MovingUsage() * usage_buf_frac_ - curr_qty);
  }

  Ondemand& use_max() {max_ = true; return *this;};
  Ondemand& use_avg() {max_ = false; return *this;};
  Ondemand& usage_buf_frac(double frac) {usage_buf_frac_ = frac; return *this;};
  Ondemand& empty_thresh(double qty) {empty_thresh_ = qty; return *this;};
  Ondemand& window(int width) {window_ = width; return *this;};

  double MovingUsage() {
    if (qty_used_->size() < 2) {
      return 0;
    }

    std::list<double>::iterator it;
    if (max_) {
      double max = 0;
      for (it = qty_used_->begin(); it != qty_used_->end(); ) {
        double val = *it;
        ++it;
        if (it == qty_used_->end()) break;

        if (val > max) {
          max = val;
        }
      }
      return max;
    } else { // avg
      double tot = 0;
      for (it = qty_used_->begin(); it != qty_used_->end(); ) {
        double val = *it;
        ++it;
        if (it == qty_used_->end()) break;
        tot += val;
      }
      return tot / std::max(1e-100, qty_used_->size() - 1.00);
    }
  }
  
  void UpdateUsage(double pre_use_qty, double post_use_qty) {
    double prev_qty = qty_used_->back();
    double used = pre_use_qty - post_use_qty; 
    if (pre_use_qty < empty_thresh_) {
      // demand was probably larger than available inventory so assume
      // demand/virtual-usage is equal to usage plus safety buffer
      used = MovingUsage() * usage_buf_frac_;
    }

    qty_used_->pop_back();
    qty_used_->push_back(used);
    qty_used_->push_back(post_use_qty);

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
