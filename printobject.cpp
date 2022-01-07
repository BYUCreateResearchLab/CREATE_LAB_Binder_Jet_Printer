#include "printobject.h"

/**************************************************************************
 *                        CLASS  Print Line                               *
 **************************************************************************/

// Static Defaults
double PrintLine::defaultDropletSpacing = 5;
double PrintLine::defaultVelocity = 5;
double PrintLine::defaultAcceleration = 100;

PrintLine::PrintLine()
{
    p1 = vec2d(0,0);
    p2 = vec2d(0,0);
    velocity = defaultVelocity;
    acceleration = defaultAcceleration;
    deceleration = defaultAcceleration;
    dropletSpacing = defaultDropletSpacing;
}

PrintLine::PrintLine(vec2d p1, vec2d p2) : p1(p1), p2(p2)
{
    velocity = defaultVelocity;
    acceleration = defaultAcceleration;
    deceleration = defaultAcceleration;
    dropletSpacing = defaultDropletSpacing;
}

PrintLine::PrintLine(vec2d p1, vec2d p2, double velocity, double acceleration, double dropletSpacing) : p1(p1), p2(p2), velocity(velocity), acceleration(acceleration), dropletSpacing(dropletSpacing)
{
    deceleration = acceleration; // Make equal by default
}

// Getters
double PrintLine::getJettingFrequency()
{
    return (velocity * 1000.0) / dropletSpacing; // CHECK THIS
}


// Setters
void PrintLine::setJettingFrequency(double freq)
{
    // adjust velocity so that same droplet spacing gives desired jetting frequency
    velocity = (dropletSpacing * freq) / 1000.0; // CHECK THIS AS WELL
}


double PrintLine::length()
{
    return p1.distTo(p2);
}

vec2d PrintLine::dist()
{
    return p2 - p1;
}

bool PrintLine::isPrintPath()
{
    return (dropletSpacing > 0) ? true : false;
}

bool PrintLine::isnotPrintPath()
{
    return (dropletSpacing <= 0) ? true : false;
}

double PrintLine::printTime()
{
    return 0; // Implement this later
}


/**************************************************************************
 *                        CLASS  Print Layer                              *
 **************************************************************************/

PrintLayer::PrintLayer()
{

}


/**************************************************************************
 *                        CLASS  Print Object                             *
 **************************************************************************/

PrintObject::PrintObject()
{

}
