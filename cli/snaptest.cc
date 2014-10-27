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
#include "query_backend.h"
#include "sim_init.h"
#include "sqlite_back.h"
#include "sqlite_db.h"
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

struct Ctx {
  SqliteDb* db;
  std::string simid1;
  std::string simid2;
  int agentid;
  std::string tbl;
  std::string field;
  int type;
};

void TestSnaps();
void TestTable(Ctx ctx);
void TestField(Ctx ctx);

int main(int argc, char* argv[]) {
  ArgInfo ai;
  int ret = ParseCliArgs(&ai, argc, argv);
  if (ret > -1) {
    return ret;
  }
  warn_limit = 0;

  // load templated test input file and user specified prototype config
  remove("snaptest.sqlite");
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

  TestSnaps();

  return 0;
}

void TestSnaps() {
  SqliteDb db("snaptest.sqlite", true);
  db.open();
  Ctx ctx;
  ctx.db = &db;

  // get simulation ids and agent id;
  SqlStatement::Ptr stmt;
  stmt = db.Prepare("SELECT hex(SimId),AgentId FROM AgentEntry;");
  while (stmt->Step()) {
    ctx.simid2 = stmt->GetText(0, NULL);
    ctx.agentid = stmt->GetInt(1);
  }
  stmt = db.Prepare("SELECT hex(SimId) FROM Info WHERE hex(SimId) != ?;");
  stmt->BindText(1, ctx.simid2.c_str());
  while (stmt->Step()) {
    ctx.simid1 = stmt->GetText(0, NULL);
  }

  stmt = db.Prepare("SELECT name FROM sqlite_master WHERE type='table'");

  while (stmt->Step()) {
    ctx.tbl = stmt->GetText(0, NULL);
    if (boost::starts_with(ctx.tbl, "AgentState")) {
      TestTable(ctx);
    }
  }
  db.close();
}

void TestTable(Ctx ctx) {
  SqlStatement::Ptr stmt;
  stmt = ctx.db->Prepare("SELECT Field,Type  FROM FieldTypes WHERE TableName = ?");
  stmt->BindText(1, ctx.tbl.c_str());
  while (stmt->Step()) {
    std::string f = stmt->GetText(0, NULL);
    if (f == "SimId" || f == "SimTime" || f == "AgentId") {
      continue;
    }

    ctx.field = f;
    ctx.type = stmt->GetInt(1);
    TestField(ctx);
  }
}

void TestField(Ctx ctx) {
  SqlStatement::Ptr stmt;
  if (ctx.type > VL_STRING) {
    stmt = ctx.db->Prepare("SELECT hex(SimId),hex("+ctx.field+") FROM "+ctx.tbl+" WHERE SimTime=0 AND AgentId=?");
  } else {
    stmt = ctx.db->Prepare("SELECT hex(SimId),"+ctx.field+" FROM "+ctx.tbl+" WHERE SimTime=0 AND AgentId=?");
  }
  stmt->BindInt(1, ctx.agentid);

  if (!stmt->Step()) {
    throw Error("ERROR: not enough rows in table "+ctx.tbl);
  }
  std::string ida = stmt->GetText(0, NULL);
  std::string idb;

  switch (ctx.type) {
    case BOOL: {
      bool a = stmt->GetInt(1);
      if (!stmt->Step()) {
        throw Error("ERROR: not enough rows in table "+ctx.tbl);
      }
      idb = stmt->GetText(0, NULL);
      bool b = stmt->GetInt(1);
      if (a != b) {
        std::cout << ctx.tbl << "." << ctx.field << " (bool):  " << a << " != " << b << "\n";
      }
      break;
    } case INT: {
      int a = stmt->GetInt(1);
      if (!stmt->Step()) {
        throw Error("ERROR: not enough rows in table "+ctx.tbl);
      }
      idb = stmt->GetText(0, NULL);
      int b = stmt->GetInt(1);
      if (a != b) {
        std::cout << ctx.tbl << "." << ctx.field << " (int):  " << a << " != " << b << "\n";
      }
      break;
   } case FLOAT:
     case DOUBLE: {
      double a = stmt->GetDouble(1);
      if (!stmt->Step()) {
        throw Error("ERROR: not enough rows in table "+ctx.tbl);
      }
      idb = stmt->GetText(0, NULL);
      double b = stmt->GetDouble(1);
      if (a != b) {
        std::cout << ctx.tbl << "." << ctx.field << " (double):  " << a << " != " << b << "\n";
      }
      break;
   } case STRING:
     case VL_STRING: {
      std::string a = stmt->GetText(1, NULL);
      if (!stmt->Step()) {
        throw Error("ERROR: not enough rows in table "+ctx.tbl);
      }
      idb = stmt->GetText(0, NULL);
      std::string b = stmt->GetText(1, NULL);
      if (a != b) {
        std::cout << ctx.tbl << "." << ctx.field << " (string):  " << a << " != " << b << "\n";
      }
      break;
   } default: {
      std::string a = stmt->GetText(1, NULL);
      if (!stmt->Step()) {
        throw Error("ERROR: not enough rows in table "+ctx.tbl);
      }
      idb = stmt->GetText(0, NULL);
      std::string b = stmt->GetText(1, NULL);
      if (a != b) {
        std::cout << ctx.tbl << "." << ctx.field << " (other):  " << a << " != " << b << "\n";
      }
      break;
   }
  }
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


