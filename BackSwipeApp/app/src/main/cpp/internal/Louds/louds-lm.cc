#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <limits>
#include <utility>

#include "../languageModel/encodingutils.h"
#include "../base/scoped-file-descriptor.h"
#include "../languageModel/case.h"
#include "louds-lm.h"
#include "louds-trie.h"

#include <android/log.h>

#define LOG_TAG "louds_lm"

#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)

namespace keyboard {
namespace lm {
namespace louds {

    const uint32 LoudsLm::kMagicNumber;

    // The number of top unigram next-word predictions to pre-compute.
    const int kMaxUnigramPredictions = 10;

    // The special backoff weight for unigram next-word predictions. This should be
    // negative enough to ensure that they are always ranked at the bottom.
    const float kUnigramPredictionBackoff = -100.0f;

    LoudsLm::~LoudsLm() {}

    NgramLoudsTrie::Key LoudsLm::TermsToTermIds(
            const std::vector<string>& terms) const {
        NgramLoudsTrie::Key term_ids;
        const int size = terms.size();
        for (int i = 0; i < size; ++i) {
            TermId16 term_id = TermToTermId(terms[i]);
            term_ids.push_back(term_id);
        }
        return term_ids;
    }

    bool LoudsLm::Build(const std::vector<Ngram>& ngrams) {
        std::vector<std::pair<string, LogProbFloat>> regular_unigrams;

        for (int i = 0; i < ngrams.size(); ++i) {
            if (ngrams[i].terms.size() == 1) {
                const string& term = ngrams[i].terms[0];
                if (!IsReservedTerm(term)) {
                    regular_unigrams.push_back({term, ngrams[i].logp});
                }
            }
        }

        // Build the lexicon.
        lexicon_ = LoudsLexicon::CreateFromUnigramsOrNull(
                regular_unigrams, params_.logp_quantizer_range,
                params_.max_num_term_ids, params_.enable_prefix_unigrams);

        // Construct the key to values map.
        NgramLoudsTrie::KeyValueMap keys_to_values;

        // Construct the key to backoff weights map.
        NgramLoudsTrie::KeyValueMap keys_to_backoffs;

        // Add the default values for the special terms (e.g., <S>, </S>, <UNK>).
        // These will be overridden by the ngrams if available.
        for (TermId16 id = 0; id < kFirstUnreservedId; ++id) {
            keys_to_values[{id}] =
                    quantizer_->Encode(-std::numeric_limits<float>::infinity());
        }

        max_n_ = 1;
        for (int i = 0; i < ngrams.size(); ++i) {
            if (ngrams[i].terms.size() == 1 && ngrams[i].terms[0] == kUnk) {
                keys_to_values[{kUnkId}] = quantizer_->Encode(-ngrams[i].logp);
                continue;
            }
            NgramLoudsTrie::Key key = TermsToTermIds(ngrams[i].terms);
            bool has_unk = false;
            for (TermId16 term_id : key) {
                if (term_id == kUnkId) {
                    has_unk = true;
                }
            }
            if (!has_unk) {
                // Note: Exclude n-grams that contain the <UNK> term.
                // this means that the LM will not be properly normalized.
                keys_to_values[key] = quantizer_->Encode(-ngrams[i].logp);
                if (params_.has_backoff_weights) {
                    keys_to_backoffs[key] = quantizer_->Encode(-ngrams[i].backoff);
                }
                if (key.size() > max_n_) {
                    max_n_ = key.size();
                }
            }
        }

        // Build the n-gram model.
        ngram_trie_ = NgramLoudsTrie::CreateFromKeyValueMapOrNull(
                keys_to_values, false /* has_explicit_terminals */);

        // Populate the backoff weights if needed.
        if (ngram_trie_ != nullptr && params_.has_backoff_weights) {
            std::vector<std::pair<LoudsNodeId, QuantizedLogProb>> terminals_to_backoffs;
            for (auto& entry : keys_to_backoffs) {
                const LoudsTerminalId terminal_id =
                        ngram_trie_->KeyToTerminalId(entry.first);
                if (terminal_id >= 0) {
                    terminals_to_backoffs.push_back(
                            std::pair<LoudsNodeId, QuantizedLogProb>(terminal_id,
                                                                     entry.second));
                }
            }
            std::sort(terminals_to_backoffs.begin(), terminals_to_backoffs.end());
            for (const auto& entry : terminals_to_backoffs) {
                if (entry.second != 0) {
                    // Only store non-zero (quantized) backoff weights.
                    while (has_backoff_weights_.size() < entry.first) {
                        has_backoff_weights_.push_back(false);
                    }
                    has_backoff_weights_.push_back(true);
                    backoff_weights_.push_back(entry.second);
                }
            }
            LOG(INFO) << "Populated backoff weights: " << backoff_weights_.size() << "/"
                      << has_backoff_weights_.size();
            has_backoff_weights_.build();
        }

        if (ngram_trie_ != nullptr && params_.include_unigram_predictions) {
            PopulateUnigramPredictions();
        }

        return (ngram_trie_ != nullptr);
    }

