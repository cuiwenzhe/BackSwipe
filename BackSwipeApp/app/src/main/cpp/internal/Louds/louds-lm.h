// An n-gram language model implemented using a LoudsTrie. Terms are encoded
// using 16-bit integer TermId16. Log probabilities are encoded using 8-bit
// QuantizedLogProb.
//
// The LoudsLm can also store backoff weights when the 'has_backoff_weights'
// flag is true. It minimizes storage cost by using an additional bit-vector to
// indicate only those n-grams that have a non-zero backoff weight. Values are
// encoded using 8-bit QuantizedLogProb.
//
// The LoudsLm also owns a LoudsLexicon, which provides the term to TermId
// mapping. This lexicon can also be traversed directly by the decoder,
// without the need for a separate data structure.
//
// Note: Currently the LoudsLm exclude n-grams that contain the <UNK> term.
// this means that the language model may not be properly normalized.

#ifndef INPUTMETHOD_KEYBOARD_LM_LOUDS_LOUDS_LM_H_
#define INPUTMETHOD_KEYBOARD_LM_LOUDS_LOUDS_LM_H_

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <vector>
#include <android/log.h>

#include "../base/integral_types.h"
#include "../base/logging.h"
#include "../basic-types.h"
#include "louds-lexicon.h"
#include "louds-trie.h"
//#include "inputmethod/keyboard/lm/louds/proto/louds-lm.pb.h"
#include "../base/quantizer.h"
#include "../base/stringpiece.h"
#include "../languageModel/top_n.h"
#include "../base/scoped_mmap.h"
#include "LoudsLmParams.h"

namespace keyboard {
namespace lm {
namespace louds {

    // The language model is represented as a LoudsTrie with 16-bit termids as
    // node labels and QuantizedLogProbs as node values.
    typedef LoudsTrie<TermId16, QuantizedLogProb> NgramLoudsTrie;

    class LoudsLm {
    public:
        // A "magic" number to store in the header of each LoudsLm file to ensure
        // that the file is not corrupted. Files that do not start with this 32-bit
        // unsigned integer will not be loaded.
        static constexpr uint32 kMagicNumber = 0xEFA31CB9;

        // Stores an n-gram as a sequence of terms and associated log probabilities
        // and backoff weights.
        struct Ngram {
            std::vector<string> terms;
            LogProbFloat logp;
            LogProbFloat backoff;
            bool operator<(const Ngram& t) const { return logp > t.logp; }
            bool operator>(const Ngram& t) const { return logp < t.logp; }
            bool operator==(const Ngram& t) const { return logp == t.logp; }
        };

        // A prediction represents a term id and log probability pair.
        typedef std::pair<TermId16, LogProbFloat> Prediction;

        // Comparator that returns true if the left prediction has a greater logprob.
        struct PredictionGreaterLogProb {
            bool operator()(const Prediction& left, const Prediction& right) {
                return left.second > right.second;
            }
        };

        // An TopN beam for predictions.
        typedef TopN<Prediction, PredictionGreaterLogProb> PredictionBeam;

        // Creates a LoudsLm by reading the input file.
        static std::unique_ptr<LoudsLm> CreateFromFileOrNull(const string& filename) {
            const LoudsLmParams default_parms;
            std::unique_ptr<LoudsLm> lm(new LoudsLm(default_parms));
            if (!lm->ReadFromFile(filename)) {
                return nullptr;
            } else {
                return lm;
            }
        }

        // Creates a LoudsLm by memory mapping the input file.
        static std::unique_ptr<LoudsLm> CreateFromMappedFileOrNull(
                const string& filename) {
            const LoudsLmParams default_params;
            std::unique_ptr<LoudsLm> lm(new LoudsLm(default_params));
            if (!lm->MapFromFile(filename)) {
                return nullptr;
            } else {
                return lm;
            }
        }

        // Creates a LoudsLm by reading the input stream.
        static std::unique_ptr<LoudsLm> CreateFromStreamOrNull(std::istream* stream) {
            const LoudsLmParams default_params;
            std::unique_ptr<LoudsLm> lm(new LoudsLm(default_params));
            if (!lm->ReadFromStream(stream)) {
                return nullptr;
            } else {
                return lm;
            }
        }

