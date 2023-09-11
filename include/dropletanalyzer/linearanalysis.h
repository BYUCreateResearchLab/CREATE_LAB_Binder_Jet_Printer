#ifndef LINEARANALYSIS_H
#define LINEARANALYSIS_H

#include <vector>
#include <random>

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

// Function to find the best fit line using RANSAC
inline FitLine find_fit_line_ransac(const std::vector<double>& x,
                                    const std::vector<double>& y,
                                    int numIterations,
                                    double distanceThreshold)
{
    FitLine bestFitLine;
    int numPoints = x.size();

    if (numPoints != y.size() || numPoints < 2)
    {
        bestFitLine.fitFailed = true;
        return bestFitLine;
    }

    std::random_device rd;
    std::mt19937 gen(rd());

    int maxInliers = 0;

    for (int iter = 0; iter < numIterations; ++iter)
    {
        // Randomly select two points
        std::uniform_int_distribution<int> dist(0, numPoints - 1);
        int index1 = dist(gen);
        int index2 = dist(gen);

        // Avoid selecting the same point twice
        while (index2 == index1)
            index2 = dist(gen);

        double x1 = x[index1];
        double y1 = y[index1];
        double x2 = x[index2];
        double y2 = y[index2];

        // Compute the candidate line (slope and intercept)
        double candidateSlope = (y2 - y1) / (x2 - x1);
        double candidateIntercept = y1 - candidateSlope * x1;

        // Count inliers (points close to the candidate line)
        int inliers = 0;
        for (int i = 0; i < numPoints; ++i)
        {
            double distance = std::abs(y[i] - (candidateSlope * x[i] + candidateIntercept));
            if (distance < distanceThreshold)
                inliers++;
        }

        // Update the best fit line if this candidate has more inliers
        if (inliers > maxInliers)
        {
            maxInliers = inliers;
            bestFitLine.slope = candidateSlope;
            bestFitLine.intercept = candidateIntercept;
            bestFitLine.fitFailed = false;

            // Break early if we have enough inliers
            if (inliers > numPoints / 2)
                break;
        }
    }

    // If the number of inliers is too low, consider it a failed fit
    if (maxInliers < numPoints / 2) {
        bestFitLine.fitFailed = true;
    }

    return bestFitLine;
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
