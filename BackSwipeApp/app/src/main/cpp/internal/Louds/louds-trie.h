// A succinct traversable trie data structure using Level-Order Unary Degree
// Sequence representation (LOUDS).
//
// The main benefit of a LOUDS trie is compactness. It can encode the structure
// of the trie using only ~2 bits per node (close to the information-theoretic
// lower bound). It also provides a natural level-order indexing of the nodes
// and terminals that can be used as a "free" term-to-term-id map. In other trie
// implementations, this functionality would require storing extra integer
// term id's for each term.
//
// Preliminary benchmarks show that LoudsTrie is about 8x more compact that
// CompactTrie (util/gtl/compacttrie). In terms of speed, LoudsTrie is faster
// than CompactTrie for key-value lookups when the average branching factor is
// greater than 4, but can be slower for very sparse tries.
// See: //inputmethod/keyboard/lm/louds:trie-benchmarks_test
//
// External evaluations have also found that LOUDS-based tries are significantly
// more compact than a wide range of other trie implementations, and about
// 8-10x more compact than double-array tries (like CompactTrie).
// See: http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/
//
// This implementation of a LOUDS-based trie supports custom key/value types and
// node level traversal operations like:
// 1. Looking up an arbitrary node given a node_id.
// 2. Retrieving a node's children.
// 3. Retrieving a node's parent.
//
// Main Reference:
//     [1] "Engineering the LOUDS Succinct Tree Representation" (2006)
//     O'Neil Delpratt, Naila Rahman, Rajeev Raman
//
// Additional Reference:
//     [2] "Representing Trees of Higher Degree" (2005)
//     David Benoit, Erik D. Demaine, J. Ian Munro, and Venkatesh Raman
//
// ====================================================================
// STORAGE
// ====================================================================
//
// A LOUDS tree encodes the structure of tree using a bit-vector - a vector of
// bits augmented to support rank() and select() operations (see below) in
// constant time.
//
//  1. rank1(x) is the number of 1-bits to the left of (but not including)
//     position x in the bit-vector.
//  2. select1(i) is the i-th 1-bit in the bit vector (starting from 0-th).
//  3. rank0 and select0 are analogously defined for 0-bits.
//
// The LOUDS bit-vector representation is constructed by traversing the trie in
// level-order (i.e., breadth-first), starting with the root node, For each
// node, we emit a bit sequence of d '1' bits and one '0' bit, where d is the
// degree (number of children) of the node. This is known as the node's (unary)
// degree sequence. To make the traversal logic simpler, we follow [1] by adding
// a special super-root that acts as the parent to the regular-root node.
//
// The best way to visualize this is using an example:
//
// Example LOUDS trie with two words: "a" and "bc"
//
// Tree:          Degree sequence for each node:
//    (*)         10 (super-root)
//     |
//    (*)         110 (regular-root)
//   /   \
// [a]   (b)      0 (a)   10 (b)
//        |
//       [c]      0 (c)
//
// The LOUDS bit-vector (level-order concatenation of each degree sequences)
// bit-index:          0 1   2 3 4   5   6 7   8
// ---------------------------------------------
// LOUDS[bit-index]:   1 0 | 1 1 0 | 0 | 1 0 | 0
// rank0(bit-index):   0 0 | 1 1 1 | 2 | 3 3 | 4
// rank1(bit-index):   0 1 | 1 2 3 | 3 | 3 4 | 4
// ---------------------------------------------
// label(child edge):  *   | a b   |   | c   |
//
// Note: | delimits the degree sequences for different nodes.
//
// NODE ID SCHEME:
// This implementation uses the "ones-based" naming scheme to refer to nodes,
// where we define the node_id for each node as the position of the i-th 1 bit
// in the LOUDS bit-vector (i.e., its child edge). It is essentially the order
// of the node in level-order traversal if the super-root is skipped (since
// the super-root is not a child, it doesn't have a corresponding 1).
//
// node_id := rank1(bit_index).
//
// Since rank and select are inverse operations:
// bit_index = select1(node_id)
//
// This index can be used to retrieve metadata (like UTF8 labels and logprob
// values) associated with each node, and also for traversal of the trie (read
// below).
//
// Some examples of rank/select on the LOUDS trie above:
// rank1(3) = 2      the rank of the child edge bit for [b] (i.e., its node_id)
// select1(2) = 3    select1 is the inverse of rank1
// select0(2) = 5    the 0-bit just before the start of [b]'s degree sequence
// select0(2+1) = 7  the 0-bit at the end of [b]'s degree sequence
// rank0(2) = 1      the number of 0-bits before the start of [b]'s parent's
//                   degree sequence (including the super-root's 0-bit)
//
// ====================================================================
// TRAVERSAL
// ====================================================================
//
// The node_id is based on the occurrence of a node in its parent degree
// sequence, whereas the node's children occur in its own degree sequence.  To
// traverse the trie, we need to be able find the 1-based index of the node's
// children and its parent in the LOUDS bit-vector given a node_id. We can then
// translate the 1-based index into a node_id for the respective child/parent as
// described above.
//
// NODE CHILDREN LOOKUP:
// 1. The '0' at the end of the preceding node's degree sequence is:
//    select0(node_id)
// 2. The '0' at the end of the node's own degree sequence is:
//    select0(node_id + 1)
//
// Note: The (+1) offset is needed since the super-root has a corresponding
// 0-bit (to represent the end of its own degree sequence) but no corresponding
// 1-bit (since it is not a child of another node). As a result, the super-root
// needs to be explicit skipped when using rank0/select0 but not rank1/select1.
//
// Therefore:
// 1. The bit index of the first child = select0(node_id) + 1
// 2. The bit index of the last child = select0(node_id + 1) - 1
//
// NODE PARENT LOOKUP BY BIT_INDEX:
// 1. Assume bit_index is the index of the 1-bit corresponding to a node.
// 2. rank0(bit_index): represents the number of 0-bits visited before
//    reaching the 0-bit corresponding to the end of the parent node's degree
//    sequence. This is the parent's rank in the level-order traversal, when
//    including the 0-bit for the super-root.
//
// parent_node_id = rank0(bit_index) - 1     (-1 in oder to skip the super-root)
//
// NODE PARENT LOOKUP BY NODE_ID:
// 1. parent_node_id = rank0(select1(node_id)) - 1
// 2. Apply rule: rank0(x) = x - rank1(x):
//    This just means that the number of 0's before index x is equal to the
//    value of x minus the number of 1's before x (since every bit that's not a
//    1 is a 0).
// 3. parent_node_id = (select1(node_id) - rank1(select1(node_id))) - 1
// 4. parent_node_id = select1(node_id) - node_id - 1
//
// These traversal rules are also presented in [1] (Figure 3), under the
// "Ones-based numbering" scheme.