    bool LoudsLm::LookupConditionalLogProb(
            const std::vector<TermId16>& preceding_term_ids,
            const std::vector<StringPiece>& terms, LogProbFloat* value) const {
        // TODO(ouyang): Handle n-grams that include <UNK> terms rather than always
        // backing off. This would require the following:
        // 1. Move the probability mass from the terms-with-no-term-id to the <UNK>
        //    term. For example, if 'z' is a term-with-no-term-id:
        //      P(<UNK2>) = P(<UNK>) + P(z)
        //      P(a | <UNK2>) = P(a | (z or <UNK>))
        // 2. For terms-with-no-term-id, incorporate the conditional probability for
        //    <UNK> given the history.
        //      P(term | history) = P(term | <UNK>) * P(<UNK> | history)
        //                        = P(term) / P(<UNK>) * P(<UNK> | history)
        NgramLoudsTrie::Key term_ids =
                BackoffToInVocabTermIds(preceding_term_ids, terms, max_n_, true);
        if (term_ids.empty()) {
            *value = -quantizer_->Decode(LookupLogProbForTermId(kUnkId));
//            __android_log_print(ANDROID_LOG_INFO, "##### LoudsLm::LookupConditionalLogProb", "logp = %f", *value);
            return false;
        }
        const int term_count = terms.size() + preceding_term_ids.size();
        float backoff_cost = 0.0f;
        if (!params_.has_backoff_weights) {
            // Apply stupid backoff weight to skipped terms.
            const int backoff_count = std::min(max_n_, term_count) - term_ids.size();
            backoff_cost = backoff_count * stupid_backoff_factor();
        }
        QuantizedLogProb quantized_value;
        while (term_ids.size() > 1) {
            const bool found = ngram_trie_->KeyToValue(term_ids, &quantized_value);
            if (found) {
                *value = -quantizer_->Decode(quantized_value) + backoff_cost;
//                __android_log_print(ANDROID_LOG_INFO, "##### LoudsLm::LookupConditionalLogProb", "backoff_cost = %f", backoff_cost);
//                __android_log_print(ANDROID_LOG_INFO, "##### LoudsLm::LookupConditionalLogProb", "logp = %f", *value);
                return true;
            }
            const std::vector<TermId16> backoff_terms(term_ids.begin(),
                                                      term_ids.end() - 1);
            backoff_cost += GetBackoffCost(backoff_terms);
            term_ids.erase(term_ids.begin());
        }
        if (backoff_cost < 0) {
            const string last_term =
                    terms.empty() ? TermIdToTerm(term_ids[0]) : terms.back().ToString();
            const bool is_uppercase = (last_term != UniLib::ToLower(last_term));
            if (is_uppercase) {
                // Add extra weight for backing off to an uppercase unigrams.
                backoff_cost += params_.uppercase_unigram_extra_backoff_weight;
            }
        }

        if (term_ids[0] == kUnkId) {
            // There is no term_id for the last term in the n-gram LM, check whether it
            // is in the lexicon.
            const LoudsNodeId lexicon_node_id =
                    terms.size() > 0 ? lexicon_->KeyToNodeId(terms.back())
                                     : NgramLoudsTrie::kInvalidId;
            if (lexicon_node_id != NgramLoudsTrie::kInvalidId &&
                lexicon_->TermLogProbForNodeId(lexicon_node_id, value)) {
                *value += backoff_cost;
                return true;
            }
            *value = -quantizer_->Decode(LookupLogProbForTermId(kUnkId));
            return false;
        } else {
            *value =
                    -quantizer_->Decode(LookupLogProbForTermId(term_ids[0])) + backoff_cost;
        }
        return true;
    }

