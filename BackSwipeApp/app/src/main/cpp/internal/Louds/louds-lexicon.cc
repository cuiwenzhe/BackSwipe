#include "louds-lexicon.h"

#include <map>
#include <utility>

#include "../base/logging.h"
#include "../base/unilib.h"

namespace keyboard {
namespace lm {
namespace louds {

    bool LoudsLexicon::BuildFromUnigrams(
            const std::vector<std::pair<string, LogProbFloat>>& unigrams) {
        Utf8CharTrie::KeyValueMap key_values;
        for (int i = 0; i < unigrams.size(); ++i) {
            const QuantizedLogProb qlogp = quantizer_->Encode(-unigrams[i].second);
            Utf8CharTrie::Key key;
            StringToKey(unigrams[i].first, &key);
            key_values[key] = qlogp;
        }
        trie_ = Utf8CharTrie::CreateFromKeyValueMapOrNull(key_values, true);
        if (trie_ == nullptr) {
            return false;
        }
        if (max_num_term_ids_ > 0) {
            MapExternalTermIds(unigrams);
        }
        if (has_prefix_unigrams_) {
            IntegratePrefixLogProbs(unigrams);
        }
        return true;
    }

    bool LoudsLexicon::PrefixLogProbForNodeId(LoudsNodeId node_id,
                                              LogProbFloat* logp) const {
        if (node_id >= has_prefix_values_.size()) {
            return false;
        }
        if (!has_prefix_values_[node_id]) {
            return false;
        }
        const int prefix_id = has_prefix_values_.rank1(node_id);
        *logp = -quantizer_->Decode(prefix_values_[prefix_id]);
//        __android_log_print(ANDROID_LOG_INFO, "##### LoudsLexicon::BuildFromUnigrams", "logp = %f", *logp);
        return true;
    }

    void LoudsLexicon::IntegratePrefixLogProbs(
            const std::vector<std::pair<string, LogProbFloat>>& unigrams) {
        const int size = unigrams.size();
        std::map<string, LogProbFloat> prefix_logps;

        // Retrieve the maximum unigram completion probabilities for each possible
        // prefix. Note that this does not take into account the language context.
        for (int i = 0; i < size; ++i) {
            const string term = unigrams[i].first;
            const LogProbFloat logp = unigrams[i].second;
            const LogProbFloat term_logp = logp;
            int prefix_length = 0;
            while (prefix_length < term.length()) {
                // Only extract prefixes that align to full UTF-8 character boundaries.
                prefix_length =
                        prefix_length + UniLib::OneCharLen(term.c_str() + prefix_length);
                const string prefix = term.substr(0, prefix_length);
                {
                    auto it = prefix_logps.find(prefix);
                    LogProbFloat max_logp = term_logp;
                    if (it != prefix_logps.end() && it->second > max_logp) {
                        max_logp = it->second;
                    }
                    prefix_logps[prefix] = max_logp;
                }
            }
        }

        std::vector<Utf8CharTrie::Key> keys;
        std::map<LoudsNodeId, LogProbFloat> node_id_prefix_logps;

        // Add all of the prefixes that are not redundant.
        for (const auto& entry : prefix_logps) {
            const int len = entry.first.length();
            LogProbFloat parent_logp = -std::numeric_limits<float>::infinity();
            for (int parent_len = len - 1; parent_len > 0; --parent_len) {
                const string parent_prefix = entry.first.substr(0, parent_len);
                if (prefix_logps.find(parent_prefix) != prefix_logps.end()) {
                    parent_logp = prefix_logps[parent_prefix];
                    break;
                }
            }
            // Add a prefix logprob only if it differs from that of the parent prefix.
            if (entry.second != parent_logp) {
                const LoudsNodeId node_id = KeyToNodeId(entry.first);
                node_id_prefix_logps[node_id] = entry.second;
            }
        }

        for (const auto& entry : node_id_prefix_logps) {
            const LoudsNodeId node_id = entry.first;
            while (has_prefix_values_.size() < node_id) {
                has_prefix_values_.push_back(0);
            }
            has_prefix_values_.push_back(1);
            prefix_values_.push_back(quantizer_->Encode(-entry.second));
        }
        has_prefix_values_.build();
    }

    void LoudsLexicon::MapExternalTermIds(
            const std::vector<std::pair<string, LogProbFloat>>& unigrams) {
        const int size = unigrams.size();
        std::vector<std::pair<LogProbFloat, StringPiece>> sorted_terms;
        for (int i = 0; i < size; ++i) {
            sorted_terms.push_back(std::pair<LogProbFloat, StringPiece>(
                    unigrams[i].second, unigrams[i].first));
        }
        std::sort(sorted_terms.begin(), sorted_terms.end());
        std::reverse(sorted_terms.begin(), sorted_terms.end());

        std::vector<bool> is_frequent_termids;
        is_frequent_termids.resize(size);
        for (int i = 0; i < size; ++i) {
            is_frequent_termids[i] = false;
        }
        const int num_regular_term_ids = max_num_term_ids_ - kFirstUnreservedId;
        for (int i = 0; i < num_regular_term_ids && i < size; ++i) {
            const int node_id = KeyToNodeId(sorted_terms[i].second);
            CHECK_GE(node_id, 0) /*<< sorted_terms[i].second*/;
            const LoudsTerminalId terminal_id = trie_->NodeIdToTerminalId(node_id);
            CHECK_GE(terminal_id, 0) /*<< sorted_terms[i].second*/;
            is_frequent_termids[terminal_id] = true;
        }
        for (int i = 0; i < size; ++i) {
            has_termids_.push_back(is_frequent_termids[i]);
        }
        has_termids_.build();
    }

