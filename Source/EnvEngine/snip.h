// constants
enum SNIP_INPUT_TYPE {
   I_CONSTANT = 1
   I_CONSTWSTOP = 2,
   I_SINESOIDAL = 3,
   I_RANDOM = 4,
   I_TRACKOUTPUT = 5
   };


const int SEB_TRANSEFF = 0;
const int SEB_INFLUENCE = 1;
const int SNB_DEGREE = 0;
const int SNB_REACTIVITY = 1;

enum SNIP_STATE {
   STATE_INACTIVE = 0,
   STATE_ACTIVATING = 1,  // edges onl
   STATE_ACTIVE = 2,
   STATE_DELETED = 3
   };

enum NUP_INFMODEL_TYPE {
   IM_SENDER_RECEIVER = 0,
   IM_SIGNAL_RECEIVER = 1,
   IM_SIGNAL_SENDER_RECEIVER = 2
   };

const ET_INPUT = 0;
const ET_NETWORK = 1;

// node types
enum SNIP_NODETYPE {
   NT_INPUT_SIGNAL = 0,
   NT_NETWORK_ACTOR = 1,
   NT_LANDSCAPE_ACTOR = 2
   };

// cytoscape selectors
// edge type selectors
const INPUT_EDGE = '[type=0]';
const NETWORK_EDGE = '[type=1]';

// node selectors
const INPUT_SIGNAL = '[type=0]';
const NETWORK_ACTOR = '[type=1]';
const LANDSCAPE_ACTOR = '[type=2]';
const ANY_ACTOR = '[type=1],[type=2]';

// node, edge "state" selectors
const INACTIVE = '[state=0]';
const ACTIVATING = '[state=1]';
const ACTIVE = '[state=2]';


// Node Status
// On LOAD    | Input  | Network  | LA       |
// state      | Active | Inactive | Inactive |
// reactivity |  1.0   | 0.0      | 0.0      |

// During RUN | Input  | Network  | LA       |
// state      | Active | Both     | Both     |
// reactivity |  1.0   | calculated          |

const DEBUG = 0;