    void LoudsLm::PredictNextWords(const NgramLoudsTrie::Key& preceding_term_ids,
                                   const std::vector<StringPiece>& terms,
                                   const int max_results,
                                   std::map<string, LogProbFloat>* results) const {
        NgramLoudsTrie::Key term_ids =
                BackoffToInVocabTermIds(preceding_term_ids, terms, max_n_ - 1, false);
        std::set<TermId16> predicted_term_ids;
        if (!term_ids.empty()) {
            const int term_count = preceding_term_ids.size() + terms.size();
            float backoff_cost = 0.0f;
            if (!params_.has_backoff_weights) {
                const int backoff_count =
                        std::min(max_n_ - 1, term_count) - term_ids.size();
                backoff_cost = backoff_count * stupid_backoff_factor();
            }
            PredictionBeam top_predictions(max_results);
            while (!term_ids.empty()) {
                LookupNextWords(term_ids, max_results, backoff_cost, &top_predictions);
                backoff_cost += GetBackoffCost(term_ids);
                term_ids.erase(term_ids.begin());
            }
            for (const auto& prediction : top_predictions.Take()) {
                predicted_term_ids.insert(prediction.first);
                const string term = TermIdToTerm(prediction.first);
//                __android_log_print(ANDROID_LOG_INFO, "##### LoudsLm::PredictNextWords", "term = %s", term.c_str());
                if (IsReservedTerm(term)) {
                    // Never predict reserved terms like '<UNK>'.
                    continue;
                }
                const auto& entry = results->find(term);
                if (entry == results->end() || entry->second < prediction.second) {
                    (*results)[term] = prediction.second;
//                    __android_log_print(ANDROID_LOG_INFO, "##### LoudsLm::PredictNextWords", "prediction.second = %f", prediction.second);
                }
            }
        }
        if (params_.include_unigram_predictions) {
            for (const auto& prediction : top_unigrams_predictions_) {
                if (results->size() >= max_results) {
                    break;
                }
                if (predicted_term_ids.find(prediction.first) ==
                    predicted_term_ids.end()) {
                    const string term = TermIdToTerm(prediction.first);
//                    __android_log_print(ANDROID_LOG_INFO, "##### LoudsLm::PredictNextWords", "unigram_prediction = %s", term.c_str());
                    if (!IsReservedTerm(term)) {
                        (*results)[term] = prediction.second + kUnigramPredictionBackoff;
//                        __android_log_print(ANDROID_LOG_INFO, "##### LoudsLm::PredictNextWords", "prediction.second = %f", prediction.second);
//                        __android_log_print(ANDROID_LOG_INFO, "##### LoudsLm::PredictNextWords", "kUnigramPredictionBackoff = %f", kUnigramPredictionBackoff);
                    }
                }
            }
        }
    }

    bool LoudsLm::LookupNextWords(const NgramLoudsTrie::Key& key,
                                  const int max_results, const LogProbFloat backoff,
                                  PredictionBeam* top_predictions) const {
        const int node_id = ngram_trie_->KeyToNodeId(key);
        if (node_id == NgramLoudsTrie::kInvalidId) {
            return false;
        }
        std::vector<TermId16> child_term_ids;
        std::vector<LoudsNodeId> child_node_ids;

        // Extract the term_ids that were already predicted at higher n-gram orders.
        std::set<TermId16> predicted_term_ids;
        if (!top_predictions->empty()) {
            for (const auto& prediction : top_predictions->TakeNondestructive()) {
                predicted_term_ids.insert(prediction.first);
            }
        }

        ngram_trie_->GetChildren(node_id, &child_term_ids, &child_node_ids);
        for (size_t i = 0; i < child_term_ids.size(); ++i) {
            const TermId16 lexicon_term_id = child_term_ids[i];
            if (predicted_term_ids.find(lexicon_term_id) != predicted_term_ids.end()) {
                // Do not add or update a term that was already predicted at a higher
                // n-gram order. Note: this means that backed-off predictions are ignored
                // even if they have a higher probability, as according to the stupid
                // backoff paper http://www.aclweb.org/anthology/D07-1090.pdf.
                continue;
            }
            if (key.size() > 1) {
                // For predictions based on 3-grams and above, only predict next-words
                // that exceed the unigram logp threshold.
                LogProbFloat unigram_logp =
                        -quantizer_->Decode(LookupLogProbForTermId(lexicon_term_id));
                if (unigram_logp < params_.min_unigram_logp_for_predictions) {
                    continue;
                }
            }
            const LoudsNodeId node_id = child_node_ids[i];
            LogProbFloat logp = -quantizer_->Decode(ngram_trie_->TerminalIdToValue(
                    ngram_trie_->NodeIdToTerminalId(node_id))) +
                                backoff;
            Prediction prediction = {lexicon_term_id, logp};
            top_predictions->push({lexicon_term_id, logp});
        }
        return true;
    }