#ifndef INPUTMETHOD_KEYBOARD_LM_LOUDS_LOUDS_TRIE_H_
#define INPUTMETHOD_KEYBOARD_LM_LOUDS_LOUDS_TRIE_H_

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "../base/integral_types.h"
#include "../base/logging.h"
#include "../base/macros.h"
#include "../languageModel/marisa-bitvector.h"
#include "../languageModel/marisa-io.h"
#include "../languageModel/marisa-vector.h"

using std::vector;

namespace keyboard {
namespace lm {
namespace louds {

    // An id used to refer to a node in the trie.
    // See LoudsTrie::GetNodeIdForBitIndex for more details.
    typedef int LoudsNodeId;

    // The LoudsTerminalId is used to reference terminal nodes, which are nodes that
    // have an associated value. This concept of a terminal is especially useful for
    // lexicon tries, where we need to distinguish between complete keys (like
    // "word") that have an associated value and incomplete keys (like "wor").
    // If has_explicit_terminals_ is false, then all nodes have a LoudsTerminalId.
    // Otherwise, only complete keys have a LoudsTerminalId.
    typedef int LoudsTerminalId;

    // A LOUDS trie encodes a mapping between keys (sequences of type T) and values
    // (of type V). This data structure is traversable, providing node-level access
    // to the internal tree structure.
    //
    // A key represents a sequence of labels (type T), and defines the path from
    // the root of the trie (empty label) to the last label in the vector.
    // The type K must be a sequence container with bidirectional iterators.
    //
    // Example usage:
    // 1. Call factory method CreateFrom* to create or load a LoudsTrie.
    // 2. Get the node_id of the root by calling LoudsTrie::GetRootNodeId
    // 3. Get the vector of children by calling LoudsTrie::GetChildren
    // 4. Get the key associated with a node_id by calling LoudsTrie::NodeIdToKey.
    //    or by traversing up the parents of the node to the root using
    //    LoudsTrie::NodeIdToParentNodeId
    // 5. Get the terminal id using LoudsTrie::NodeIdToTerminalId. This can be used
    //    as a term-to-term_id map.
    //
    template <typename T, typename V, typename K = vector<T>>
    class LoudsTrie {
    public:
        static constexpr LoudsTerminalId kInvalidId = -1;

