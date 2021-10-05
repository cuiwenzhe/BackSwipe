#include "constants.h"

#include "../base/logging.h"

namespace keyboard {
namespace lm {

    //bool IsReservedTerm(const StringPiece& term) {
    bool IsReservedTerm(const StringPiece& term) {
        return (term == kUnk || term == kBOS || term == kEOS || term == kNone);
    }

    //TermId ReservedTermToTermId(const StringPiece& term) {
    TermId ReservedTermToTermId(const StringPiece& term) {
        if (term == kUnk) {
            return kUnkId;
        } else if (term == kBOS) {
            return kBOSId;
        } else if (term == kEOS) {
            return kEOSId;
        } else if (term == kNone) {
            return kNoneId;
        }
        return kFirstUnreservedId;
    }

    std::string ReservedTermIdToTerm(const TermId termid) {
        if (termid == kUnkId) {
            return kUnk;
        } else if (termid == kBOSId) {
            return kBOS;
        } else if (termid == kEOSId) {
            return kEOS;
        } else if (termid == kNoneId) {
            return kNone;
        }
        LOG(ERROR) << "Not a reserved termid: " << termid;
        return "";
    }

}  // namespace lm
}  // namespace keyboard
