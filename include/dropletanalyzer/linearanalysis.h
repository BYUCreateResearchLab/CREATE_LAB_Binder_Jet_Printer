#ifndef LINEARANALYSIS_H
#define LINEARANALYSIS_H

#include <vector>

namespace LinearAnalysis {

struct FitLine
{
    double slope {0};
    double intercept {0};
    bool fitFailed {false};
};

inline FitLine find_fit_line(const std::vector<double>& x,
                             const std::vector<double>& y)
{
    FitLine result;
    if (x.size() != y.size())
    {
        result.fitFailed = true;
        return result;
    }
    double n = x.size();
    double sumX {0};
    double sumY {0};
    double sumXY {0};
    double sumXSquared {0};
    for (size_t i{0}; i < x.size(); i++)
    {
        const double& xVal = x[i];
        const double& yVal = y[i];
        sumX += xVal;
        sumY += yVal;
        sumXY += xVal * yVal;
        sumXSquared += xVal * xVal;
    }
    const double denominator = ((n * sumXSquared) - (sumX * sumX));
    result.intercept = ((sumY * sumXSquared) - (sumX * sumXY)) / denominator;
    result.slope = ((n * sumXY) - (sumX * sumY)) / denominator;
    result.fitFailed = false;
    return result;
}

inline std::vector<double> get_fit_vals(const FitLine& fit,
                                        const std::vector<double>& x)
{
    std::vector<double> result;
    result.reserve(x.size());
    for (const auto& val : x)
        result.push_back(fit.intercept + (fit.slope * val));
    return result;
}

inline std::vector<double> calculate_residuals(const FitLine& fit,
                                               const std::vector<double>& x,
                                               const std::vector<double>& y)
{
    std::vector<double> result;
    result.reserve(y.size());
    auto fitY = get_fit_vals(fit, x);
    for (size_t i{0}; i < y.size(); i++)
    {
        result.push_back(y[i] - fitY[i]);
    }
    return result;
}

}

#endif // LINEARANALYSIS_H
