//
// Copyright 2007 Google Inc.  All Rights Reserved.
// Author: dsites@google.com (Dick Sites)
//
// Design document: eng/designdocs/util/utf8/offsetmap.pdf

#ifndef UTIL_UTF8_PUBLIC_OFFSETMAP_H_
#define UTIL_UTF8_PUBLIC_OFFSETMAP_H_

#include <string>                       // for string

#include "integral_types.h"        // for uint32
#include "port.h"

// ***************************** OffsetMap **************************
//
// An OffsetMap object is a container for a mapping from offsets in one text
// buffer A' to offsets in another text buffer A. It is most useful when A' is
// built from A via substitutions that occasionally do not preserve byte length.
//
// A series of operators are used to build the correspondence map, then
// calls can be made to map an offset in A' to an offset in A, or vice versa.
// The map starts with offset 0 in A corresponding to offset 0 in A'.
// The mapping is then built sequentially, adding on byte ranges that are
// identical in A and A', byte ranges that are inserted in A', and byte ranges
// that are deleted from A. All bytes beyond those specified when building the
// map are assumed to correspond, i.e. a Copy(infinity) is assumed at the
// end of the map.
//
// The internal data structure records positions at which bytes are added or
// deleted. Using the map is O(1) when increasing the A' or A offset
// monotonically, and O(n) when accessing random offsets, where n is the
// number of differences.
//
//
// OffsetMap simply keeps track of byte offsets across insert/delete/replacement
// from an original string to a modified one. It reliably maps between the
// strings for the starting and ending+1 offsets of such changes, but not for
// the middles, for which various groups would like to provide varying behavior.
// Feel free to wrap whatever extra logic you want around the  basic offsetmap.
//
// The now-deprecated MapBack and MapForward routines could land in the middle
// of a UTF-8 character, potentially creating bad text. Use the MapBackAligned
// and MapForwardAligned routines instead.
//
// In particular, the underlying implementation of byte changes in
// utf8statetable.cc has no concept of UTF-8 characters -- it just inserts or
// deletes bytes. Replacements are optimized to make the minimal changes.
//
// In the first example below the byte d7 is not changed by the replacement,
// so it DOES NOT PARTICIPATE in the replacement: the actual replacement changes
// just 1 byte b0  to three bytes 95 d7 95. This optimization saves a noticeable
// amount of space in the all-Unicode lowercasing table, for example.
//
// Some examples of the internal workings:
//
// (a) ? [one 2-byte character] -> ?? [two 2-byte characters]
//     U+05F0  ->  U+05D5 U+05D5
//     HEBREW LIGATURE YIDDISH DOUBLE VAV to HEBREW LETTER VAV HEBREW LETTER VAV
//
// xx d7 b0 yy
// xx d7 95 d7 95 yy
//     0  1  2  3
// offsetmap maps xx <--> xx, d7 <--> d7, and yy <--> yy
// Mappings in the middle of the replacement are not specified.
// As it happens, the first 95 <--> b0, the second d7 -->yy, and the
// second 95 --> yy. Note that 95 and b0 are mid-character.
// The fixed routine MapBackAligned backscans to aligned UTF-8 boundaries:
// b0 --> d7 and 95 -->d7.
//
//
// (b)  [one 2-byte character] -> ae [two 1-byte character]
//     U+00E6  ->   U+0061 U+0065
//     LATIN SMALL LETTER AE to a e
//
// xx c3 a6 yy
// xx 61 65 yy
//     0   1
// offsetmap maps xx <--> xx,c3 <--> 61, and yy <--> yy
// Mappings in the middle of the replacement are not specified.
// As it happens, 65 <--> a6. Note that a6 is mid-character.
// The fixed routine MapBackAligned backscans to aligned UTF-8 boundaries:
// 65 --> c3.

class OffsetMap {
public:
    // Constructor, destructor
    OffsetMap();
    ~OffsetMap();

    // Clear the map
    void Clear();

    // Add to  mapping from A to A', specifying how many next bytes correspond
    // in A and A'
    void Copy(int bytes);

