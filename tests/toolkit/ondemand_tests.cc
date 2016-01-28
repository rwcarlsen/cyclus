
#include "toolkit/ondemand.h"

#include <gtest/gtest.h>

namespace cyclus {
namespace toolkit {

void CheckBuy(double usage_guess, int window, double frac, std::vector<double> recv, std::vector<double> used, std::vector<double> want) {
  std::list<double> qty_used;
  Ondemand od;
  od.Init(&qty_used, usage_guess).window(window).usage_buf_frac(frac).use_max();

  double qty = 0.0;
  double surplus = 0;
  double deficit = 0;
  for (int i = 0; i < recv.size(); i++) {
    double touse = std::min(qty, used[i] + deficit);
    deficit += used[i] - touse;
    od.UpdateUsage(qty, qty - touse);
    qty -= touse;
    //EXPECT_DOUBLE_EQ(want[i], od.ToMove(qty));
    double tomove = std::min(recv[i] + surplus, od.ToMove(qty));
    qty += tomove;
    surplus += recv[i] - tomove;
    std::cout << "- qty0=" << qty+touse-tomove << ", qty1=" << qty-tomove << ", qty2=" << qty << "\n";
    std::cout << "  moved " << tomove << "/" << od.ToMove(qty-tomove) << ", used " << touse << ", surplus=" << surplus << ", deficit=" << deficit << "\n";
  }
}

TEST(OndemandTests, ScaleUp_NetZero) {
  std::vector<double> recv_history = {30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30};
  std::vector<double> send_history = {0, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30};
  std::vector<double> want_to_move = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  double usage_guess = 3;
  int window = 12;
  double frac = 2.0;

  CheckBuy(usage_guess, window, frac, recv_history, send_history, want_to_move);
}

}
}