        // The node_id of the root, which is always 0 in our LOUDS representation.
        static constexpr LoudsNodeId kRootNodeId = 0;

        typedef V Value;
        typedef K Key;
        typedef std::map<Key, Value> KeyValueMap;

        // An index into the LOUDS bit-vector. This value is only used internally.
        typedef int BitIndex;

        using NodeId = LoudsNodeId;

        static bool IsValidId(NodeId node_id) { return node_id != kInvalidId; }

        // Creates a LoudsTrie from the provided MarisaMapper. This will sequentially
        // memory map the contents from the mapper.
        static std::unique_ptr<LoudsTrie<T, V, K>> CreateFromMapperOrNull(
                MarisaMapper* mapper) {
            std::unique_ptr<LoudsTrie<T, V, K>> trie(new LoudsTrie<T, V, K>(true));
            if (!trie->MapFromMapper(mapper)) {
                return nullptr;
            } else {
                return trie;
            }
        }

        // Creates a LoudsTrie from the provided MarisaReader. This will sequentially
        // load the contents from the reader. Returns null if the initialization
        // failed.
        static std::unique_ptr<LoudsTrie<T, V, K>> CreateFromReaderOrNull(
                MarisaReader* reader) {
            std::unique_ptr<LoudsTrie<T, V, K>> trie(new LoudsTrie<T, V, K>(true));
            if (!trie->ReadFromReader(reader)) {
                return nullptr;
            } else {
                return trie;
            }
        }

        // Creates a LoudsTrie by reading the input file. Returns null if the
        // initialization failed.
        static std::unique_ptr<LoudsTrie<T, V, K>> CreateFromFileOrNull(
                const string& filename) {
            std::unique_ptr<LoudsTrie<T, V, K>> trie(new LoudsTrie<T, V, K>(true));
            if (!trie->ReadFromFile(filename)) {
                return nullptr;
            } else {
                return trie;
            }
        }

        // Creates a LoudsTrie by memory mapping provided file. Returns null if the
        // initialization failed.
        static std::unique_ptr<LoudsTrie<T, V, K>> CreateFromMappedFileOrNull(
                const string& filename) {
            std::unique_ptr<LoudsTrie<T, V, K>> trie(new LoudsTrie<T, V, K>(true));
            if (!trie->MapFromFile(filename)) {
                return nullptr;
            } else {
                return trie;
            }
        }

        // Creates a new LoudsTrie from the provide KeyValueMap. Returns null if the
        // initialization failed.
        //
        // Args:
        //   key_value_map:
        //     The key to value map representing the contents of the trie.
        //   has_explicit_terminals:
        //     Whether the LOUDS trie contains explicit terminal nodes, which are the
        //     only nodes that have values (e.g., for a lexicon). If not, all nodes in
        //     the tree are implicitly treated as terminals (e.g., for an n-gram
        //     trie). In this case, there should be a key/value associated with each
        //     possible key prefix when building the trie.
        static std::unique_ptr<LoudsTrie<T, V, K>> CreateFromKeyValueMapOrNull(
                const KeyValueMap& key_value_map, const bool has_explicit_terminals) {
            std::unique_ptr<LoudsTrie<T, V, K>> trie(
                    new LoudsTrie<T, V, K>(has_explicit_terminals));
            if (!trie->Build(key_value_map)) {
                return nullptr;
            } else {
                return trie;
            }
        }

        virtual ~LoudsTrie() {}

        // Returns the node id of the root node.
        LoudsNodeId GetRootNodeId() const { return kRootNodeId; }

        // Returns the terminal ID of key if it's in the trie, -1 otherwise
        LoudsTerminalId KeyToTerminalId(const Key& key) const {
            const LoudsNodeId node_id = KeyToNodeId(key);
            return NodeIdToTerminalId(node_id);
        }

        // Returns the internal node ID of key if it's in the trie, -1 otherwise.
        // Performs a binary search (assuming the child list is sorted).
        LoudsNodeId KeyToNodeId(const Key& key) const;

        // Returns the value for the given key. Returns false if the key is not in
        // the trie.
        bool KeyToValue(const Key& key, Value* value) const;

        // Returns the value for the given terminal id.
        Value TerminalIdToValue(const LoudsTerminalId terminal_id) const {
            CHECK_LT(terminal_id, values_.size());
            return values_[terminal_id];
        }

