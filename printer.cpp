#include "printer.h"

int mm2cnts(double mm, char axis)
{
    switch(axis)
    {
        case 'X':
            return mm * X_CNTS_PER_MM;
            break;
        case 'Y':
            return mm * Y_CNTS_PER_MM;
            break;
        case 'Z':
            return mm * Z_CNTS_PER_MM;
            break;
        default:
            break;
    }
}

Printer::Printer()
{

}
