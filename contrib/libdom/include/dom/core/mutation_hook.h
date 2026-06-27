#ifndef dom_core_mutation_hook_h_
#define dom_core_mutation_hook_h_

#include <dom/core/exceptions.h>
#include <dom/functypes.h>

struct dom_document;

dom_exception dom_document_set_mutation_hook(struct dom_document *doc,
    dom_mutation_hook hook, void *pw);

#endif
