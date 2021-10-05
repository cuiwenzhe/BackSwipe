// Overwrites exception related macro definitions of Marisa lib
// This allows us to compile without the -fexceptions flag.

#ifndef INPUTMETHOD_KEYBOARD_LM_LOUDS_REMOVE_MARISA_EXCEPTIONS_H_
#define INPUTMETHOD_KEYBOARD_LM_LOUDS_REMOVE_MARISA_EXCEPTIONS_H_

#ifdef MARISA_EXCEPTION_H_
#error "This header was not included before MARISA_EXCEPTION_H_"
#else
#define MARISA_EXCEPTION_H_
#define MARISA_THROW(error_code, error_message)
#define MARISA_THROW_IF(condition, error_code)
#define MARISA_DEBUG_IF(cond, error_code)
#endif  // MARISA_EXCEPTION_H_

#endif  // INPUTMETHOD_KEYBOARD_LM_LOUDS_REMOVE_MARISA_EXCEPTIONS_H_