        // Creates a LoudsLm by memory mapping the input file, with the given offset
        // and size. This is mainly used to load a resource file from an Android apk.
        // (suwen) This is the main method used to create a louds lm.
        static std::unique_ptr<LoudsLm> CreateFromMappedFileOrNull(
                const string& filename, const int offset, const int size) {
            const LoudsLmParams default_params;
            std::unique_ptr<LoudsLm> lm(new LoudsLm(default_params));
            if (!lm->MapFromFile(filename, offset, size)) {
                return nullptr;
            } else {
                return lm;
            }
        }

        // Creates a new LoudsLm (language model and lexicon) from the given ngrams
        // and params.
        //
        // The lexicon will be constructed automatically from the unigrams.
        static std::unique_ptr<LoudsLm> CreateFromNgramsOrNull(
                const std::vector<Ngram>& ngrams, const LoudsLmParams params) {
            std::unique_ptr<LoudsLm> lm(new LoudsLm(params));
            if (!lm->Build(ngrams)) {
                return nullptr;
            } else {
                return lm;
            }
        }

        ~LoudsLm();

        // Returns the TermId for the given term. If the term is OoV, this method
        // returns kUnkTermId.
        TermId16 TermToTermId(const StringPiece term) const {
            return lexicon_->TermToTermId(term);
        }

        // Returns the term (string) for the given term id.
        string TermIdToTerm(const TermId16 term_id) const {
            return lexicon_->TermIdToTerm(term_id);
        }

        // Returns the sequence of TermIds for the given string terms.
        NgramLoudsTrie::Key TermsToTermIds(const std::vector<string>& terms) const;

        // Looks up the conditional log probability of the last term in the terms
        // vector, given the preceding term_ids and then the other terms.
        // Returns whether or not the last term was in the lexicon.
        //
        // Example: log p(terms[last] | term_ids[...], terms[0..last-1])
        bool LookupConditionalLogProb(const NgramLoudsTrie::Key& term_ids,
                                      const std::vector<StringPiece>& terms,
                                      LogProbFloat* value) const;

        // Predicts the most probable next words given the preceding term_ids and then
        // the preceding context terms.
        void PredictNextWords(const NgramLoudsTrie::Key& term_ids,
                              const std::vector<StringPiece>& context,
                              const int max_results,
                              std::map<string, LogProbFloat>* results) const;

        // Writes the contents of the LM (lexicon and n-gram trie) to the file.
        void WriteToFile(const string& filename);

        // Writes the contents of the LM (lexicon and n-gram trie) to the ostream.
        void WriteToStream(std::ostream& filename);

        // Returns the lexicon for the language model.
        const LoudsLexicon* lexicon() const { return lexicon_.get(); }

        // Returns the maximum n-gram order of the language model.
        int max_n() const { return max_n_; }

        // Returns the stupid backoff factor for the language model.
        LogProbFloat stupid_backoff_factor() const {
            // TODO(ouyang): Add experiment config to modify this value.
            return params_.stupid_backoff_logp;
        }

        // Returns the params for this LoudsLm.
        const LoudsLmParams& params() const { return params_; }


        // Sets the badwords associated with this LM.  Duplicates or words
        // outside the unigram set are ignored.
        void SetBadwords(const std::vector<string>& badwords);


        // Sets the parameters to the given new_params.
        void set_params(const LoudsLmParams& new_params) {
            params_ = new_params;
        }

        // Returns all of the n-grams in the LM.  Intended for use when debugging, or
        // by tools that inspect the LM.  We include
        std::vector<Ngram> DumpNgrams() const;

    private:
        // Private constructor for a new LoudsLm with a 16-bit TermId address space.
        explicit LoudsLm(LoudsLmParams params)
                : params_(params),
                  max_n_(0),
                  lexicon_(),
                  ngram_trie_(),
                  mmapped_region_(),
                  quantizer_(
                          new EqualSizeBinQuantizer(params.logp_quantizer_range, 8)) {}

        // Builds the language model and lexicon with the given ngrams.
        // The vocabulary will be constructed from the unigrams.
        bool Build(const std::vector<Ngram>& ngrams);

        // Converts an ngram object to an ngram LOUDS trie key.
        bool NgramToKey(const Ngram& ngram, NgramLoudsTrie::Key* key) const;

