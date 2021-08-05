/****************************************************************************
**
** printobject.h
** implementations are found in printobject.cpp
** This file is part of the binder jetting test platform UI
** Jacob Lawrence
**
** printobject.h includes classes for storing data for move paths for printing
** experimental lines using the binder jetting test platform.
**
** There are three classes for handling the data:
** -PrintLine
** -PrintLayer
** -PrintObject
**
** The PrintLine class contains the data for a single move path including:
** Starting coordinate
** Ending coordinate
** Print speed
** Jetting frequency
**
** The PrintLayer class contains a vector of print lines
**
** The PrintObject class contains a vector of layers
**
****************************************************************************/

#ifndef PRINTOBJECT_H
#define PRINTOBJECT_H

#include "vec2.h"

/**************************************************************************
 *                        CLASS  Print Line                               *
 **************************************************************************/

class PrintLine
{
public:
    PrintLine();
    PrintLine(vec2d p1, vec2d p2);
    PrintLine(vec2d p1, vec2d p2, double velocity, double acceleration, double dropletSpacing);

    // Methods

    // Getters
    vec2d getP1(){return p1;};
    vec2d getP2(){return p2;};
    double getVelocity(){return velocity;};
    double getAcceleration(){return acceleration;};
    double getDeceleration(){return deceleration;};
    double getDropletSpacing(){return dropletSpacing;};
    double getJettingFrequency();

    // Setters
    void setP1(vec2d arg1){p1 = arg1;};
    void setP2(vec2d arg1){p2 = arg1;};
    void setVelocity(double arg1){velocity = arg1;};
    void setAcceleration(double arg1){acceleration = arg1;};
    void setDeceleration(double arg1){deceleration = arg1;};
    void setDropletSpacing(double arg1){dropletSpacing = arg1;};
    void setJettingFrequency(double freq);

    double length();
    vec2d dist();
    bool isPrintPath();
    bool isnotPrintPath();
    double printTime();

    // Defaults
    static double defaultVelocity;
    static double defaultAcceleration;
    static double defaultDropletSpacing;


private:
    // Private Members
    vec2d p1; // mm
    vec2d p2; // mm
    double velocity; // mm/s
    double acceleration; // mm/s^2
    double deceleration; // mm/s^2
    double dropletSpacing; // microns
};

/**************************************************************************
 *                        CLASS  Print Layer                              *
 **************************************************************************/

class PrintLayer
{
public:
    PrintLayer();
};

/**************************************************************************
 *                        CLASS  Print Object                             *
 **************************************************************************/

class PrintObject
{
public:
    PrintObject();
};

#endif // PRINTOBJECT_H