    LexiconTermId LoudsLexicon::TermToTermId(const StringPiece term) const {
        const TermId reserved_termid = ReservedTermToTermId(term);
        if (reserved_termid < kFirstUnreservedId) {
            return reserved_termid;
        }
        const int node_id = KeyToNodeId(term);
        if (node_id == Utf8CharTrie::kInvalidId) {
            return kUnkId;
        }
        return NodeIdToTermId(node_id);
    }

    string LoudsLexicon::TermIdToTerm(LexiconTermId term_id) const {
        if (term_id < kFirstUnreservedId) {
            return ReservedTermIdToTerm(term_id);
        }
        const LoudsTerminalId terminal_id = TermIdToTerminalId(term_id);
        LoudsNodeId node_id = trie_->TerminalIdToNodeId(terminal_id);
        return NodeIdToKey(node_id);
    }

    LoudsNodeId LoudsLexicon::KeyToNodeId(const StringPiece string_key) const {
        Utf8CharTrie::Key key;
        StringToKey(string_key, &key);
        return trie_->KeyToNodeId(key);
    }

    bool LoudsLexicon::TermLogProbForNodeId(int node_id, LogProbFloat* logp) const {
        if (node_id < 0) {
            return false;
        }
        const LoudsTerminalId terminal_id = trie_->NodeIdToTerminalId(node_id);
        if (terminal_id < 0) {
            return false;
        }
        *logp = -quantizer_->Decode(trie_->TerminalIdToValue(terminal_id));
//        __android_log_print(ANDROID_LOG_INFO, "##### LoudsLexicon::TermIdToTerm", "logp = %f", *logp);
        return true;
    }

    void LoudsLexicon::WriteToWriter(MarisaWriter* writer) const {
        trie_->WriteToWriter(writer);
        has_termids_.write(writer);
        has_prefix_values_.write(writer);
        prefix_values_.write(writer);
        writer->write(has_prefix_unigrams_);
        writer->write(quantizer_logp_range_);
        writer->write(max_num_term_ids_);
    }

    void LoudsLexicon::WriteToFile(const string& filename) const {
        MarisaWriter writer;
        writer.open(filename.c_str());
        WriteToWriter(&writer);
    }

    bool LoudsLexicon::MapFromMapper(MarisaMapper* mapper) {
        trie_ = Utf8CharTrie::CreateFromMapperOrNull(mapper);
        if (trie_ == nullptr) {
            return false;
        }
        has_termids_.map(mapper);
        has_prefix_values_.map(mapper);
        prefix_values_.map(mapper);
        mapper->map(&has_prefix_unigrams_);
        mapper->map(&quantizer_logp_range_);
        mapper->map(&max_num_term_ids_);
        quantizer_.reset(
                new EqualSizeBinQuantizer(quantizer_logp_range_, kQuantizedBits));
        return true;
    }

    bool LoudsLexicon::MapFromFile(const string& filename) {
        int fd = 0;
        fd = ::open(filename.c_str(), O_RDONLY);
        if (fd < 0) {
            LOG(ERROR) << "Failed to open file " << filename;
            return false;
        }

        struct stat file_stat;
        fstat(fd, &file_stat);
        const int length = file_stat.st_size;

        const int pagesize = sysconf(_SC_PAGESIZE);

        void* map =
                mmapped_region_.Map(fd, 0, length, pagesize, PROT_READ, MAP_SHARED);

        MarisaMapper mapper;
        mapper.open(map, length);
        return MapFromMapper(&mapper);
    }

    bool LoudsLexicon::ReadFromReader(MarisaReader* reader) {
        trie_ = Utf8CharTrie::CreateFromReaderOrNull(reader);
        if (trie_ == nullptr) {
            return false;
        }
        has_termids_.read(reader);
        has_prefix_values_.read(reader);
        prefix_values_.read(reader);
        reader->read(&has_prefix_unigrams_);
        reader->read(&quantizer_logp_range_);
        reader->read(&max_num_term_ids_);
        quantizer_.reset(
                new EqualSizeBinQuantizer(quantizer_logp_range_, kQuantizedBits));
        return true;
    }

    bool LoudsLexicon::ReadFromFile(const string& filename) {
        MarisaReader reader;
        reader.open(filename.c_str());
        return ReadFromReader(&reader);
    }

}  // namespace louds
}  // namespace lm
}  // namespace keyboard
