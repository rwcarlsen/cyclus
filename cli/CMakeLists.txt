##############################################################################################
##################################### begin cyclus app #######################################
##############################################################################################

INCLUDE_DIRECTORIES(${CYCLUS_CORE_INCLUDE_DIRS})

# Build the cyclus executable from the CYCLUS_SOURCE source files
SET( CYCLUS_SOURCE ${CYCLUS_SOURCE}
  cyclus.cc
  )

ADD_EXECUTABLE( cyclus ${CYCLUS_SOURCE} )

TARGET_LINK_LIBRARIES( cyclus dl ${LIBS} cycluscore )

INSTALL(
  TARGETS cyclus
  RUNTIME DESTINATION bin
  COMPONENT cyclus
  )

##############################################################################################
###################################### module tester #########################################
##############################################################################################

# Build Module inquirer
ADD_EXECUTABLE( moduletest moduletest.cc)
TARGET_LINK_LIBRARIES( moduletest dl ${LIBS} cycluscore)
INSTALL(TARGETS moduletest RUNTIME DESTINATION bin COMPONENT testing)

