/****************************************************************************
**
** LinePrintData.cpp
** Definitions are found in LinePrintData.h
** This file is part of the binder jetting test platform UI
** Jacob Lawrence
**
** LinePrintData.h includes classes for storing user-input data for printing
** experimental lines using the binder jetting test platform.
**
** There are three classes for handling the data:
** -TableData
** -LineSet
** -LinePrintData
**
** These three classes are nested and LinePrintData is the only class meant to
** be used externally with the other classes acting as data structures for the
** LinePrintData class
**
** The LinePrintData class stores the data in a vector called 'data' which is
** a vector of LineSets. Each LineSet represents a row in the user input table
** A LineSet includes a member for each type of data needed to print a set of
** lines. The data is stored in the TableData class which stores the data,
** min and max acceptable values, and whether the type should be a float or int,
** although internally all values are stored as floats.
**
****************************************************************************/

#include "lineprintdata.h"

#include <cmath>

/**************************************************************************
 *                        CLASS  Table Data                               *
 **************************************************************************/

QString TableData::toQString(){
    if(std::isnan(value)){ // Return blank if data is NAN
        return QString::fromStdString("");
    }else{ // Return QString of value according to data type
    switch(dataType){
    case int_type:
        return QString::number(value);
        break;
    case float_type:
        return QString::number(value);
        break;
    default:
        return QString::fromStdString("Error in conversion to qstring");
    }
    }
}

bool TableData::tooSmall(float val)
{
    return (val < min) ? true : false;
}

bool TableData::tooLarge(float val)
{
    return (val > max) ? true : false;
}

errorType TableData::checkinRange(float val)
{
    if(tooSmall(val)){return errortooSmall;}
    else if(tooLarge(val)){return errortooLarge;}
    else {return errorNone;}

}
// Work on updating this so I can implement checking between related fields
// (droplet spacing, freq, and printing velocity)

errorType TableData::updateValue(float val)
{
    errorType returnerror = checkinRange(val);

    switch(returnerror){
    case errorNone:
        value = val;
        break;
    case errortooSmall:
        value = min;
        break;
    case errortooLarge:
        value = max;
        break;
    case errorCannotConvert: // This should never happen here
        value = NAN;
        break;
    default:
        break;
    }

    return returnerror;
}

errorType TableData::updateData(QString input){
    if(input != toQString()){ // If the QString text has changed
        bool ok=false; // indicator for errors of toInt function
        switch(dataType){
        case int_type: // if data is an int
        {
            int cellVal = input.toInt(&ok);
            if (!ok){ // if error
                value = NAN;
                return errorCannotConvert; // cannot convert error code
            }else{ // successful conversion to int

                return updateValue(cellVal); // update value and return error code
            }
        }
            break;

        case float_type: // if data is a float
        {
            float cellVal = input.toFloat(&ok);
            if (!ok){ // if error
                value = NAN;
                return errorCannotConvert; // cannot convert error code
            }else{ // successful conversion to float
                return updateValue(cellVal);
            }
        }
            break;
        default:
            return errorCannotConvert; // Should be an error...
        }
    }
    return errorNone;
}

/**************************************************************************
 *                        CLASS  Line Set                                 *
 **************************************************************************/

LineSet::LineSet()
{

}




/**************************************************************************
 *                        CLASS  Line Print Data                          *
 **************************************************************************/

LinePrintData::LinePrintData()
{

}

void LinePrintData::addRows(int num_sets=1)
{
    for(int i=0; i<num_sets; i++){
       data.push_back(LineSet());
    }
}

void LinePrintData::removeRows(int num_sets=1)
{
    data.erase(data.end() - num_sets, data.end());
}

std::vector<QLineF> LinePrintData::qLines()
{
    float xpos = startX;
    float ypos = startY;
    float yheight = 100; // Height of printer
    // Note: The coordinate system of vector graphics have the origin in the top left (Quad IV)
    //       but the printer will have the origin in the bottom left corner (Quad I)
    //       This will require that the y axis coordinate be subtracted by the height
    //       to visualize in a vector graphic how coordinates will be represented on the printer.

    std::vector<QLineF> returnVec;
    for(size_t s=0; s < data.size(); s++){ // for each set of lines
        // Confirm all of the data fields for the set are not empty
        bool completeSet = true;
        for(int c=0; c < data[s].size; c++){ // for each value in a set
            if(std::isnan(data[s][c].value)){
                completeSet = false; // mark the set as not complete if there is a NaN value
            }
        }

        if(completeSet){ // only if there are no NaNs
            for(int i=0; i < (int)data[s].numLines.value; i++){// for each line in the set
                // Add a QLine object for the line
                returnVec.push_back(QLineF(xpos, yheight - ypos, xpos + data[s].lineLength.value, yheight - ypos));
                ypos += data[s].lineSpacing.value; // Move the Y position by the lineSpacing amount
            }
        }
        ypos = startY; // Reset the Y Spacing
        xpos += setSpacing + data[s].lineLength.value; // Move the X Spacing
    }
    return returnVec;
}
