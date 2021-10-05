#include "math-utils.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <vector>

#include "base/macros.h"

using std::vector;

namespace keyboard {
namespace decoder {

    vector<double> MathUtils::mean_xs_(0);
    vector<double> MathUtils::mean_ys_(0);
    vector<double> MathUtils::sd_xs_(0);
    vector<double> MathUtils::sd_ys_(0);
    vector<double> MathUtils::rhos_(0);

}  // namespace decoder
}  // namespace keyboard