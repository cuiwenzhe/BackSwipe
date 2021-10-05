//
// Copyright 2007 Google Inc.  All Rights Reserved.
// Author: dsites@google.com (Dick Sites)
//
// Design document: eng/designdocs/util/utf8/offsetmap.pdf
//

#include "offsetmap.h"

#include <string.h>                     // for strcmp
#include <algorithm>                    // for min
#include <cstdio>                       // for fprintf, stderr, fclose, etc

#include "logging.h"               // for FLAGS_v, operator<<, etc
#include "port.h"                  // for PRIuS
#include "stringprintf.h"          // for StringAppendF
#include "unilib_utf8_utils.h"  // for OneCharLen, IsTrailByte

// Constructor, destructor
OffsetMap::OffsetMap() {
    //if (FLAGS_v > 2) {fprintf(stderr, "New ");}
    Clear();
}

OffsetMap::~OffsetMap() {
}

// Clear the map
void OffsetMap::Clear() {
    diffs_.clear();
    pending_op_ = COPY_OP;
    pending_length_ = 0;
    next_diff_sub_ = 0;
    current_lo_aoffset_ = 0;
    current_hi_aoffset_ = 0;
    current_lo_aprimeoffset_ = 0;
    current_hi_aprimeoffset_ = 0;
    current_diff_ = 0;
}

static inline char OpPart(const char c) {
    return (c >> 6) & 3;
}
static inline char LenPart(const char c) {
    return c & 0x3f;
}

// Print map to file, for debugging
void OffsetMap::Printmap(const char* filename) {
    FILE* fout;
    bool needs_close = false;
    if (strcmp(filename, "stdout") == 0) {
        fout = stdout;
    } else if (strcmp(filename, "stderr") == 0) {
        fout = stderr;
    } else {
        fout = fopen(filename, "w");
        needs_close = true;
    }
    if (fout == nullptr) {
        fprintf(stderr, "%s did not open\n", filename);
        return;
    }

    Flush();    // Make sure any pending entry gets printed
    fprintf(fout, "Offsetmap: %" PRIuS " bytes\n", diffs_.size());
    for (int i = 0; i < diffs_.size(); ++i) {
        fprintf(fout, "%c%02d ", "&=+-"[OpPart(diffs_[i])], LenPart(diffs_[i]));
        if ((i % 20) == 19) {fprintf(fout, "\n");}
    }
    fprintf(fout, "\n");
    if (needs_close) {
        fclose(fout);
    }
}

string OffsetMap::DebugString() const {
    string output;
    base::StringAppendF(&output,  "{Offsetmap: %" PRIuS " bytes ", diffs_.size());
    for (int i = 0; i < diffs_.size(); ++i) {
        base::StringAppendF(&output,
                      "%c%02d ",
                      "&=+-"[OpPart(diffs_[i])],
                      LenPart(diffs_[i]));
    }
    base::StringAppendF(&output, "}");
    return output;
}


// Reset to offset 0
void OffsetMap::Reset() {
    MaybeFlushAll();

    next_diff_sub_ = 0;
    current_lo_aoffset_ = 0;
    current_hi_aoffset_ = 0;
    current_lo_aprimeoffset_ = 0;
    current_hi_aprimeoffset_ = 0;
    current_diff_ = 0;
}

// Add to  mapping from A to A', specifying how many next bytes are
// identical in A and A'
void OffsetMap::Copy(int bytes) {
    //if (FLAGS_v > 2) {fprintf(stderr, "Copy(%u) ", bytes);}
    if (bytes == 0) {return;}
    if (pending_op_ == COPY_OP) {
        pending_length_ += bytes;
    } else {
        Flush();
        pending_op_ = COPY_OP;
        pending_length_ = bytes;
    }
}

// Add to mapping from A to A', specifying how many next bytes are
// inserted in A' while not advancing in A at all
void OffsetMap::Insert(int bytes){
    //if (FLAGS_v > 2) {fprintf(stderr, "Insert(%u) ", bytes);}
    if (bytes == 0) {return;}
    if (pending_op_ == INSERT_OP) {
        pending_length_ += bytes;
    } else {
        Flush();
        pending_op_ = INSERT_OP;
        pending_length_ = bytes;
    }
}

// Add to mapping from A to A', specifying how many next bytes are
// deleted from A while not advancing in A' at all
void OffsetMap::Delete(int bytes){
    //if (FLAGS_v > 2) {fprintf(stderr, "Delete(%u) ", bytes);}
    if (bytes == 0) {return;}
    if (pending_op_ == DELETE_OP) {
        pending_length_ += bytes;
    } else {
        Flush();
        pending_op_ = DELETE_OP;
        pending_length_ = bytes;
    }
}

