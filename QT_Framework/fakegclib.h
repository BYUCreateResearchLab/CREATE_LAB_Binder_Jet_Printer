#ifndef FAKEGCLIB_H
#define FAKEGCLIB_H
#include <string>


class GCon
{
public:
    GCon(){}
    GCon(int i){}

    bool operator ==(int v2){
        return true;
    }

    operator int() const{
        return 3;
    }
};

class GCStringIn
{
    GCStringIn(){}

};

class GReturn{
public:
    GReturn(){}
    GReturn(int val): val(val){}

    int val;

    bool operator !=(const GReturn &val2){
        return true;
    }

};

GReturn GClose(GCon g);

GReturn GCmd(GCon g,std::string x);
GReturn GMotionComplete(GCon g,std::string x);


GReturn GOpen(std::string address, GCon* g);

extern GReturn G_NO_ERROR;

#endif // FAKEGCLIB_H
