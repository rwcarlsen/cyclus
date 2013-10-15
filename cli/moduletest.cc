
#include <iostream>
#include <string>
#include <stdio.h>
#include <boost/program_options.hpp>

#include "cyclus.h"
#include "dynamic_module.h"

namespace po = boost::program_options;
namespace cyc = cyclus;

po::variables_map ParseArgs(int argc, char* argv[]);

int main(int argc, char* argv[]) {
  po::variables_map vm = ParseArgs(argc, argv);
  std::string path = cyc::Env::PathBase(argv[0]);
  cyc::Env::SetCyclusRelPath(path);

  if (vm.count("module-name") == 0) {
    std::cout << "missing positional argument for module-name\n";
    return 1;
  }

  std::string name = vm["module-name"].as<std::string>();
  cyc::Timer ti;
  cyc::EventManager em;
  cyc::Context ctx(&ti, &em);

  try {
    cyc::DynamicModule dyn(name);
    cyc::Model* m = dyn.ConstructInstance(&ctx);

    if (vm.count("info") > 0) {
      std::cout << "Impl: " << name << "\n";
      std::cout << "Type: " << m->ModelType() << "\n";
    } else if (vm.count("schema") > 0) {
      std::cout << m->schema() << "\n";
    } else {
      std::cout << "Impl: " << name << "\n";
      std::cout << "Type: " << m->ModelType() << "\n";
    }

    delete m;
    return 0;
  } catch (cyc::Error err) {
    std::cout << err.what() << "\n";
    return 1;
  }
  return 1;
}

po::variables_map ParseArgs(int argc, char* argv[]) {
  // parse command line options
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help,h", "produce help message")
    ("info", "print basic model info (name, type, etc.)")
    ("schema", "print the module xml rng schema")
    ("module-name", po::value<std::string>(), "module name")
    ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  po::positional_options_description p;
  p.add("module-name", 1);

  //po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).
            options(desc).positional(p).run(), vm);
  po::notify(vm);

  return vm;
}