    NgramLoudsTrie::Key LoudsLm::BackoffToInVocabTermIds(
            const std::vector<TermId16>& preceding_term_ids,
            const std::vector<StringPiece>& terms, int max_term_count,
            bool preserve_last_term) const {
        NgramLoudsTrie::Key term_ids;
        if (!terms.empty()) {
            for (int i = terms.size() - 1; i >= 0; --i) {
                TermId16 term_id = TermToTermId(terms[i]);
                if (term_id == kUnkId && (!preserve_last_term || i < terms.size() - 1)) {
                    std::reverse(term_ids.begin(), term_ids.end());
                    return term_ids;
                }
                term_ids.push_back(term_id);
                if (term_ids.size() == max_term_count) {
                    std::reverse(term_ids.begin(), term_ids.end());
                    return term_ids;
                }
            }
        }
        if (!preceding_term_ids.empty()) {
            for (int i = preceding_term_ids.size() - 1; i >= 0; --i) {
                if (preceding_term_ids[i] == kUnkId) {
                    break;
                }
                term_ids.push_back(preceding_term_ids[i]);
                if (term_ids.size() == max_term_count) {
                    break;
                }
            }
        }
        std::reverse(term_ids.begin(), term_ids.end());
        return term_ids;
    }

    LogProbFloat LoudsLm::GetBackoffCost(
            const std::vector<TermId16>& backoff_terms) const {
        if (!params_.has_backoff_weights) {
            return stupid_backoff_factor();
        }
        const LoudsTerminalId terminal_id =
                backoff_terms.size() == 1 ? backoff_terms[0]
                                          : ngram_trie_->KeyToTerminalId(backoff_terms);
        if (terminal_id >= 0 && terminal_id < has_backoff_weights_.size()) {
            if (has_backoff_weights_[terminal_id]) {
                const int index = has_backoff_weights_.rank1(terminal_id);
                return -quantizer_->Decode(backoff_weights_[index]);
            }
            return 0.0f;
        } else {
            return 0.0f;
        }
    }

    bool LoudsLm::MapFromFile(const string& filename, const int offset,
                              const int length) {
        if (length < sizeof(uint64)) {
            LOG(ERROR) << "Cannot map file: length too small to contain header";
            return false;
        }
        const ScopedFileDescriptor mmap_fd(open(filename.c_str(), O_RDONLY));
        if (!mmap_fd.is_valid()) {
            LOG(ERROR) << "Can't open file descriptor. path = " << filename;
            return false;
        }
        struct stat file_stat;
        fstat(*mmap_fd, &file_stat);
        const int file_size = file_stat.st_size;
        if (length + offset > file_size) {
            LOG(ERROR) << "Cannot map file: (offset + length) greater than file size";
            return false;
        }
        const int pagesize = sysconf(_SC_PAGESIZE);
        void* const mmapped_region = mmapped_region_.Map(
                *mmap_fd, offset, length, pagesize, PROT_READ, MAP_PRIVATE);
        const bool success = (mmapped_region != nullptr)
                             ? MapFromPointer(mmapped_region, length)
                             : false;
        return success;
    }

