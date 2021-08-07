#include "fakegclib.h"

GReturn G_NO_ERROR = GReturn();

GReturn GOpen(std::string address, GCon* g){
    (void)address;
    *g = 12;
    return GReturn();
}


GReturn GCmd(GCon g,std::string x){
    (void)g;
    (void)x;
    return GReturn();
}

GReturn GMotionComplete(GCon g,std::string x){
    (void)g;
    (void)x;
    return GReturn();
}

GReturn GClose(GCon g){
    (void)g;
    return GReturn();
}
