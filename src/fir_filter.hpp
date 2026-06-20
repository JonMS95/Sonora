#ifndef FIR_FILTER_HPP
#define FIR_FILTER_HPP

#include <vector>
#include <cstddef>

class FIRFilter
{
private:
    std::vector<float> kernel_;

public:
    explicit FIRFilter(const float cutoff = 0.5, const std::size_t filter_size = 101);
    
    std::vector<float> applyFIR(const std::vector<float>& signal) const;
};

#endif
