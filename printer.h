#ifndef PRINTER_H
#define PRINTER_H
#include "gclib.h"
#include "gclibo.h"
#include "gclib_errors.h"
#include "gclib_record.h"

#define X_CNTS_PER_MM 1000
#define Y_CNTS_PER_MM 800
#define Z_CNTS_PER_MM 75745.7108f

#define X_STAGE_LEN_MM 150
#define Y_STAGE_LEN_MM 500
#define Z_STAGE_LEN_MM 15

int mm2cnts(double mm, char axis);

class Printer
{
public:
    Printer();

    char const *address = "192.168.42.100"; // IP address of motion controller
    GCon g{0}; // Handle for connection to Galil Motion Controller
};

#endif // PRINTER_H
