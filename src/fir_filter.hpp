#ifndef FIR_FILTER_HPP
#define FIR_FILTER_HPP

#include <vector>
#include <iostream>

class FIRFilter
{
private:
    std::vector<float> kernel_;

public:
    FIRFilter(const float cutoff = 0.5, const int size = 101);
    std::vector<float> applyFIR(const std::vector<float>& signal) const;
};

#endif
