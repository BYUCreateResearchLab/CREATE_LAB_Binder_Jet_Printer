#include "fakegclib.h"

GReturn G_NO_ERROR = GReturn();

GReturn GOpen(std::string address, GCon* g){
    return GReturn();
}


GReturn GCmd(GCon g,std::string x){
    return GReturn();
}

GReturn GMotionComplete(GCon g,std::string x){
    return GReturn();
}

GReturn GClose(GCon g){
    return GReturn();
}
