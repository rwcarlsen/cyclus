
#include "ondemand.h"

namespace cyclus {
namespace toolkit {

Ondemand::Ondemand()
    : qty_used_(nullptr),
      usage_buf_frac_(2.0),
      max_(true),
      window_(24),
      empty_thresh_(1e-6){};

Ondemand::~Ondemand(){};

Ondemand& Ondemand::Init(std::list<double>* qty_used, double usage_guess) {
  qty_used_ = qty_used;
  if (qty_used_->size() < 1) {
    qty_used_->clear();
    qty_used_->push_back(usage_guess);
  }
  return *this;
};

Ondemand& Ondemand::use_max() {
  max_ = true;
  return *this;
};

Ondemand& Ondemand::use_avg() {
  max_ = false;
  return *this;
};

Ondemand& Ondemand::usage_buf_frac(double frac) {
  usage_buf_frac_ = frac + 1;
  return *this;
};

Ondemand& Ondemand::empty_thresh(double qty) {
  empty_thresh_ = qty;
  return *this;
};

Ondemand& Ondemand::window(int width) {
  window_ = width;
  return *this;
};

double Ondemand::ToMove(double curr_qty) {
  return std::max(0.0, MovingUsage() * usage_buf_frac_ - curr_qty);
}

double Ondemand::ToHold(double curr_qty) {
  return std::max(curr_qty, curr_qty + ToMove(curr_qty));
}

double Ondemand::MovingUsage() {
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
  } else {  // avg
    double tot = 0;
    for (it = qty_used_->begin(); it != qty_used_->end(); ++it) {
      tot += *it;
    }
    return tot / std::max(1e-100, (double)qty_used_->size());
  }
}

void Ondemand::UpdateUsage(double pre_use_qty, double post_use_qty) {
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

}
}
