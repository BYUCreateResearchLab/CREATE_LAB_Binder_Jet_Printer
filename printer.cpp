#include "printer.h"
#include "commandcodes.h"

#include <QDebug>

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
        return 0;
        break;
    }
}

int um2cnts(double um, char axis)
{
    return mm2cnts(um * 1000.0, axis);
}

Printer::Printer()
{

}

/*
ParserStatus Printer::parse_command(const std::string &commandType, const std::string &commandString)
{
    if(commandType == "GCmd")
    {
        qDebug() << QString::fromStdString(commandString);
    }
    else if(commandType == "GMotionComplete")
    {
        qDebug() << QString::fromStdString(commandString);
    }
    else if(commandType == "GSleep")
    {

    }
    else if(commandType == "JetDrive")
    {

    }
    else
    {
        return ParserStatus::CommandTypeNotFound;
    }
    return ParserStatus::NoError;
}
*/