        // Returns the unigram probability for the given term_id.
        QuantizedLogProb LookupLogProbForTermId(const TermId16 term_id) const {
            // The ngram trie is structured such that for unigrams, the term_id is
            // equal to the LoudsTerminalId.
            return ngram_trie_->TerminalIdToValue(term_id);
        }

        // Returns the most probable next words with the given key as context.
        bool LookupNextWords(const NgramLoudsTrie::Key& key, const int max_results,
                             const LogProbFloat backoff,
                             PredictionBeam* top_predictions) const;

        // Takes a sequence of term_ids and terms, then performs backoffs until there
        // are only in-vocabulary terms remaining.
        //
        // Example: {<UNK>, and, <UNK>, are, here} => {are, here}
        // Example: {<UNK>, and, <UNK>, are, <UNK>} => {}
        // Example: {<UNK>, and, <UNK>, are, <UNK>} => {<UNK>} (preserve_last_term)
        NgramLoudsTrie::Key BackoffToInVocabTermIds(
                const std::vector<TermId16>& preceding_term_ids,
                const std::vector<StringPiece>& terms, int max_term_count,
                bool preserve_last_term) const;

        // Returns the backoff cost associated with the given term_id sequence.
        LogProbFloat GetBackoffCost(const std::vector<TermId16>& backoff_terms) const;

        // Memory maps the contents of the LM (lexicon trie and n-gram trie) from the
        // file.
        bool MapFromFile(const string& filename);

        // Memory maps the contents of the LM (lexicon trie and n-gram trie) from the
        // file, with the given offset and length. This is mainly used to load a
        // resource file from an Android apk.
        bool MapFromFile(const string& filename, const int offset, const int length);

        // Memory maps the contents of the LM (lexicon and n-gram trie) from the
        // given pointer with the given size.
        bool MapFromPointer(void* const ptr, const size_t size);

        // Loads the contents of the LM (lexicon and n-gram trie) from the file.
        // Note that this method only works with local files. To load files from CNS,
        // please open the file as a stream and use ReadFromStream (see example in
        // louds-lm_test.cc).
        bool ReadFromFile(const string& filename);

        // Loads the contents of the LM (lexicon and n-gram trie) from the stream.
        bool ReadFromStream(std::istream* stream);

        // Loads the contents of the LM (lexicon and n-gram trie) from the reader.
        bool ReadFromReader(MarisaReader* reader);

        // Outputs the LoudsLm through the provided MarisaWriter.
        void WriteInternal(MarisaWriter* writer);

        // Helper function called by DumpNgrams() that dumps all n-grams in the
        // subtree rooted at "node_id", excluding "node_id" itself.  "prefix" stores
        // the term strings on the path from the root to "node_id", inclusive.
        void DumpNgrams(LoudsNodeId node_id, std::vector<string> prefix,
                        std::vector<Ngram>* ngrams) const;

        // Populates the pre-computed list of top unigram predictions. Should be
        // called once on initialization.
        void PopulateUnigramPredictions();

        // The params for this LoudsLm.
        LoudsLmParams params_;

        // The maximum n-gram order of the language model.
        int max_n_;

        // The lexicon for the language model. Also provides the term-to-termid map.
        std::unique_ptr<LoudsLexicon> lexicon_;

        // The LOUDS trie storing the n-grams for the language model.
        std::unique_ptr<NgramLoudsTrie> ngram_trie_;

        // The scoped memory map region used to load the language model.
        ScopedMmap mmapped_region_;

        // The quantizer for log probabilities.
        std::unique_ptr<EqualSizeBinQuantizer> quantizer_;

        // A bit vector specifying whether or not each terminal id (ngram) has
        // an associated backoff weight. If false, we assume the backoff weight is 0.
        MarisaBitVector has_backoff_weights_;

        // Backoff weights
        MarisaVector<QuantizedLogProb> backoff_weights_;

        // The pre-computed list of top unigram predictions.
        std::vector<Prediction> top_unigrams_predictions_;
    };


}  // namespace louds
}  // namespace lm
}  // namespace keyboard

#endif  // INPUTMETHOD_KEYBOARD_LM_LOUDS_LOUDS_LM_H_