    // Add to mapping from A to A', specifying how many next bytes are
    // inserted in A' while not advancing in A at all
    void Insert(int bytes);

    // Add to mapping from A to A', specifying how many next bytes are
    // deleted from A while not advancing in A' at all
    void Delete(int bytes);

    // Print map to file, for debugging
    void Printmap(const char* filename);
    string DebugString() const;

    // [Finish building map,] Re-position to offset 0
    // This call is optional; MapForward and MapBack finish building the map
    // if necessary
    void Reset();

    // DEPRECATED
    // Map an offset in A' to the corresponding offset in A
    // This call can return an offset in the middle of a multi-byte UTF-8
    // character. Use the Aligned version below if you want to avoid this.
    int MapBack(int aprimeoffset);

    // DEPRECATED
    // Map an offset in A to the corresponding offset in A'
    // This call can return an offset in the middle of a multi-byte UTF-8
    // character. Use the Aligned version below if you want to avoid this.
    int MapForward(int aoffset);

    // Map an offset in A' to the corresponding offset in A and back up, if
    // needed, to a UTF-8 character boundary in a_src
    int MapBackAligned(int aprimeoffset,
                       const char* a_src, const char* aprime_src);

    // Map an offset in A to the corresponding offset in A' and back up, if
    // needed, to a UTF-8 character boundary in aprime_src
    int MapForwardAligned(int aoffset,
                          const char* a_src, const char* aprime_src);

    // h = ComposeOffsetMap(g, f), where f is a map from A to A', g is
    // from A' to A'' and h is from A to A''.
    //
    // Note that g->MoveForward(f->MoveForward(aoffset)) always equals
    // to h->MoveForward(aoffset), while
    // f->MoveBack(g->MoveBack(aprimeprimeoffset)) doesn't always equals
    // to h->MoveBack(aprimeprimeoffset). This happens when deletion in
    // f and insertion in g are at the same place.  For example,
    //
    // A    1   2   3   4
    //      ^   |  ^    ^
    //      |   | /     |  f
    //      v   vv      v
    // A'   1'  2'      3'
    //      ^   ^^      ^
    //      |   | \     |  g
    //      v   |  v    v
    // A''  1'' 2'' 3'' 4''
    //
    // results in:
    //
    // A    1   2   3   4
    //      ^   ^\  ^   ^
    //      |   | \ |   |  h
    //      v   |  vv   v
    // A''  1'' 2'' 3'' 4''
    //
    // 2'' is mapped 3 in the former figure, while 2'' is mapped to 2 in
    // the latter figure.
    // Returns false if the OffsetMaps f and g cannot be composed, true otherwise.
    // (We assume that f maps A to A' and that g maps A' to A''. If the target of
    // f is not the source of g, then f and g cannot be composed, and the value of
    // h is undefined. This is usually an error by the caller.)
    static bool ComposeOffsetMap(OffsetMap* g, OffsetMap* f, OffsetMap* h);

private:
    enum MapOp {PREFIX_OP, COPY_OP, INSERT_OP, DELETE_OP};

    void Flush();
    void FlushAll();
    void MaybeFlushAll();
    void Emit(MapOp op, int len);
    void MoveRight();
    void MoveLeft();
    void DumpString();
    void DumpWindow();

    // Copies insert operations from source to dest. Returns true if no
    // other operations are found.
    static bool CopyInserts(OffsetMap* source, OffsetMap* dest);

    // Copies delete operations from source to dest. Returns true if no other
    // operations are found.
    static bool CopyDeletes(OffsetMap* source, OffsetMap* dest);

    string diffs_;
    MapOp pending_op_;
    uint32 pending_length_;

    // Offsets in the ranges below correspond to each other, with A' = A + diff
    int next_diff_sub_;
    int current_lo_aoffset_;
    int current_hi_aoffset_;
    int current_lo_aprimeoffset_;
    int current_hi_aprimeoffset_;
    int current_diff_;
};

#endif  // UTIL_UTF8_PUBLIC_OFFSETMAP_H_