// Map an offset in A' to the corresponding offset in A
// This call can return an offset in the middle of a multi-byte UTF-8
// character. Use the Aligned version below if you want to avoid this.
int OffsetMap::MapBack(int aprimeoffset){
    //if (FLAGS_v > 2) {fprintf(stderr, "MapBack(%d) ", aprimeoffset);}
    MaybeFlushAll();
    while (aprimeoffset < current_lo_aprimeoffset_) {
        MoveLeft();
    }
    while (current_hi_aprimeoffset_ <= aprimeoffset) {
        MoveRight();
    }

    int aoffset = aprimeoffset - current_diff_;
    if (aoffset >= current_hi_aoffset_) {
        // A' is in an insert region, all bytes of which backmap to A=hi_aoffset_
        aoffset = current_hi_aoffset_;
    }
    //if (FLAGS_v > 2) {fprintf(stderr, "= %d\n", aoffset);}
    return aoffset;
}

// Map an offset in A to the corresponding offset in A'
// This call can return an offset in the middle of a multi-byte UTF-8
// character. Use the Aligned version below if you want to avoid this.
int OffsetMap::MapForward(int aoffset){
    //if (FLAGS_v > 2) {fprintf(stderr, "MapForward(%d) ", aoffset);}
    MaybeFlushAll();
    while (aoffset < current_lo_aoffset_) {
        MoveLeft();
    }
    while (current_hi_aoffset_ <= aoffset) {
        MoveRight();
    }

    int aprimeoffset = aoffset + current_diff_;
    if (aprimeoffset >= current_hi_aprimeoffset_) {
        // A is in a delete region, all bytes of which map to A'=hi_aprimeoffset_
        aprimeoffset = current_hi_aprimeoffset_;
    }
    //if (FLAGS_v > 2) {fprintf(stderr, "= %d\n", aprimeoffset);}
    return aprimeoffset;
}

namespace {
    const char* FindLastCharBoundary(const char* start, const char* end) {
        // Back up, if necessary, from 'end' to a UTF-8 character boundary.
        // We can't safely look at the byte that 'end' points to, since that
        // might be past the end of the string, but we can look at previous
        // bytes.
        const char* ptr = end;
        while (ptr > start) {
            if (UniLib::IsTrailByte(*--ptr)) {
                // Just keep backing up.
            } else if (ptr + UniLib::OneCharLen(ptr) == end) {
                // Normal case: 'end' is the next character boundary.
                return end;
            } else {
                // If the next character boundary is past 'end', then 'end' was
                // in the middle of a multibyte character.  If the next
                // character boundary is before 'end', then the string is
                // ill-formed: it has extra trailing bytes.  In either case,
                // return the current pointer, since we know it points to the
                // start of a UTF-8 character.
                return ptr;
            }
        }
        return start;
    }
}  // namespace

// Map an offset in A' to the corresponding offset in A and back up, if
// needed, to a UTF-8 character boundary in a_src
int OffsetMap::MapBackAligned(int aprimeoffset,
                              const char* a_src, const char* aprime_src) {
    int aoffset = MapBack(aprimeoffset);
    return FindLastCharBoundary(a_src, a_src + aoffset) - a_src;
}

// Map an offset in A to the corresponding offset in A' and back up, if
// needed, to a UTF-8 character boundary in aprime_src
int OffsetMap::MapForwardAligned(int aoffset,
                                 const char* a_src, const char* aprime_src) {
    int aprimeoffset = MapForward(aoffset);
    return FindLastCharBoundary(aprime_src, aprime_src + aprimeoffset) - aprime_src;
}

void OffsetMap::Flush() {
    if (pending_length_ == 0) {
        return;
    }
    if (pending_length_ > 0x3f) {
        bool non_zero_emitted = false;
        for (int shift = 30; shift > 0; shift -= 6) {
            int prefix = (pending_length_ >> shift) & 0x3f;
            if (non_zero_emitted) {
                Emit(PREFIX_OP, prefix);
            } else if (prefix > 0) {
                non_zero_emitted = true;
                Emit(PREFIX_OP, prefix);
            }
        }
    }
    Emit(pending_op_, pending_length_ & 0x3f);
    pending_length_ = 0;
}


// Add one more entry to copy one byte off the end, then flush
void OffsetMap::FlushAll() {
    Copy(1);
    Flush();

//    if (FLAGS_v > 2) {
//        fprintf(stderr, "FlushAll: ");
//        DumpString();
//    }
}

// Flush all if necessary
void OffsetMap::MaybeFlushAll() {
    if ((0 < pending_length_) || diffs_.empty()) {
        FlushAll();
    }
}

