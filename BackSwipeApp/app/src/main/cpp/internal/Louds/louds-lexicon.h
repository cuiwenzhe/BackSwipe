// A traversable lexicon based on LoudsTrie. This lexicon supports node-level
// traversals and can encode unigram log probabilities for each term and prefix.
//
// This lexicon can also serve as the vocabulary and term-to-term-id mapping for
// a language model. The advantage of the LOUDS format is that these term_ids
// are essentially free, since we can reuse the natural level-order terminal_ids
// from the LoudsTrie. One disadvantage (addressed below) is that this mapping
// is in level-order, so shorter terms will always have a lower terminal_id
// regardless of frequency.
//
// However, the language model may have a limited addressable lexicon size
// (e.g., 16-bits to conserve memory), and requires that only a subset of the
// most frequent terms from the lexicon be mapped to externally visible
// term_ids. In this case, the LoudsLexicon can create a special term_id mapping
// for only these top terms, guaranteeing that the term_ids are always between
// 0 and (max_num_term_ids_ - 1). This special mapping is implemented using an
// additional bit-vector that indicates whether or not each terminal_id
// maps to a term_id.
//
// This mapping allows a two-tier combination of a very large lexicon (with
// encoded unigrams) and a smaller higher-order language model that only needs
// to address a subset of most frequent terms.

#ifndef INPUTMETHOD_KEYBOARD_LM_LOUDS_LOUDS_LEXICON_H_
#define INPUTMETHOD_KEYBOARD_LM_LOUDS_LOUDS_LEXICON_H_

#include <algorithm>
#include <string>
#include <vector>
#include <android/log.h>

#include "../base/integral_types.h"
#include "../languageModel/constants.h"
#include "../basic-types.h"
#include "louds-trie.h"
#include "../languageModel/marisa-bitvector.h"
#include "../languageModel/marisa-io.h"
#include "../languageModel/marisa-vector.h"
#include "../base/quantizer.h"
#include "../base/stringpiece.h"
#include "../base/scoped_mmap.h"

namespace keyboard {
namespace lm {
namespace louds {

    typedef LoudsTrie<TermChar, QuantizedLogProb> Utf8CharTrie;

    // An externally visible id to refer to an term (word) in the lexicon. This id
    // is used in the (n-gram) language model.
    typedef uint32 LexiconTermId;

    class LoudsLexicon {
    public:
        // The number of bits used to quantize log probabilities.
        static constexpr int kQuantizedBits = 8;

        // Creates a new LoudsLexicon based on the following arguments.
        //
        // Args:
        //   unigrams -             A vector of unigrams (<term, logp> pairs) used to
        //                          build the lexicon.
        //   quantizer_logp_range - The lexicon quantizes log probabilities into 256
        //                          equally spaced bins spanning the range:
        //                          [-quantizer_logp_range, 0].
        //   max_num_term_ids -     The maximum number of term_ids supported by the
        //                          lexicon. If greater than 0, only the most frequent
        //                          'max_num_term_ids' terms will have an externally
        //                          visible term_id (from 0 to max_num_term_ids-1).
        //                          This is to support LMs that have a smaller
        //                          addressable term_id space (e.g., 16-bits) than the
        //                          underlying lexicon. If equal to 0, all terms will
        //                          have an externally visible term_id.
        //   has_prefix_unigrams  - Whether the encode prefix log probabilities for
        //                          non-terminal nodes. These are used to guide the
        //                          search during decoding.
        static std::unique_ptr<LoudsLexicon> CreateFromUnigramsOrNull(
                const std::vector<std::pair<string, LogProbFloat>>& unigrams,
                float quantizer_logp_range, int max_num_term_ids,
                bool has_prefix_unigrams) {
            std::unique_ptr<LoudsLexicon> lexicon(new LoudsLexicon(
                    quantizer_logp_range, max_num_term_ids, has_prefix_unigrams));
            if (!lexicon->BuildFromUnigrams(unigrams)) {
                return nullptr;
            } else {
                return lexicon;
            }
        }

        // Creates a LoudsLexicon from the provided MarisaMapper. This will
        // sequentially memory map the contents from the mapper.
        static std::unique_ptr<LoudsLexicon> CreateFromMapperOrNull(
                MarisaMapper* mapper) {
            std::unique_ptr<LoudsLexicon> lexicon(new LoudsLexicon(0, 0, false));
            if (!lexicon->MapFromMapper(mapper)) {
                return nullptr;
            } else {
                return lexicon;
            }
        }

