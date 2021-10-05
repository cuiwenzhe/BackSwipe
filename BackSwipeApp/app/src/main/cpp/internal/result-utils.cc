#include "result-utils.h"
#include "base/basictypes.h"
#include "base/unilib.h"
#include "base/hash.h"
#include <ext/hash_map>
using __gnu_cxx::hash_map;

namespace keyboard {
namespace decoder {
    struct str_hash{
        size_t operator()(const string& str) const
        {
            unsigned long __h = 0;
            for (size_t i = 0 ; i < str.size() ; i ++)
                __h = 5*__h + str[i];
            return size_t(__h);
        }
    };
    using namespace std;
    vector<DecoderResult> SuppressUppercaseResults(
            const std::vector<DecoderResult>& results,
            float uppercase_suppression_score_threshold) {
        vector<DecoderResult> filtered_results;
        hash_map<Utf8String, float, str_hash> lowercase_word_scores;
        for (const auto& result : results) {
            const Utf8String& word = result.word();
            const Utf8String lowercase_word = UniLib::ToLower(word);
            const bool is_lowercase = (word == lowercase_word);
            if (is_lowercase) {
                lowercase_word_scores[lowercase_word] = result.score();
            } else {
                const auto& lowercase_entry = lowercase_word_scores.find(lowercase_word);
                if (lowercase_entry != lowercase_word_scores.end()) {
                    // Suppress this uppercase variant if it scores too far below the
                    // lowercase variant of the same word (if one exists).
                    const float case_variant_score_threshold =
                            lowercase_entry->second + uppercase_suppression_score_threshold;
                    if (result.score() < case_variant_score_threshold) continue;
                }
            }
            filtered_results.push_back(result);
        }
        return filtered_results;
    }

}  // namespace decoder
}  // namespace keyboard