    bool LoudsLm::MapFromFile(const string& filename) {
        const ScopedFileDescriptor mmap_fd(open(filename.c_str(), O_RDONLY));
        if (!mmap_fd.is_valid()) {
            LOG(ERROR) << "Can't open file descriptor. path = " << filename;
            return false;
        }
        struct stat file_stat;
        fstat(*mmap_fd, &file_stat);
        const int size = file_stat.st_size;

        const bool success = MapFromFile(filename, 0, size);
        return success;
    }

    bool LoudsLm::MapFromPointer(void* const map, const size_t length) {
        if (length < sizeof(uint64)) {
            LOG(ERROR) << "Cannot map file: length too small to contain header";
            return false;
        }
        // Process the header.
        MarisaMapper mapper;
        mapper.open(map, length);
        uint32 magic_number;
        mapper.map(&magic_number);
        if (magic_number != kMagicNumber) {
            LOG(ERROR) << "Map failed: invalid magic number " << magic_number;
            return false;
        }
        MarisaVector<char> params_byte_vector;
        params_byte_vector.map(&mapper);
        if (params_byte_vector.size() > 0) {
            string params_str;
            for (int i = 0; i < params_byte_vector.size(); ++i) {
                params_str.push_back(params_byte_vector[i]);
            }
            //TODO: Recover this - Wenzhe.
//            if (!params_.ParseFromString(params_str)) {
//                LOG(ERROR) << "Cannot parse params string as protobuf";
//                return false;
//            }
        }
//        if (params_.format_version != LoudsLmParams_FormatVersionNumber_FAVA_BETA) {
//            LOG(ERROR) << "Map failed: invalid format version "
//                       << params_.format_version();
//            return false;
//        }

        // Process the LM contents.
        lexicon_ = LoudsLexicon::CreateFromMapperOrNull(&mapper);
        if (!lexicon_) {
            return false;
        }
        ngram_trie_ = NgramLoudsTrie::CreateFromMapperOrNull(&mapper);
        if (!ngram_trie_) {
            return false;
        }
        mapper.map(&max_n_);
        quantizer_.reset(
                new EqualSizeBinQuantizer(params_.logp_quantizer_range, 8));

        // Load the backoff weights if they are enabled.
        if (params_.has_backoff_weights) {
            has_backoff_weights_.map(&mapper);
            backoff_weights_.map(&mapper);
        }
        if (params_.include_unigram_predictions) {
            PopulateUnigramPredictions();
        }
        return true;
    }

    bool LoudsLm::ReadFromFile(const string& filename) {
        MarisaReader reader;
        if (reader.open(filename.c_str())) {
            return ReadFromReader(&reader);
        }
        return false;
    }

    bool LoudsLm::ReadFromStream(std::istream* stream) {
        MarisaReader reader;
        reader.open(stream);
        return ReadFromReader(&reader);
    }

    bool LoudsLm::ReadFromReader(MarisaReader* reader) {
        // Process the header.
        uint32 magic_number;
        reader->read(&magic_number);
        if (magic_number != kMagicNumber) {
            LOG(ERROR) << "Read failed: invalid magic number " << magic_number;
            return false;
        }
        MarisaVector<char> params_byte_vector;
        params_byte_vector.read(reader);
        if (params_byte_vector.size() > 0) {
            string params_str;
            for (int i = 0; i < params_byte_vector.size(); ++i) {
                params_str.push_back(params_byte_vector[i]);
            }
            if (!params_.ParseFromString(params_str)) {
                LOG(ERROR) << "Cannot parse params string as protobuf";
                return false;
            }
        }
//        if (params_.format_version() != LoudsLmParams_FormatVersionNumber_FAVA_BETA) {
//            LOG(ERROR) << "Map failed: invalid format version "
//                       << params_.format_version();
//            return false;
//        }

        // Process the LM contents.
        lexicon_ = LoudsLexicon::CreateFromReaderOrNull(reader);
        if (!lexicon_) {
            return false;
        }
        ngram_trie_ = NgramLoudsTrie::CreateFromReaderOrNull(reader);
        if (!ngram_trie_) {
            return false;
        }
        reader->read(&max_n_);
        quantizer_.reset(
                new EqualSizeBinQuantizer(params_.logp_quantizer_range, 8));

        // Load the backoff weights if they are enabled.
        if (params_.has_backoff_weights) {
            has_backoff_weights_.read(reader);
            backoff_weights_.read(reader);
        }
        if (params_.include_unigram_predictions) {
            PopulateUnigramPredictions();
        }
        return true;
    }

