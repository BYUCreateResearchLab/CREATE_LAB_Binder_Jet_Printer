#ifndef HEATLAMP_H
#define HEATLAMP_H

#include <vector>
#include <QObject>
#include "printer.h"

struct TempData {
    double intensity;
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
        double kd{0.0}; //intensity per degree/passes
        void clear_history();
        std::string set_intensity(int intensity);
        double get_next_intensity();
        double max_intensity{15};
        double min_intensity{0};
        double starting_intensity{2};
        double default_intensity{1};
        std::string get_temp_history();
    
    private:
        double last_intensity;
        std::vector<TempData> temp_history;
};

#endif
