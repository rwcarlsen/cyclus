#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>

#include "cyclus.h"
#include "query_backend.h"
#include "sim_init.h"
#include "sqlite_back.h"
#include "sqlite_db.h"
#include "xml_flat_loader.h"

namespace po = boost::program_options;
using namespace cyclus;

static std::string usage = "Usage:  dyno [opts] <agent-spec> <config-file>";

struct ArgInfo {
  po::variables_map vm;  // Holds parsed/specified cli opts and values
  po::options_description desc;  // Holds cli opts description
  cyclus::AgentSpec spec;
  std::string config;
  int dur;
};

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

struct Ctx {
  SqliteDb* db;
  AgentSpec spec;
  std::string simid;
  int agentid;
  int dur;
};

int ParseCliArgs(ArgInfo* ai, int argc, char* argv[]);
void Results(ArgInfo& ai);
void ReportTrans(Ctx ctx);
void ReportInv(Ctx ctx);
void ReportFlow(Ctx ctx);

int main(int argc, char* argv[]) {
  ArgInfo ai;
  int ret = ParseCliArgs(&ai, argc, argv);
  if (ret > -1) {
    return ret;
  }
  warn_limit = 0;

  // load templated test input file and user specified prototype config
  remove("dynotest.sqlite");
  std::string fpath = Env::GetInstallPath() + "/share/cyclus/dynotest.xml";
  std::stringstream ss;
  try {
    LoadStringstreamFromFile(ss, fpath);
  } catch (Error err) {
    std::cerr << err.what() << "\n";
    return 1;
  }

  std::string infile = ss.str();

  if (ai.spec.path() != "") {
    boost::replace_all(infile, "{{path}}", "<path>" + ai.spec.path() + "</path>");
  } else {
    boost::replace_all(infile, "{{path}}", "");
  }
  boost::replace_all(infile, "{{lib}}", ai.spec.lib());
  boost::replace_all(infile, "{{name}}", ai.spec.agent());
  boost::replace_all(infile, "{{config}}", ai.config);

  ss.str("");
  ss << ai.dur;
  boost::replace_all(infile, "{{dur}}", ss.str());

  FileDeleter fd("dynotest.sqlite");
  FullBackend* fback = new SqliteBack("dynotest.sqlite");

  { // manual block scoping forces proper closure of recorder and sqlite db
    Recorder rec;  // Must be after backend deleter because ~Rec does flushing
    rec.RegisterBackend(fback);

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
    rec.Flush();
    fback->Flush();
  }

  delete fback;

  Results(ai);

  return 0;
}

void Results(ArgInfo& ai) {
  SqliteDb db("dynotest.sqlite", true);
  db.open();

  // populate context
  Ctx ctx;
  ctx.spec = ai.spec;
  ctx.db = &db;
  ctx.dur = ai.dur;

  SqlStatement::Ptr stmt;
  stmt = db.Prepare("SELECT hex(SimId),AgentId FROM AgentEntry;");
  while (stmt->Step()) {
    ctx.simid = stmt->GetText(0, NULL);
    ctx.agentid = stmt->GetInt(1);
    break;
  }

  int exit = -1;
  try {
    stmt = db.Prepare("SELECT ExitTime FROM AgentExit;");
    if (stmt->Step()) {
      exit = stmt->GetInt(0);
    }
  } catch(Error err) { }

  std::cout << "Archetype: " << ctx.spec.str() << "\n";
  std::cout << "Duration: " << ctx.dur << " months\n";
  if (exit == -1) {
    std::cout << "Decommissioned: never\n";
  } else {
    std::cout << "Decommissioned: month " << exit << "\n";
  }

  ReportTrans(ctx);
  ReportInv(ctx);
  ReportFlow(ctx);

  db.close();
}

void ReportFlow(Ctx ctx) {
}
void ReportInv(Ctx ctx) {
}

void ReportTrans(Ctx ctx) {
  SqlStatement::Ptr stmt;

  stmt = ctx.db->Prepare("SELECT COUNT(*) FROM Transactions WHERE SenderId = ?;");
  stmt->BindInt(1, ctx.agentid);
  int nsend;
  if (stmt->Step()) {
    nsend = stmt->GetInt(0);
  }

  stmt = ctx.db->Prepare("SELECT COUNT(*) FROM Transactions WHERE ReceiverId = ?;");
  stmt->BindInt(1, ctx.agentid);
  int nrcv = 0;
  if (stmt->Step()) {
    nrcv = stmt->GetInt(0);
  }

  stmt = ctx.db->Prepare(
      "SELECT SUM(Quantity) FROM Resources AS r INNER JOIN Transactions AS t "
      "ON t.ResourceId = r.ResourceId WHERE t.ReceiverId = ? AND type = 'Material';");
  stmt->BindInt(1, ctx.agentid);
  int qty_rcv = 0;
  if (stmt->Step()) {
    qty_rcv = stmt->GetInt(0);
  }

  stmt = ctx.db->Prepare(
      "SELECT SUM(Quantity) FROM Resources AS r INNER JOIN Transactions AS t "
      "ON t.ResourceId = r.ResourceId WHERE t.SenderId = ? AND type = 'Material';");
  stmt->BindInt(1, ctx.agentid);
  int qty_send = 0;
  if (stmt->Step()) {
    qty_send = stmt->GetInt(0);
  }

  std::cout << "Transactions:\n";
  std::cout << "    * counts: " << nrcv+nsend << " total (" << nrcv
            << " received, " << nsend << " sent)\n";
  std::cout << "    * quantity: " << qty_rcv-qty_send << " kg net (" << qty_rcv
            << " kg received, " << qty_send << " kg sent)\n";
}

int ParseCliArgs(ArgInfo* ai, int argc, char* argv[]) {
  // specify and parse cli flags
  ai->desc.add_options()
      ("help,h", "print help message")
      ("spec", "colon-separated string form of archetype to test")
      ("dur", po::value<int>(), "# of time steps to run for (default 1200)")
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

  ai->dur = 1200;
  if (ai->vm.count("dur")) {
    ai->dur = ai->vm["dur"].as<int>();
  }

  ai->spec = AgentSpec(ai->vm["spec"].as<std::string>());

  std::stringstream ss;
  try {
    LoadStringstreamFromFile(ss, ai->vm["config"].as<std::string>());
  } catch (Error err) {
    std::cerr << err.what() << "\n";
    return 1;
  }
  ai->config = ss.str();

  return -1;
}