        // Returns the terminal id for the given node id.
        LoudsTerminalId NodeIdToTerminalId(const LoudsNodeId node_id) const {
            if (!has_explicit_terminals_) {
                // If there are no explicit terminals, use the NodeId as the TerminalId
                // (but subtract 1 for the root, which is not considered a terminal).
                return node_id - 1;
            }
            if (node_id >= 0 && node_id < terminals_.size() && terminals_[node_id]) {
                return terminals_.rank1(node_id);
            }
            return kInvalidId;
        }

        // Returns the node id for the given terminal id.
        inline LoudsNodeId TerminalIdToNodeId(LoudsTerminalId terminal_id) const {
            if (!has_explicit_terminals_) {
                // If there are no explicit terminals, use the TerminalId as the NodeId.
                // (but add 1 for the root, which is not considered a terminal).
                return terminal_id + 1;
            }
            CHECK_LT(terminal_id, values_.size());
            return terminals_.select1(terminal_id);
        }

        // Returns the key for the given node id. This performs a sequential search
        // from this node up to the root of the trie.
        void NodeIdToKey(LoudsNodeId node_id, Key* key) const {
            key->clear();
            while (node_id != 0) {
                key->push_back(labels_[node_id]);
                node_id = NodeIdToParentNodeId(node_id);
            }
            std::reverse(key->begin(), key->end());
        }

        // Returns the node id reachable from the given parent node id and label.
        // Returns kInvalidId if the node does not have a child node for this label.
        LoudsNodeId FindChildNode(LoudsNodeId node_id, const T& label) const;

        // Retrieves the children (labels and node_ids) for the given parent node id.
        void GetChildren(const LoudsNodeId node_id, std::vector<T>* child_labels,
                         std::vector<LoudsNodeId>* child_node_ids) const;

        // Returns whether the node referenced by node_id has any children.
        bool HasChildren(const LoudsNodeId node_id) const;

        // Returns whether the given node id is a terminal.
        inline bool HasTerminalId(const LoudsNodeId node_id) const {
            if (!has_explicit_terminals_) {
                return true;
            }
            CHECK_LT(node_id, terminals_.size());
            return terminals_[node_id];
        }

        // Writes the contents of the trie to a POSIX file.
        void WriteToFile(const string& filename) const;

        // Writes the contents of the trie using the provided writer.
        void WriteToWriter(MarisaWriter* writer) const {
            louds_.write(writer);
            labels_.write(writer);
            terminals_.write(writer);
            values_.write(writer);
            writer->write(has_explicit_terminals_);
        }

    private:
        // Private constructor for LoudsTrie.
        //
        // Args:
        //   has_explicit_terminals:
        //     See comment for LoudsTrie::CreateFromKeyValueMapOrNull.
        explicit LoudsTrie(bool has_explicit_terminals)
                : has_explicit_terminals_(has_explicit_terminals) {}

        // Private method to build the LOUDS trie with the given key/value pairs.
        // This method is only called from the static factory method
        // CreateFromKeyValueMapOrNull.
        bool Build(const KeyValueMap& key_value_map);

        // Returns whether or not key1 and key2 share the same prefix of the given
        // length len.
        static bool PrefixEquals(const Key& key1, const Key& key2, const size_t len) {
            if (key1.size() < len || key2.size() < len) {
                return false;
            }
            for (size_t i = 0; i < len; ++i) {
                if (key1[i] != key2[i]) {
                    return false;
                }
            }
            return true;
        }

        // Returns the NodeId of the given bit index.
        // The bit at bit_index must be 1, which represents an edge to a child in the
        // parent node's degree sequence. The node id is defined as the rank1 of this
        // index (e.g., the number of 1s up to and including this index).
        inline LoudsNodeId BitIndexToNodeId(BitIndex bit_index) const {
            CHECK(louds_[bit_index]);
            return louds_.rank1(bit_index);
        }

        // Returns the bit index corresponding to the node's first edge.
        inline BitIndex NodeIdToFirstEdgeBitIndex(LoudsNodeId node_id) const {
            return louds_.select0(node_id) + 1;
        }

        // Returns the bit index corresponding to the node's first edge.
        // (e.g., it's first child)
        inline BitIndex NodeIdToLastEdgeBitIndex(LoudsNodeId node_id) const {
            return louds_.select0(node_id + 1) - 1;
        }

        // Returns the parent node id for the given node.
        inline LoudsNodeId NodeIdToParentNodeId(LoudsNodeId node_id) const {
            return louds_.select1(node_id) - node_id - 1;
        }