// Len may be 0, for example as the low piece of length=64
void OffsetMap::Emit(MapOp op, int len) {
    char c = (static_cast<char>(op) << 6) | (len & 0x3f);
    diffs_.push_back(c);
}

void OffsetMap::DumpString() {
    for (int i = 0; i < diffs_.size(); ++i) {
        fprintf(stderr, "%c%02d ", "&=+-"[OpPart(diffs_[i])], LenPart(diffs_[i]));
    }
    fprintf(stderr, "\n");
}

void OffsetMap::DumpWindow() {
    fprintf(stderr, "A  [%u..%u)\n", current_lo_aoffset_, current_hi_aoffset_);
    fprintf(stderr, "A' [%u..%u)\n", current_lo_aprimeoffset_,
            current_hi_aprimeoffset_);
    fprintf(stderr, "  diff = %d\n", current_diff_);
    DumpString();
    for (int i = 0; i <= diffs_.size(); ++i) {
        if (i == next_diff_sub_) {
            fprintf(stderr, "^^^ ");
        } else {
            fprintf(stderr, "    ");
        }
    }
    fprintf(stderr, "\n");
}

void OffsetMap::MoveRight() {
    // Move past current ranges
    current_diff_ += ((current_hi_aprimeoffset_ - current_lo_aprimeoffset_) -
                      (current_hi_aoffset_ - current_lo_aoffset_));
    current_lo_aoffset_ = current_hi_aoffset_;
    current_lo_aprimeoffset_ = current_hi_aprimeoffset_;

    // Check off end
    if (next_diff_sub_ >= diffs_.size()) {
        // Off the end; make a dummy copy range
        //LOG_EVERY_N_SEC(DFATAL, 1) << "OffsetMap::MoveRight() above offset max";
        current_hi_aoffset_ = std::numeric_limits<int>::max();
        current_hi_aprimeoffset_ = std::numeric_limits<int>::max();
//        if (FLAGS_v > 2) {
//            fprintf(stderr, "MoveRight\n");
//            DumpWindow();
//        }
        return;
    }

    // Update to next range
    char c = 0;
    MapOp op = COPY_OP;
    int length = 0;
    while (next_diff_sub_ < diffs_.size()) {
        c = diffs_[next_diff_sub_++];
        op = static_cast<MapOp>(OpPart(c));
        int len = LenPart(c);
        length = (length << 6) + len;
        if (op != PREFIX_OP) {
            break;
        }
    }
    if (op == COPY_OP) {
        current_hi_aoffset_ = current_lo_aoffset_ + length;
        current_hi_aprimeoffset_ = current_lo_aprimeoffset_ + length;
    } else if (op == INSERT_OP) {
        current_hi_aoffset_ = current_lo_aoffset_ + 0;
        current_hi_aprimeoffset_ = current_lo_aprimeoffset_ + length;
    } else if (op == DELETE_OP) {
        current_hi_aoffset_ = current_lo_aoffset_ + length;
        current_hi_aprimeoffset_ = current_lo_aprimeoffset_ + 0;
    } else {
        LOG(DFATAL) << "OffsetMap::MoveRight() unknown operator " <<
                    static_cast<int>(op);
    }
//    if (FLAGS_v > 2) {
//        fprintf(stderr, "MoveRight\n");
//        DumpWindow();
//    }
}

