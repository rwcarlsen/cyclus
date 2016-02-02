
#include "toolkit/ondemand.h"

#include <gtest/gtest.h>

namespace cyclus {
namespace toolkit {

class Chain {
 public:
   Chain() {};

   Chain& Add(Ondemand* od, double qty = 0) {
     ods.push_back(od);
     qtys.push_back(qty);
     prev_qtys.push_back(qty);
     return *this;
   }
   double qty(Ondemand* od) {
     for (int i = 0; i < ods.size(); i++) {
       if (ods[i] == od) {
         return qtys[i];
       }
     }
     return -1;
   }

   // returns the amound successfully pulled out of the last added ondemand
   // tracker's inventory.
   double Step(double supply_qty, double demand_qty) {
     double demand_usage = std::min(demand_qty, qtys.back());

     std::vector<double> tomoves;

     ods[0]->UpdateUsage(prev_qtys[0], qtys[0]);
     supply_usage = ods[0]->ToMove(qtys[0]);
     if (supply_qty >= 0) {
       supply_usage = std::min(supply_qty, supply_usage);
     }
     tomoves.push_back(supply_usage);

     for (int i = 1; i < qtys.size(); i++) {
       ods[i]->UpdateUsage(prev_qtys[i], qtys[i]);
       double tomove = std::min(qtys[i-1], ods[i]->ToMove(qtys[i]));
       tomoves.push_back(tomove);
     }

     for (int i = 0; i < qtys.size(); i++) {
       qtys[i] += tomoves[i];
     }

     tomoves.push_back(demand_usage);
     for (int i = 0; i < qtys.size(); i++) {
       prev_qtys[i] = qtys[i];
       qtys[i] -= tomoves[i+1];
     }

     return demand_usage;
   }

   std::string status() {
     std::stringstream ss;
     for (int i = 0; i < qtys.size(); i++) {
       if (i > 0) {
         ss << ", ";
       }
       ss << "ch" << i+1 << " = " << qtys[i];
     }
     return ss.str();
   }

   double supply_usage;
   std::vector<Ondemand*> ods;
   std::vector<double> qtys;
   std::vector<double> prev_qtys;
};

void CheckBuy(double usage_guess, int window, double frac, std::vector<double> recv, std::vector<double> used, std::vector<double> want) {
  std::list<double> qty_used;
  Ondemand od;
  od.Init(&qty_used, usage_guess).window(window).usage_buf_frac(frac);

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

TEST(OndemandTests, Chain) {
  std::list<double> h1;
  std::list<double> h2;
  std::list<double> h3;
  std::list<double> h4;

  Ondemand b1;
  Ondemand b2;
  Ondemand b3;
  Ondemand b4;
  b1.Init(&h1, 1).usage_buf_frac(2);
  b2.Init(&h2, 1).usage_buf_frac(2);
  b3.Init(&h3, 1).usage_buf_frac(2);
  b4.Init(&h4, 1).usage_buf_frac(2);

  Chain ch;
  ch.Add(&b1);
  ch.Add(&b2);
  ch.Add(&b3);
  ch.Add(&b4);

  double demand = 30;
  double supply = -1;
  double surplus = 2000;
  double deficit = 0;
  for (int i = 0; i < 1000; i++) {
    double usage = ch.Step(supply + surplus, demand + deficit);
    deficit += demand - usage;
    surplus += supply - ch.supply_usage;
    deficit = std::max(0.0, deficit);
    std::cout << ch.status() << ", deficit = " << deficit << "\n";
  }
}

}
}

