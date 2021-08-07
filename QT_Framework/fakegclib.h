#ifndef FAKEGCLIB_H
#define FAKEGCLIB_H
#include <string>


typedef std::string GCStringIn ;
typedef int GCon ;
typedef int GReturn;

extern GReturn G_NO_ERROR;

GReturn GClose(GCon g);
GReturn GCmd(GCon g, GCStringIn);
GReturn GMotionComplete(GCon g, GCStringIn);
GReturn GOpen(GCStringIn address, GCon* g);


#endif // FAKEGCLIB_H
