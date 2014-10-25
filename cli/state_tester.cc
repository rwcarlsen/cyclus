
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <algorithm>

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

int main(int argc, char* argv[]) {
  cyclus::AgentSpec spec; // = ...

  // load templated test input file and user specified prototype config
  std::stringstream ss;
  LoadStringstreamFromFile(ss, "state_test.xml");
  std::string s = ss.str();

  ss.str(""); // clear stream
  LoadStringstreamFromFile(ss, configfile);
  std::string config = ss.str();

  std::replace(s.begin(), s.end(), "{{path}}", spec.path());
  std::replace(s.begin(), s.end(), "{{lib}}", spec.lib());
  std::replace(s.begin(), s.end(), "{{name}}", spec.agent());
  std::replace(s.begin(), s.end(), "{{config}}", config);

  // load and initialize simulation
  XMLFileLoader l(&rec, fback, s);
  l.LoadSim();

  FullBackend* fback = new SqliteBack("test_db.sqlite");
  RecBackend::Deleter bdel;
  Recorder rec;  // Must be after backend deleter because ~Rec does flushing

  rec.RegisterBackend(fback);
  bdel.Add(fback);

  SimInit si;
  si.Init(&rec, fback); // creates first snapshot

  // initialize another simulation from previous si's time 0 snapshot
  SimInit si2;
  si2.Restart(QueryableBackend* b, rec.sim_id(), 0);
  si2.rec()->RegisterBackend(fback);
  si2.context()->Snapshot(); // schedule snapshot for start first timestep (0).
  si2.timer()->RunSim();

  // compare agent-state rows for the two sim id's for each time zero snapshot.


  rec.Flush();
  return 0;
}
