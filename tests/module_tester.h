#ifndef CLI_MODEL_TESTS_H_
#define CLI_MODEL_TESTS_H_

#include <string>
#include <sstream>
#include <gtest/gtest.h>

#include "cyclus.h"
#include "dynamic_module.h"
#include "xml_parser.h"

class ModelTests : public ::testing::TestWithParam<std::string> {
  protected:
    cyclus::Timer ti;
    cyclus::EventManager em;
    cyclus::Context* ctx;
    cyclus::Model* m;
    cyclus::DynamicModule* dyn;

    virtual void SetUp(){
      ctx = new cyclus::Context(&ti, &em);
      dyn = new cyclus::DynamicModule(GetParam());
      m = dyn->ConstructInstance(ctx);
    };

    virtual void TearDown() {
      delete m;
      delete ctx;
      dyn->CloseLibrary();
      delete dyn;
    }
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
TEST_P(ModelTests, Schema) {
  std::stringstream schema;
  schema << "<element name=\"foo\">\n";
  schema << m->schema();
  schema << "</element>\n";

  cyclus::XMLParser p;
  EXPECT_NO_THROW(p.Init(schema));
}

#endif