void OffsetMap::MoveLeft() {
    // Collapse current ranges
    current_hi_aoffset_ = current_lo_aoffset_;
    current_hi_aprimeoffset_ = current_lo_aprimeoffset_;

    // Back up over current operator
    --next_diff_sub_;
    while ((next_diff_sub_ != 0) &&
           (OpPart(diffs_[next_diff_sub_ - 1]) == PREFIX_OP)) {
        --next_diff_sub_;
    }

    // Check off end
    if (next_diff_sub_ == 0) {
        // Fatal error
        LOG(DFATAL) << "OffsetMap::MoveLeft() below offset 0";
        // Meanwhile, reinit
        next_diff_sub_ = 0;
        current_lo_aoffset_ = 0;
        current_hi_aoffset_ = 0;
        current_lo_aprimeoffset_ = 0;
        current_hi_aprimeoffset_ = 0;
        current_diff_ = 0;
        MoveRight();
        return;
    }

    // Update to previous range
    int save_sub = next_diff_sub_;
    char c = diffs_[--next_diff_sub_];
    MapOp op = static_cast<MapOp>(OpPart(c));
    int len = LenPart(c);
    int length = len;
    int k6 = 0;
    // Accumulate prefixes, if any
    while (next_diff_sub_ != 0) {
        char prior_c = diffs_[next_diff_sub_ - 1];
        MapOp prior_op = static_cast<MapOp>(OpPart(prior_c));
        if (prior_op == PREFIX_OP) {
            // Accumulate larger length
            k6 += 6;
            length += ((prior_c & 0x3f) << k6);
            --next_diff_sub_;
        } else {
            break;
        }
    }
    next_diff_sub_ = save_sub;
    if (op == COPY_OP) {
        current_lo_aoffset_ = current_hi_aoffset_ - length;
        current_lo_aprimeoffset_ = current_hi_aprimeoffset_ - length;
        current_diff_ -= 0;
    } else if (op == INSERT_OP) {
        current_lo_aoffset_ = current_hi_aoffset_ - 0;
        current_lo_aprimeoffset_ = current_hi_aprimeoffset_ - length;
        current_diff_ -= length;
    } else if (op == DELETE_OP) {
        current_lo_aoffset_ = current_hi_aoffset_ - length;
        current_lo_aprimeoffset_ = current_hi_aprimeoffset_ - 0;
        current_diff_ += length;
    } else {
        LOG(DFATAL) << "OffsetMap::MoveLeft() unknown operator " <<
                    static_cast<int>(op);
    }

//    if (FLAGS_v > 2) {
//        fprintf(stderr, "MoveLeft\n");
//        DumpWindow();
//    }
}

// static
bool OffsetMap::CopyInserts(OffsetMap* source, OffsetMap* dest) {
    while (source->next_diff_sub_ != source->diffs_.size()) {
        source->MoveRight();
        if (source->current_lo_aoffset_ != source->current_hi_aoffset_) {
            return false;
        }
        dest->Insert(
                source->current_hi_aprimeoffset_ - source->current_lo_aprimeoffset_);
    }
    return true;
}

// static
bool OffsetMap::CopyDeletes(OffsetMap* source, OffsetMap* dest) {
    while (source->next_diff_sub_ != source->diffs_.size()) {
        source->MoveRight();
        if (source->current_lo_aprimeoffset_ != source->current_hi_aprimeoffset_) {
            return false;
        }
        dest->Delete(source->current_hi_aoffset_ - source->current_lo_aoffset_);
    }
    return true;
}

// static
bool OffsetMap::ComposeOffsetMap(
        OffsetMap* g, OffsetMap* f, OffsetMap* h) {
    h->Clear();
    f->Reset();
    g->Reset();

    int lo = 0;
    for (;;) {
        // Consume delete operations in f. This moves A without moving
        // A' and A''.
        if (lo >= g->current_hi_aoffset_ && CopyInserts(g, h)) {
            if (!(lo >= f->current_hi_aprimeoffset_ && CopyDeletes(f, h))) {
                LOG(DFATAL) << "f is longer than g. f->current_hi_aprimeoffset_ ="
                            << f->current_hi_aprimeoffset_
                            << ", g->current_hi_aoffset_ = "
                            << g->current_hi_aoffset_;
                h->Clear();
                return false;  // Should not be reached.
            }
            // FlushAll(), called by Reset(), MapForward() or MapBack(), has
            // added an extra COPY_OP to f and g, so this function has
            // composed an extra COPY_OP in h from those. To avoid
            // FlushAll() adds one more extra COPY_OP to h later, dispatch
            // Flush() right now.
            h->Flush();
            return true;
        }

        // Consume insert operations in g. This moves A'' without moving A
        // and A'.
        if (lo >= f->current_hi_aprimeoffset_) {
            if (CopyDeletes(f, h)) {
                LOG(DFATAL) << "g is longer than f. f->current_hi_aprimeoffset_ = "
                            <<  f->current_hi_aprimeoffset_
                            <<  ", g->current_hi_aoffset_ = " << g->current_hi_aoffset_;
                h->Clear();
                return false;  // Should not be reached.
            }
        }

        // Compose one operation which moves A' from lo to hi.
        int hi = std::min(f->current_hi_aprimeoffset_, g->current_hi_aoffset_);
        if (f->current_lo_aoffset_ != f->current_hi_aoffset_ &&
            g->current_lo_aprimeoffset_ != g->current_hi_aprimeoffset_) {
            h->Copy(hi - lo);
        } else if (f->current_lo_aoffset_ != f->current_hi_aoffset_) {
            h->Delete(hi - lo);
        } else if (g->current_lo_aprimeoffset_ != g->current_hi_aprimeoffset_) {
            h->Insert(hi - lo);
        }

        lo = hi;
    }
    h->Clear();
    return false;  // Should not be reached.
}