    void LoudsLm::WriteInternal(MarisaWriter* writer) {
        // Process the header.
        writer->write(static_cast<uint32>(kMagicNumber));
        const string params_str = params_.SerializeAsString();
        MarisaVector<char> params_byte_vector;
        for (int i = 0; i < params_str.size(); ++i) {
            params_byte_vector.push_back(params_str[i]);
        }
        params_byte_vector.write(writer);

        // Process the LM contents.
        lexicon_->WriteToWriter(writer);
        ngram_trie_->WriteToWriter(writer);
        writer->write(max_n_);

        // Write the backoff weights if they are enabled.
        if (params_.has_backoff_weights) {
            has_backoff_weights_.write(writer);
            backoff_weights_.write(writer);
        }
    }

    void LoudsLm::WriteToFile(const string& filename) {
        MarisaWriter writer;
        writer.open(filename.c_str());
        WriteInternal(&writer);
    }

    void LoudsLm::WriteToStream(std::ostream& stream) {
        MarisaWriter writer;
        writer.open(stream);
        WriteInternal(&writer);
    }

    void LoudsLm::DumpNgrams(LoudsNodeId node_id, std::vector<string> prefix,
                             std::vector<Ngram>* ngrams) const {
        std::vector<TermId16> child_term_ids;
        std::vector<LoudsNodeId> child_node_ids;
        ngram_trie_->GetChildren(node_id, &child_term_ids, &child_node_ids);
        CHECK_EQ(child_term_ids.size(), child_node_ids.size()); //QCHECK_EQ

        for (int child_index = 0; child_index < child_term_ids.size();
             ++child_index) {
            const TermId16 term_id = child_term_ids[child_index];
            const LoudsNodeId child_node_id = child_node_ids[child_index];
            const LoudsTerminalId terminal_id =
                    ngram_trie_->NodeIdToTerminalId(child_node_id);
            prefix.push_back(TermIdToTerm(term_id));
            // QCHECK
            CHECK(terminal_id != -1)
                    << "Missing terminal id for node: this should not happen, since "
                    << "LoudsLm is created with has_explicit_terminals = false.";
            const LogProbFloat logp = -quantizer_->Decode(
                    ngram_trie_->TerminalIdToValue(terminal_id));
            //__android_log_print(ANDROID_LOG_INFO, "##### LoudsLm::DumpNgrams", "logp = %f", logp);
            ngrams->push_back({prefix, logp, 0.0});
            DumpNgrams(child_node_id, prefix, ngrams);
            prefix.pop_back();
        }
    }

    void LoudsLm::PopulateUnigramPredictions() {
        if (ngram_trie_ == nullptr || !top_unigrams_predictions_.empty()) {
            return;
        }
        top_unigrams_predictions_.clear();
        std::vector<TermId16> child_term_ids;
        std::vector<LoudsNodeId> child_node_ids;
        ngram_trie_->GetChildren(ngram_trie_->GetRootNodeId(), &child_term_ids,
                                 &child_node_ids);
        CHECK_EQ(child_term_ids.size(), child_node_ids.size()); //QCHECK_EQ
        PredictionBeam top_predictions(kMaxUnigramPredictions);
        for (int child_index = 0; child_index < child_term_ids.size();
             ++child_index) {
            const TermId16 term_id = child_term_ids[child_index];
            if (term_id >= kFirstUnreservedId) {
                const float logp = -quantizer_->Decode(LookupLogProbForTermId(term_id));
                top_predictions.push({term_id, logp});
            }
        }
        top_unigrams_predictions_ = top_predictions.Take();
    }

    std::vector<LoudsLm::Ngram> LoudsLm::DumpNgrams() const {
        std::vector<string> prefix;
        std::vector<Ngram> ngrams;
        DumpNgrams(ngram_trie_->GetRootNodeId(), prefix, &ngrams);
        return ngrams;
    }

}  // namespace louds
}  // namespace lm
}  // namespace keyboard
