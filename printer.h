#ifndef PRINTER_H
#define PRINTER_H
#include "gclib.h"
#include "gclibo.h"
#include "gclib_errors.h"
#include "gclib_record.h"

class Printer
{
public:
    Printer();

    char const *address = "192.168.42.100";
    GCon g{0}; // Handle for connection to Galil Motion Controller
};

#endif // PRINTER_H
