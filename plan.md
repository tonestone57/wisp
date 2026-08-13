1. **Understand the spec:** `DOM_INVALID_CHARACTER_ERR` is raised if any of the imported names are invalid. This applies to nodes that have names that must be valid XML names, such as element names, attribute names, processing instruction targets, entity names, notation names, and document type names.
Wait, DOM3 Core spec says: "DOM_INVALID_CHARACTER_ERR: Raised if one of the imported names is not a valid XML name according to the XML version in use and the boolean parameter is `true`." - well, it says boolean parameter is `true` but it might be deep, anyway, we should validate the name.
2. **Implementation details in `_dom_document_import_node`:**
   We can inspect `((dom_node_internal *)node)->type`.
   For types:
   - `DOM_ELEMENT_NODE`
   - `DOM_ATTRIBUTE_NODE`
   - `DOM_ENTITY_REFERENCE_NODE`
   - `DOM_ENTITY_NODE`
   - `DOM_PROCESSING_INSTRUCTION_NODE`
   - `DOM_NOTATION_NODE`
   - `DOM_DOCUMENT_TYPE_NODE`
   We should check if `((dom_node_internal *)node)->name` is a valid name using `_dom_validate_name`.
   If it's not valid, return `DOM_INVALID_CHARACTER_ERR`.
   Wait! Some nodes like Document Type, Entity, Notation might have different requirements or might not be checked in other parts. But according to the `importNode` doc in libdom, `DOM_INVALID_CHARACTER_ERR if any of the names are invalid`.
   Wait, wait, it says "if any of the names are invalid". If `deep == true`, does it need to recursively validate all children's names? Yes, "any of the names". However, if we do a deep import, `dom_document_dup_node` calls `dom_document_import_node` recursively for all children! Thus, if we just validate the current node being imported, the recursive calls will handle the children! Let's verify `dom_document_dup_node`. It calls `dom_document_import_node` for children if `deep == true`. So we just need to validate the current node's name!

Let's check what nodes should have their names validated.
`_dom_document_create_element` validates `tag_name`.
`_dom_document_create_processing_instruction` validates `target` (which maps to `name`).
`_dom_document_create_attribute` validates `name`.
`_dom_document_create_entity_reference` validates `name`.

So the types that need validation are:
- `DOM_ELEMENT_NODE`
- `DOM_ATTRIBUTE_NODE`
- `DOM_PROCESSING_INSTRUCTION_NODE`
- `DOM_ENTITY_REFERENCE_NODE`
- `DOM_ENTITY_NODE` ? (wait, we can't create entities directly, but they might be imported)
- `DOM_NOTATION_NODE` ?
- `DOM_DOCUMENT_TYPE_NODE` ? (Usually document type cannot be imported, `importNode` for document type node throws `NOT_SUPPORTED_ERR` in many DOM implementations). Actually `dom_document_dup_node` returns `NOT_SUPPORTED_ERR` for `DOCUMENT_NODE` and `DOCUMENT_TYPE_NODE`. So they don't reach the recursive part, or they throw.

Let's check `dom_document_dup_node`:
```c
    if (n->type == DOM_DOCUMENT_NODE || n->type == DOM_DOCUMENT_TYPE_NODE)
        return DOM_NOT_SUPPORTED_ERR;
```
So we only need to worry about the others.
Let's validate `n->name` if the type is one of the types that have valid XML names:
```c
    dom_node_internal *n = (dom_node_internal *) node;

    if (n->type == DOM_ELEMENT_NODE ||
        n->type == DOM_ATTRIBUTE_NODE ||
        n->type == DOM_ENTITY_REFERENCE_NODE ||
        n->type == DOM_ENTITY_NODE ||
        n->type == DOM_PROCESSING_INSTRUCTION_NODE ||
        n->type == DOM_NOTATION_NODE) {
        if (n->name != NULL && _dom_validate_name(n->name) == false) {
            return DOM_INVALID_CHARACTER_ERR;
        }
    }
```
Wait, is there any node type that we are missing?
Wait, if it has a prefix or localName, we might also need to validate those? But `n->name` for these nodes is already constructed, and `_dom_validate_name` takes the full QName.
Is it `_dom_validate_name(n->name)` or do we need to check namespace validity? The error is `DOM_INVALID_CHARACTER_ERR`, which corresponds to `_dom_validate_name` failing.

Let's write a small patch and see if it passes tests.
