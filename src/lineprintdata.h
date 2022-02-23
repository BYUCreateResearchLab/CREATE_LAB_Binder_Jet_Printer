/****************************************************************************
**
** lineprintdata.h
** implementations are found in lineprintdata.cpp
** This file is part of the binder jetting test platform UI
** Jacob Lawrence
**
** lineprintdata.h includes classes for storing user-input data for printing
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

#ifndef LINEPRINTDATA_H
#define LINEPRINTDATA_H

#include <string>
#include <vector>
#include <QString>
#include <QLineF> // used in LinePrintData method

enum type {int_type, float_type}; // Type specifier for TableData class.
// Add types here for additional functionality down the road
enum errorType {errorNone, errortooSmall, errortooLarge, errorCannotConvert};


/**************************************************************************
 *                        CLASS  Table Data                               *
 **************************************************************************/


class TableData{
public:
    // Empty constructor
    TableData();

    // Detailed constructor
    TableData(std::string typeName, type dataType, float value=0, float min=0, float max=0, bool state = false):
        typeName(typeName), dataType(dataType), value(value), min(min), max(max), state(state){}

    // Converts the 'value' member variable to a QString for displaying in the UI table
    QString toQString();

    // Returns true if the input value is out of range
    bool tooSmall(float val);
    bool tooLarge(float val);

    // Checks whether the input value is within the range of the min & max
    errorType checkinRange(float val);

    // Takes a QString input from the UI table and checks if the value can be converted to an int/float
    // returns error code from updating data from Qstring input:
    // 0=no error, 1=too small, 2=too large 3=cannot convert to desired type
    errorType updateData(QString input);


    std::string typeName;
    type dataType;
    float value, min, max;
    bool state = false; // Not used yet
private:
    errorType updateValue(float val); // Internal updating of value. Returns code based off of value limits
};


/**************************************************************************
 *                        CLASS  Line Set                                 *
 **************************************************************************/

class LineSet
{
public:
    LineSet();

    TableData numLines = TableData("Number of Lines", type::int_type, 1, 1, 5000);
    TableData lineSpacing = TableData("Line Spacing\n(mm)", type::float_type, 10, 0.005f, 50);
    TableData lineLength = TableData("Line Length\n(mm)", type::float_type, 15, 0.01f, 100);
    TableData dropletSpacing = TableData("Droplet Spacing\n(µm)", type::int_type, 5, 1, 50);
    TableData jettingFreq = TableData("Jetting Frequency\n(Hz)", type::int_type, 1000, 100, 10000);
    TableData printVelocity = TableData("Printing Velocity\n(mm/s)", type::float_type, 5, 0.1f, 10000);
    TableData printAcceleration = TableData("Print Acceleration\n(mm/s2)", type::float_type, 800, 100.0, 1500.0);

    int size = 7; // Number of columns in dataset



    TableData &operator[](int i){
        switch(i){
        case 0:
            return this->numLines;
            break;
        case 1:
            return this->lineSpacing;
            break;
        case 2:
            return this->lineLength;
            break;
        case 3:
            return this->dropletSpacing;
            break;
        case 4:
            return this->jettingFreq;
            break;
        case 5:
            return this->printVelocity;
            break;
        case 6:
            return this->printAcceleration;
        default:
            return this->numLines; //ERROR
            break;
        }
    }
private:
};


/**************************************************************************
 *                        CLASS  Line Printing                            *
 **************************************************************************/

class LinePrintData
{
public:
    LinePrintData();
    std::vector<LineSet> data = {};
    float startX;
    float startY;
    float setSpacing;

    void addRows(int num_sets);
    void removeRows(int num_sets);

    int get_column_index_for(const TableData &column);

    std::vector<QLineF> qLines();
    std::vector<QLineF> qAccelerationLines();

    int numRows(){return (int)(data.size());}
};

#endif // LINEPRINTDATA_H
