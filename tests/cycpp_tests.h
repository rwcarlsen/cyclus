#include <string>

// Spies are secret agents, get it?!

#pragma cyclus exec x = 5; import time
#pragma cyclus exec import math

namespace jolly_old_england {};

namespace mi6 {

using namespace std;

namespace joe = jolly_old_england;

typedef double caliber_t;
typedef caliber_t cbt;

class Spy {
 public:
  #pragma cyclus var dict(\
    default=7,\
    units="unitless",\
    userlevel=10,\
    tooltip="eg, 007",\
    help="The spy's agent number - TOP SECRET!",\
    )
  int 
num;

  //using std::string;
  using namespace std;

  #pragma cyclus var dict(\
    default="James Bond, 00" + str(num['default']),\
    units="unitless",\
    userlevel=x,\
    tooltip="eg, James Bond",\
    help="time {0}".format(time.time()),\
    )
  string name;
  //std::string name;
  //char * name;

  #pragma cyclus var {\
    "default": 0.22,\
    "units": "unitless",\
    "userlevel": 99,\
    "tooltip": "weapon caliber",\
    "help": "favored weapon size",\
    }
  cbt caliber;

  using std::pair;
  typedef std::pair deuce;
  #pragma cyclus var {\
    "default": ("Goldfinger", ("Oddjob", "Jaws")),\
    "units": "unitless",\
    "userlevel": 0,\
    "tooltip": "(name, name)",\
    "help": "a villian and a lackey",\
    }
  //std::pair<std::string, std::list<std::string> > villan_minions;
  //std::pair<std::string, list<std::string> > villan_minions;
  //pair<std::string, list<std::string> > villan_minions;
  deuce<std::string, list<std::string> > villan_minions;
private:
  double not_var;
};

class Friend {
 public:
  #pragma cyclus var {\
    "default": Spy.name['default'], \
    }
  std::string enemy;
};

}; // namespace mi6

typedef unsigned long ulong;
typedef int int_t, *intp_t, (&fp)(int, ulong), arr_t[10];
typedef struct {int a; int b;} S, *pS;

class Villan {
 public:
  #pragma cyclus var {\
    "default": mi6.Spy.name['default'],\
    }
  std::string enemy;

  #pragma cyclus initfrom
};