        // Creates a LoudsLexicon from the provided MarisaReader. This will
        // sequentially load the contents from the reader.
        static std::unique_ptr<LoudsLexicon> CreateFromReaderOrNull(
                MarisaReader* reader) {
            std::unique_ptr<LoudsLexicon> lexicon(new LoudsLexicon(0, 0, false));
            if (!lexicon->ReadFromReader(reader)) {
                return nullptr;
            } else {
                return lexicon;
            }
        }

        // Creates a LoudsLexicon by reading the input POSIX file. To read Google
        // files, use LoudsLexicon::CreateFromReaderOrNull instead. This also involves
        // converting the file to a stream (using nlp_fst::IFStream) and calling
        // MarisaReader::open(std::istream *stream).
        static std::unique_ptr<LoudsLexicon> CreateFromFileOrNull(
                const string& filename) {
            std::unique_ptr<LoudsLexicon> lexicon(new LoudsLexicon(0, 0, false));
            if (!lexicon->ReadFromFile(filename)) {
                return nullptr;
            } else {
                return lexicon;
            }
        }

        // Creates a LoudsLexicon by memory mapping the input file.
        static std::unique_ptr<LoudsLexicon> CreateFromMappedFileOrNull(
                const string& filename) {
            std::unique_ptr<LoudsLexicon> lexicon(new LoudsLexicon(0, 0, false));
            if (!lexicon->MapFromFile(filename)) {
                return nullptr;
            } else {
                return lexicon;
            }
        }

        // Returns whether the lexicon encodes prefix unigram probabilities.
        bool has_prefix_unigrams() const { return has_prefix_unigrams_; }

        // Returns the string key for the given node id.
        string NodeIdToKey(LoudsNodeId node_id) const {
            Utf8CharTrie::Key key;
            trie_->NodeIdToKey(node_id, &key);
            return string(key.begin(), key.end());
        }

        // Retrieves the children (labels and node_ids) for the given parent node id.
        inline void GetChildren(const LoudsNodeId node_id,
                                std::vector<TermChar>* child_labels,
                                std::vector<LoudsNodeId>* child_node_ids) const {
            return trie_->GetChildren(node_id, child_labels, child_node_ids);
        }

        // Returns the term_id of the given string term if it maps to an externally
        // visible term_id. Otherwise, returns kUnkTermId.
        // If 'max_num_term_ids' > 0, this method will only return term_ids for
        // the top 'max_num_term_ids' terms, and guarantees that they will range
        // from [0, max_num_term_ids).
        LexiconTermId TermToTermId(const StringPiece term) const;

        // Returns the string term for the given term id.
        string TermIdToTerm(const LexiconTermId term_id) const;

        // Returns the node id for the given string key (which may be a complete-term
        // or term-prefix), if it exists in the lexicon. Otherwise, returns -1.
        LoudsNodeId KeyToNodeId(const StringPiece string_key) const;

        // Retrieves the term unigram log probability given node id.
        // Returns whether a term was found that matches the node_id.
        bool TermLogProbForNodeId(int node_id, LogProbFloat* logp) const;

        // Returns the prefix unigram log probability for the give node id.
        // Returns whether a prefix was found that matches the node_id.
        bool PrefixLogProbForNodeId(LoudsNodeId node_id, LogProbFloat* logp) const;

        // Writes the contents of the lexicon to a file. Does not modify the lexicon.
        void WriteToFile(const string& filename) const;

        // Writes the lexicon to the writer. Does not modify the lexicon.
        void WriteToWriter(MarisaWriter* writer) const;

    private:
        // Private constructor for a new empty LoudsLexicon with the given properties.
        // See comments for LoudsLexicon::CreateFromUnigramsOrNull.
        LoudsLexicon(float quantizer_logp_range, int max_num_term_ids,
                     bool has_prefix_unigrams)
                : trie_(),
                  has_prefix_unigrams_(has_prefix_unigrams),
                  quantizer_logp_range_(quantizer_logp_range),
                  max_num_term_ids_(max_num_term_ids),
                  has_termids_(),
                  has_prefix_values_(),
                  prefix_values_(),
                  quantizer_(
                          new EqualSizeBinQuantizer(quantizer_logp_range_, kQuantizedBits)) {}

        // Converts a string to a Utf8CharTrie key (vector<utf8>).
        static void StringToKey(const StringPiece term, Utf8CharTrie::Key* key) {
            key->assign(term.begin(), term.end());
        }

        // Converts a Utf8CharTrie key (vector<utf8>) to a string.
        static void KeyToString(const Utf8CharTrie::Key key, string* word) {
            word->assign(key.begin(), key.end());
        }

