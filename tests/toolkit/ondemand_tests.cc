
#include "res_buf_tests.h"

#include <gtest/gtest.h>

namespace cyclus {
namespace toolkit {

TEST(OndemandTests, ScaleUp_NetZero) {

  std::vector<double> recv_history = {30, 30, 30, 30};
  std::vector<double> send_history = {0, 30, 30, 30};
  std::vector<double> want_to_move = {0, 0, 0, 0};

  double usage_guess = 3;
  std::list<double> qty_used;

  Ondemand od;
  od.Init(&qty_used, usage_guess);
  check(od, recv_history, send_history, want_to_move);

}

void check(Ondemand& od, std::vector<double> recv, std::vector<double> send, std::vector<double> want) {
  double qty = 0.0;
  for (int i = 0; i < recv_history.size(); i++) {
    qty += recv[i] - send[i];
    od.UpdateUsage(qty);
    EXPECT_DOUBLE_EQ(want[i], od.ToMove());
  }

}

}
}

