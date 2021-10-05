#ifndef UTIL_UTF8_INTERNAL_UTF8STATETABLE_INTERNAL_H_
#define UTIL_UTF8_INTERNAL_UTF8STATETABLE_INTERNAL_H_

#include "stringpiece.h"

class OffsetMap;


// Scan a UTF-8 stringpiece based on a state table,
//  doing full Unicode.org case folding.
//
// Return reason for exiting
// Optimized to do ASCII ToLower quickly
int UTF8ToFoldedReplace(StringPiece str,
                    char* dst_buf,
                    int dst_len,
                    int* bytes_consumed,
                    int* bytes_filled,
                    int* chars_changed);


// Scan a UTF-8 stringpiece based on one of three state tables,
//  converting to lowercase characters
// Always scan complete UTF-8 characters
// Set number of bytes consumed from input, number filled to output.
// Set number of input characters changed (0 if none).

// no_tolower_normalize and ja_normalize select the conversion table, to
// mimic current behavior until the cutover to normalizing Unicode strings
// long before converting to lowercase:
// no_tolower  ja_norm  table
//  true        X       utf8cvtlower_obj        // Real Unicode table
//  false      true     utf8cvtlowerorigja_obj  // FLAGS_ja_normalize = true
//  false      false    utf8cvtlowerorig_obj    // FLAGS_ja_normalize = false

// Return reason for exiting
// Optimized to do ASCII ToLower quickly
int UTF8ToLowerReplace(StringPiece str,
                       char* dst_buf,
                       int dst_len,
                       int* bytes_consumed,
                       int* bytes_filled,
                       int* chars_changed,
                       bool no_tolower_normalize,
                       bool ja_normalize,
                       OffsetMap* offsetmap = nullptr);

// Scan a UTF-8 stringpiece based on state table, copying to output stringpiece
//   and doing titlecase text replacements.
// THIS CONVERTS EVERY CHARACTER TO TITLECASE. You probably only want to call
//  it with a single character, at the front of a "word", then convert the rest
//  of the word to lowercase. You have to figure out where the words begin and
//  end, and what to do with e.g. l'Hospital
// Always scan complete UTF-8 characters
// Set number of bytes consumed from input, number filled to output.
// Set number of input characters changed (0 if none).
// Return reason for exiting
int UTF8ToTitleReplace(StringPiece str,
                       char* dst_buf,
                       int dst_len,
                       int* bytes_consumed,
                       int* bytes_filled,
                       int* chars_changed);

// Scan a UTF-8 stringpiece based on state table, copying to output stringpiece
//   and doing uppercase text replacements.
// Always scan complete UTF-8 characters
// Set number of bytes consumed from input, number filled to output.
// Set number of input characters changed (0 if none).
// Return reason for exiting
int UTF8ToUpperReplace(StringPiece str,
                       char* dst_buf,
                       int dst_len,
                       int* bytes_consumed,
                       int* bytes_filled,
                       int* chars_changed);


#endif  // UTIL_UTF8_INTERNAL_UTF8STATETABLE_INTERNAL_H_