        // Private method to build the louds lexicon from the given unigrams, as
        // <term, logp> pairs. This method is only called from the static factory
        // method LoudsLexicon::CreateFromUnigramsOrNull.
        //
        // Note: Special terms (according to IsSpecialTerm) should not be included in
        // the input unigrams vector.
        bool BuildFromUnigrams(
                const std::vector<std::pair<string, LogProbFloat>>& unigrams);

        // Returns the externally visible term_id for the given internal terminal id.
        LexiconTermId TerminalIdToTermId(LoudsTerminalId terminal_id) const;

        // Returns the externally visible term_id for the given node id.
        LexiconTermId NodeIdToTermId(int node_id) const;

        // Returns the internal terminal id for the given externally visible term_id.
        LoudsTerminalId TermIdToTerminalId(LexiconTermId term_id) const;

        // Creates the special mapping for the most frequent terms to externally
        // visible term_ids.
        void MapExternalTermIds(
                const std::vector<std::pair<string, LogProbFloat>>& unigrams);

        // Integrates the prefix unigram log probabilities into the lexicon.
        void IntegratePrefixLogProbs(
                const std::vector<std::pair<string, LogProbFloat>>& unigrams);

        // Initializes and maps the contents of this Lexicon from the mapper.
        bool MapFromMapper(MarisaMapper* mapper);

        // Initializes and maps the contents of this Lexicon from a file.
        // This will create a new memory map for the file.
        bool MapFromFile(const string& filename);

        // Initializes and reads the contents of this Lexicon from a reader.
        bool ReadFromReader(MarisaReader* reader);

        // Initializes and reads the contents of this Lexicon from a file.
        bool ReadFromFile(const string& filename);

        // The underlying LOUDS trie for the lexicon.
        std::unique_ptr<Utf8CharTrie> trie_;

        // Whether the lexicon encodes prefix unigram probabilities.
        bool has_prefix_unigrams_;

        // The lexicon quantizes log probabilities into 256 equally spaced bins
        // spanning the range: [-quantizer_logp_range_, 0].
        float quantizer_logp_range_;

        // Only map term_ids for this number of most frequent terms.
        int max_num_term_ids_;

        // A bit vector specifying whether or not each terminal id represents a
        // frequent term that should have an externally visible term_id.
        MarisaBitVector has_termids_;

        // A bit vector specifying whether or not each node id has an associated
        // prefix unigram probabilities. The lexicon saves space by only encoding a
        // prefix value when the log probabilities changes from one character to the
        // next. Otherwise, the decoder can just use the parent prefix's value.
        MarisaBitVector has_prefix_values_;

        // The quantized prefix values for the node ids referenced in
        // has_prefix_values_.
        MarisaVector<QuantizedLogProb> prefix_values_;

        std::unique_ptr<EqualSizeBinQuantizer> quantizer_;

        // The scoped memory map region for mapping a LoudsLexicon. Should only be
        // used when loading the lexicon by itself.
        ScopedMmap mmapped_region_;

        // TODO(ouyang): If needed, consider adding another Vector<...> to encode
        // additional properties for each terminal (e.g., profanity flag).

        DISALLOW_COPY_AND_ASSIGN(LoudsLexicon);
    };

    inline LexiconTermId LoudsLexicon::TerminalIdToTermId(
            LoudsTerminalId terminal_id) const {
        if (max_num_term_ids_ == 0) {
            return terminal_id + kFirstUnreservedId;
        }
        if (has_termids_[terminal_id]) {
            return has_termids_.rank1(terminal_id) + kFirstUnreservedId;
        }
        return kUnkId;
    }

    inline LexiconTermId LoudsLexicon::NodeIdToTermId(int node_id) const {
        if (!trie_->HasTerminalId(node_id)) {
            return kUnkId;
        }
        const int terminal_id = trie_->NodeIdToTerminalId(node_id);
        return TerminalIdToTermId(terminal_id);
    }

    inline LoudsTerminalId LoudsLexicon::TermIdToTerminalId(
            LexiconTermId term_id) const {
        if (term_id >= kFirstUnreservedId) {
            if (max_num_term_ids_ == 0) {
                return term_id - kFirstUnreservedId;
            }
            if (term_id < max_num_term_ids_) {
                return has_termids_.select1(term_id - kFirstUnreservedId);
            }
        }
        return Utf8CharTrie::kInvalidId;
    }

}  // namespace louds
}  // namespace lm
}  // namespace keyboard

#endif  // INPUTMETHOD_KEYBOARD_LM_LOUDS_LOUDS_LEXICON_H_
