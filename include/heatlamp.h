#ifndef HEATLAMP_H
#define HEATLAMP_H

#include <vector>
#include <QObject>
#include "printer.h"

struct TempData {
    int intensity;
    double temp;
};

class HeatLamp : public QObject
{
    Q_OBJECT

    public:
        explicit HeatLamp(double target_temp, QObject *parent = nullptr);
        ~HeatLamp();
        void set_last_temp(double temperature);
        double target_temp;
        double kp{0.05}; // intensity per degree
        double ki{0.02}; //intensity per degree*passes
        void clear_history();
        std::string set_intensity(int intensity);
        int get_next_intensity();
        int max_intensity{15};
        int min_intensity{0};
        int starting_intensity{2};
    
    private:
        int last_intensity;
        std::vector<TempData> temp_history;
};

#endif