        // Memory maps the contents of the trie from a POSIX file.
        // Returns whether or not the initialization was completed successfully.
        bool MapFromFile(const string& filename);

        // Loads the contents of the trie from the POSIX file.
        // Returns whether or not the initialization was completed successfully.
        bool ReadFromFile(const string& filename);

        // Memory maps the contents of the trie from the provided mapper.
        // Returns whether or not the initialization was completed successfully.
        bool MapFromMapper(MarisaMapper* mapper) {
            louds_.map(mapper);
            labels_.map(mapper);
            terminals_.map(mapper);
            values_.map(mapper);
            mapper->map(&has_explicit_terminals_);
            return true;
        }

        // Loads the contents of the trie from the provided reader.
        // Returns whether or not the initialization was completed successfully.
        bool ReadFromReader(MarisaReader* reader) {
            louds_.read(reader);
            labels_.read(reader);
            terminals_.read(reader);
            values_.read(reader);
            reader->read(&has_explicit_terminals_);
            return true;
        }

        // Whether to trie has explicit terminals.
        bool has_explicit_terminals_;

        // The bit-vector encodes the structure of the trie (see file comment).
        MarisaBitVector louds_;

        // This bit-vector encodes the locations of terminals in the trie, referenced
        // by node id.
        MarisaBitVector terminals_;

        // The labels for each node in the trie, referenced by node id.
        MarisaVector<T> labels_;

        // The labels for the terminals in the trie, referenced by terminal id.
        MarisaVector<Value> values_;

        DISALLOW_COPY_AND_ASSIGN(LoudsTrie);
    };

    template <typename T, typename V, typename K>
    bool LoudsTrie<T, V, K>::Build(const KeyValueMap& key_values) {
        KeyValueMap remaining_key_values(key_values.begin(), key_values.end());

        // Push root.
        louds_.push_back(1);
        louds_.push_back(0);
        labels_.push_back(T(0));
        if (has_explicit_terminals_) {
            terminals_.push_back(0);
        }

        // Construct the LOUDS values for the given level, beginning at 0.
        for (size_t level = 0; !remaining_key_values.empty(); ++level) {
            Key prevkey;
            for (const auto& key_value : remaining_key_values) {
                const auto& key = key_value.first;
                const auto& value = key_value.second;
                if (prevkey.size() > 0 && !PrefixEquals(prevkey, key, level)) {
                    // If the parent node prefix has changed, then the previous key was
                    // the last child in its parent's degree sequence. Add the ending 0 to
                    // the louds bit-vector. E.g., t[h]i -> t[i]n
                    louds_.push_back(0);
                }
                if (key.size() > level) {
                    const bool prefix_changed =
                            prevkey.size() == 0 || !PrefixEquals(key, prevkey, level + 1);
                    if (prefix_changed) {
                        // If the child node prefix has changed, we have entered a new child
                        // node. Add the corresponding 1 to the LODUS bit-vector.
                        // E.g., t[h]e -> t[h]i
                        louds_.push_back(1);
                        labels_.push_back(key[level]);

                        const bool end_of_key = key.size() == level + 1;
                        if (end_of_key) {
                            // This is a terminal node.
                            if (has_explicit_terminals_) {
                                terminals_.push_back(1);
                            }
                            values_.push_back(value);
                        } else if (has_explicit_terminals_) {
                            // This is not a terminal node.
                            terminals_.push_back(0);
                        }
                    }
                }
                prevkey = key;
            }
            louds_.push_back(0);

            // Remove all terminal keys.
            auto key_iter = remaining_key_values.begin();
            while (key_iter != remaining_key_values.end()) {
                if (key_iter->first.size() <= level) {
                    remaining_key_values.erase(key_iter++);
                } else {
                    ++key_iter;
                }
            }
        }
        louds_.build();
        if (has_explicit_terminals_) {
            terminals_.build();
        } else {
            // If there are no explicit terminals, check to make sure each node has
            // an associated value.
            const int max_node_id = louds_.rank1(louds_.size() - 1);
            if (values_.size() != max_node_id - 1) {
//                LOG(ERROR) << "LoudsTrie build error: has_explicit_terminals is false, "
//                        "so all (internal and leaf) nodes in the trie must have an "
//                        "associated value.";
                return false;
            }
        }
        return true;
    }

