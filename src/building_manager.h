#ifndef CYCLUS_CORE_UTILITY_BUILDING_MANAGER_H_
#define CYCLUS_CORE_UTILITY_BUILDING_MANAGER_H_

#include <vector>
#include <map>

#include "optim/variable.h"
#include "optim/function.h"

#include "commodity_producer.h"
#include "builder.h"

namespace cyclus {
namespace action_building {
/// a struct for a build order: the number of producers to build
struct BuildOrder {
  // constructor
  BuildOrder(int n, action_building::Builder* b,
             supply_demand::CommodityProducer* cp);

  // constituents
  int number;
  action_building::Builder* builder;
  supply_demand::CommodityProducer* producer;
};

/// a struct for a problem instance
struct ProblemInstance {
  /// constructor
  ProblemInstance(Commodity& commod, double demand,
                  cyclus::optim::SolverInterface& sinterface,
                  cyclus::optim::ConstraintPtr constr,
                  std::vector<cyclus::optim::VariablePtr>& soln);

  // constituents
  Commodity& commodity;
  double unmet_demand;
  cyclus::optim::SolverInterface& interface;
  cyclus::optim::ConstraintPtr constraint;
  std::vector<cyclus::optim::VariablePtr>& solution;
};

/**
   The BuildingManager class is a managing entity that makes decisions
   about which objects to build given certain conditions.

   Specifically, the BuildingManager queries the SupplyDemandManager
   to determine if there exists unmet demand for a Commodity, and then
   decides which object(s) amongst that Commodity's producers should be
   built to meet that demand. This decision takes the form of an
   integer program:

   \f[
   \min \sum_{i=1}^{N}n_i*c_i \\
   s.t. \sum_{i=1}^{N}n_i*\phi_i \ge \Phi \\
   n_i \in [0,\infty) \forall i \in I, n_i integer
   \f]

   Where n_i is the number of objects of type i to build, c_i is the
   cost to build the object of type i, \f$\phi_i\f$ is the nameplate
   capacity of the object, and \f$\Phi\f$ is the capacity demand. Here
   the set I corresponds to all producers of a given commodity.
*/
class BuildingManager {
 public:
  /**
     constructor
  */
  BuildingManager();

  /**
     virtual destructor
  */
  virtual ~BuildingManager();

  /**
     register a builder with the manager
     @param builder the builder
   */
  void RegisterBuilder(action_building::Builder* builder);

  /**
     unregister a builder with the manager
     @param builder the builder
   */
  void UnRegisterBuilder(action_building::Builder* builder);

  /**
     given a certain commodity and demand, a decision is made as to
     how many producers of each available type to build this
     function constructs an integer program through the
     SolverInterface
     @param commodity the commodity being demanded
     @param unmet_demand the additional capacity required
     @return a vector of build orders as decided
  */
  std::vector<action_building::BuildOrder> MakeBuildDecision(Commodity& commodity,
                                                             double unmet_demand);

  // protected: @MJGFlag - should be protected. revise when tests can
  // be found by classes in the Utility folder
  /**
     set up the constraint problem
     @param problem the problem instance
   */
  void SetUpProblem(action_building::ProblemInstance& problem);

  /**
     add a variable to the constraint problem
     @param producer the producer to add
     @param builder the builder of that producer
     @param problem the problem instance
   */
  void AddProducerVariableToProblem(supply_demand::CommodityProducer* producer,
                                    action_building::Builder* builder,
                                    action_building::ProblemInstance& problem);

  /**
     given a solution to the constraint problem, construct the
     appropriate set of build orders
     @param orders the set of orders to fill
     @param solution the solution determining how to fill the orders
   */
  void ConstructBuildOrdersFromSolution(std::vector<action_building::BuildOrder>&
                                        orders,
                                        std::vector<optim::VariablePtr>& solution);

 private:
  /// the set of registered builders
  std::set<Builder*> builders_;

  /// a map of variables to their associated builder and producer
  std::map < optim::VariablePtr,
      std::pair<action_building::Builder*, supply_demand::CommodityProducer*> >
      solution_map_;
};

//#include "building_manager_tests.h"
//friend class BuildingManagerTests;
// @MJGFlag - removed for the same reason as above
}
} // namespace cyclus
#endif