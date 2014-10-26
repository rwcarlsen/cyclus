#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/string_generator.hpp>

#include "cyclus.h"
#include "hdf5_back.h"
#include "pyne.h"
#include "query_backend.h"
#include "sim_init.h"
#include "sqlite_back.h"
#include "xml_file_loader.h"
#include "xml_flat_loader.h"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

static std::string usage = "Usage:   snaptest [opts] <agent-spec> <config-file>";

using namespace cyclus;

struct ArgInfo {
  po::variables_map vm;  // Holds parsed/specified cli opts and values
  po::options_description desc;  // Holds cli opts description
  cyclus::AgentSpec spec;
  std::string config;
};

int ParseCliArgs(ArgInfo* ai, int argc, char* argv[]);

class FileDeleter {
 public:
  FileDeleter(std::string path) {
    path_ = path;
    if (boost::filesystem::exists(path_))
      remove(path_.c_str());
  }

  ~FileDeleter() {
    if (boost::filesystem::exists(path_))
      remove(path_.c_str());
  }

 private:
  std::string path_;
};

int main(int argc, char* argv[]) {
  ArgInfo ai;
  int ret = ParseCliArgs(&ai, argc, argv);
  if (ret > -1) {
    return ret;
  }
  warn_limit = 0;

  // load templated test input file and user specified prototype config
  std::string fpath = Env::GetInstallPath() + "/share/cyclus/snaptest.xml";
  std::stringstream ss;
  LoadStringstreamFromFile(ss, fpath);
  std::string infile = ss.str();

  if (ai.spec.path() != "") {
    boost::replace_all(infile, "{{path}}", "<path>" + ai.spec.path() + "</path>");
  } else {
    boost::replace_all(infile, "{{path}}", "");
  }
  boost::replace_all(infile, "{{lib}}", ai.spec.lib());
  boost::replace_all(infile, "{{name}}", ai.spec.agent());
  boost::replace_all(infile, "{{config}}", ai.config);

  //FileDeleter fd("snaptest.sqlite");
  FullBackend* fback = new SqliteBack("snaptest.sqlite");
  RecBackend::Deleter bdel;
  Recorder rec;  // Must be after backend deleter because ~Rec does flushing
  rec.RegisterBackend(fback);
  bdel.Add(fback);

  try {
    XMLFlatLoader l(&rec, fback, infile, false);
    l.LoadSim();
  } catch (cyclus::Error e) {
    std::cout << e.what() << "\n";
    return 1;
  }

  SimInit si;
  si.Restart(fback, rec.sim_id(), 0); // creates first snapshot
  si.recorder()->RegisterBackend(fback);
  si.context()->Snapshot(); // schedule snapshot for start first timestep (0).
  si.timer()->RunSim();

  // compare agent-state rows for the two sim id's for each time zero snapshot.
  rec.Flush();
  return 0;
}

int ParseCliArgs(ArgInfo* ai, int argc, char* argv[]) {
  // specify and parse cli flags
  ai->desc.add_options()
      ("help,h", "print help message")
      ("spec", "colon-separated string form of archetype to test") 
      ("config", "path to xml config snippet for archetype")
      ;

  po::positional_options_description p;
  p.add("spec", 1);
  p.add("config", 1);

  try {
    po::store(po::command_line_parser(argc, argv).
              options(ai->desc).positional(p).run(), ai->vm);
  } catch(std::exception err) {
    std::cout << "Invalid arguments.\n"
              <<  usage << "\n\n"
              << ai->desc << "\n";
    return 1;
  }
  po::notify(ai->vm);

  // pull out and process passed options/flags
  if (ai->vm.count("help") > 0) {
    std::cout << usage << "\n\n"
              << ai->desc << "\n";
    return 0;
  }

  ai->spec = AgentSpec(ai->vm["spec"].as<std::string>());

  std::stringstream ss;
  LoadStringstreamFromFile(ss, ai->vm["config"].as<std::string>());
  ai->config = ss.str();

  return -1;
}