    template <typename T, typename V, typename K>
    LoudsNodeId LoudsTrie<T, V, K>::KeyToNodeId(const Key& key) const {
        LoudsNodeId node_id = GetRootNodeId();
        for (auto key_iter = key.begin();
             key_iter != key.end() && node_id != kInvalidId; ++key_iter) {
            node_id = FindChildNode(node_id, *key_iter);
        }
        return node_id;
    }

    template <typename T, typename V, typename K>
    LoudsNodeId LoudsTrie<T, V, K>::FindChildNode(LoudsNodeId node_id,
                                                  const T& label) const {
        // bit index of first child for node_id
        BitIndex min_index = NodeIdToFirstEdgeBitIndex(node_id);
        if (!louds_[min_index]) return kInvalidId;
        // bit index of last child for node_id
        BitIndex max_index = NodeIdToLastEdgeBitIndex(node_id);
        if (!louds_[max_index]) return kInvalidId;
        // update node_id to that of the first child.
        node_id = BitIndexToNodeId(min_index);
        // calculate offset to convert from bit_index to node_id.
        // (only valid in the current child bit-vector.)
        const int bit_index_to_node_id_offset = node_id - min_index;
        while (true) {
            if (max_index < min_index) {
                return kInvalidId;
            }
            BitIndex mid_index = (min_index + max_index) / 2;
            // Update the node_id. Because of the LOUDS representation, all the
            // child bits are consecutive 1s. So we can traverse this child sequence
            // by incrementing/decrementing both node_id and bit_index.
            node_id = mid_index + bit_index_to_node_id_offset;
            const T mid_label = labels_[node_id];
            if (mid_label > label) {
                // key is in lower subset.
                max_index = mid_index - 1;
            } else if (mid_label < label) {
                // key is in upper subset.
                min_index = mid_index + 1;
            } else {
                // key is found.
                break;
            }
        }
        return node_id;
    }

    template <typename T, typename V, typename K>
    bool LoudsTrie<T, V, K>::KeyToValue(const Key& key, Value* value) const {
        const LoudsTerminalId terminal_id = KeyToTerminalId(key);
        if (terminal_id >= 0) {
            *value = values_[terminal_id];
            return true;
        }
        return false;
    }

    template <typename T, typename V, typename K>
    void LoudsTrie<T, V, K>::GetChildren(
            const LoudsNodeId node_id, vector<T>* child_labels,
            std::vector<LoudsNodeId>* child_node_ids) const {
        BitIndex bit_index = NodeIdToFirstEdgeBitIndex(node_id);
        if (!louds_[bit_index]) {
            return;
        }
        LoudsNodeId child_node_id = BitIndexToNodeId(bit_index);
        if (child_node_id >= labels_.size()) {
            return;
        }
        while (louds_[bit_index]) {
            child_labels->push_back(labels_[child_node_id]);
            child_node_ids->push_back(child_node_id);
            ++child_node_id;
            ++bit_index;
        }
    }

    template <typename T, typename V, typename K>
    bool LoudsTrie<T, V, K>::HasChildren(const LoudsNodeId node_id) const {
        BitIndex bit_index = NodeIdToFirstEdgeBitIndex(node_id);
        if (!louds_[bit_index]) {
            return false;
        }
        LoudsNodeId child_node_id = BitIndexToNodeId(bit_index);
        if (child_node_id >= labels_.size()) {
            return false;
        }
        return true;
    }

    template <typename T, typename V, typename K>
    void LoudsTrie<T, V, K>::WriteToFile(const string& filename) const {
        MarisaWriter writer;
        writer.open(filename.c_str());
        WriteToWriter(&writer);
    }

    template <typename T, typename V, typename K>
    bool LoudsTrie<T, V, K>::ReadFromFile(const string& filename) {
        MarisaReader reader;
        if (!reader.open(filename.c_str())) {
            return false;
        }
        ReadFromReader(&reader);
        return true;
    }

    template <typename T, typename V, typename K>
    bool LoudsTrie<T, V, K>::MapFromFile(const string& filename) {
        int fd = 0;
        fd = ::open(filename.c_str(), O_RDONLY);
        if (fd < 0) {
            return false;
        }

        struct stat file_stat;
        fstat(fd, &file_stat);
        const int length = file_stat.st_size;

        void* map = mmap(0, length, PROT_READ, MAP_SHARED, fd, 0);

        MarisaMapper mapper;
        mapper.open(map, length);
        MapFromMapper(&mapper);
        return true;
    }

}  // namespace louds
}  // namespace lm
}  // namespace keyboard

#endif  // INPUTMETHOD_KEYBOARD_LM_LOUDS_LOUDS_TRIE_H_
