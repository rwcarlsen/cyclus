#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>

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

static std::string usage = "Usage:  snaptest [opts] <agent-spec> <config-file>";

static bool fail = false;

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
  AgentSpec spec;
  std::string simid1;
  std::string simid2;
  int agentid;
  int protoid;
  std::string tbl;
  std::string field;
  int type;
};

void TestSnaps(AgentSpec spec);
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

  FileDeleter fd("snaptest.sqlite");
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

  TestSnaps(ai.spec);

  if (fail) {
    std::cout << "\nLikely errors in one or more of the following member functions:\n"
              << "    - InitFrom(QueryableBackend*)\n"
              << "    - Snapshot\n"
              << "    - InitFrom(Agent*)\n"
              << "    - InitInv\n"
              << "    - SnapshotInv\n";
    return 1;
  }

  return 0;
}

void TestSnaps(AgentSpec spec) {
  SqliteDb db("snaptest.sqlite", true);
  db.open();
  Ctx ctx;
  ctx.spec = spec;
  ctx.db = &db;

  // get simulation ids and agent id;
  SqlStatement::Ptr stmt;
  stmt = db.Prepare("SELECT hex(SimId),AgentId FROM AgentEntry;");
  while (stmt->Step()) {
    ctx.simid1 = stmt->GetText(0, NULL);
    ctx.agentid = stmt->GetInt(1);
    break;
  }
  stmt = db.Prepare("SELECT hex(SimId) FROM Info WHERE hex(SimId) != ?;");
  stmt->BindText(1, ctx.simid1.c_str());
  while (stmt->Step()) {
    ctx.simid2 = stmt->GetText(0, NULL);
    break;
  }

  stmt = db.Prepare("SELECT AgentId FROM Prototypes WHERE Prototype = ? AND hex(SimId) = ?");
  stmt->BindText(1, "snaptest_prototype");
  stmt->BindText(2, ctx.simid1.c_str());
  while (stmt->Step()) {
    ctx.protoid = stmt->GetInt(0);
    break;
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

template<class T>
std::string Compare(T a, T b, T c) {
  std::stringstream ss;
  if (a != b || b != c) {
    ss << a;
    if (a != b) {
      ss << " != " << b;
    } else {
      ss << " == " << b;
    }
    if (b != c) {
      ss << " != " << c;
    } else {
      ss << " == " << c;
    }
  } 
  return ss.str();
}

void TestField(Ctx ctx) {
  SqlStatement::Ptr stmt1; // prototype snapshot
  SqlStatement::Ptr stmt2; // agent snapshot 1
  SqlStatement::Ptr stmt3; // agent snapshot 2
  if (ctx.type > VL_STRING) {
    stmt1 = ctx.db->Prepare("SELECT hex("+ctx.field+") FROM "+ctx.tbl+" WHERE SimTime=0 AND AgentId=? AND hex(SimId)=?");
    stmt2 = ctx.db->Prepare("SELECT hex("+ctx.field+") FROM "+ctx.tbl+" WHERE SimTime=0 AND AgentId=? AND hex(SimId)=?");
    stmt3 = ctx.db->Prepare("SELECT hex("+ctx.field+") FROM "+ctx.tbl+" WHERE SimTime=0 AND AgentId=? AND hex(SimId)=?");
  } else {
    stmt1 = ctx.db->Prepare("SELECT "+ctx.field+" FROM "+ctx.tbl+" WHERE SimTime=0 AND AgentId=? AND hex(SimId)=?");
    stmt2 = ctx.db->Prepare("SELECT "+ctx.field+" FROM "+ctx.tbl+" WHERE SimTime=0 AND AgentId=? AND hex(SimId)=?");
    stmt3 = ctx.db->Prepare("SELECT "+ctx.field+" FROM "+ctx.tbl+" WHERE SimTime=0 AND AgentId=? AND hex(SimId)=?");
  }
  stmt1->BindInt(1, ctx.protoid);
  stmt1->BindText(2, ctx.simid1.c_str());
  stmt2->BindInt(1, ctx.agentid);
  stmt2->BindText(2, ctx.simid1.c_str());
  stmt3->BindInt(1, ctx.agentid);
  stmt3->BindText(2, ctx.simid2.c_str());

  std::string tblname = ctx.tbl;
  tblname.erase(0, std::string("AgentState").size());
  tblname.erase(0, ctx.spec.Sanitize().size());

  int i = 0;
  while (stmt1->Step()) {
    i++;
    std::stringstream ss;
    ss << tblname << "." << ctx.field << " row " << i << ":  ";
    std::string prefix = ss.str();

    if (!stmt2->Step()) {
      throw Error("not enough rows in second snapshot of "+ctx.tbl);
    }
    if (!stmt3->Step()) {
      throw Error("not enough rows in prototype snapshot of "+ctx.tbl);
    }

    std::string msg;
    switch (ctx.type) {
      case BOOL: {
        bool a = stmt1->GetInt(0);
        bool b = stmt2->GetInt(0);
        bool c = stmt3->GetInt(0);
        msg = Compare(a, b, c);
        break;
      } case INT: {
        int a = stmt1->GetInt(0);
        int b = stmt2->GetInt(0);
        int c = stmt3->GetInt(0);
        msg = Compare(a, b, c);
        break;
     } case FLOAT:
       case DOUBLE: {
        double a = stmt1->GetDouble(0);
        double b = stmt2->GetDouble(0);
        double c = stmt3->GetDouble(0);
        msg = Compare(a, b, c);
        break;
     } case STRING:
       case VL_STRING: {
        std::string a = stmt1->GetText(0, NULL);
        std::string b = stmt2->GetText(0, NULL);
        std::string c = stmt3->GetText(0, NULL);
        msg = Compare(a, b, c);
        break;
     } default: {
        std::string a = stmt1->GetText(0, NULL);
        std::string b = stmt2->GetText(0, NULL);
        std::string c = stmt3->GetText(0, NULL);
        msg = Compare(a, b, c);
     }
    }
    if (msg != "") {
      fail = true;
      std::cout << prefix << msg << "\n";
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
  try {
    LoadStringstreamFromFile(ss, ai->vm["config"].as<std::string>());
  } catch (Error err) {
    std::cerr << err.what() << "\n";
    return 1;
  }
  ai->config = ss.str();

  return -1;
}


