/**
 * @file
 * @brief               Circular doubly-linked list2 implementation.
 */

#ifndef __LIB_LIST_H
#define __LIB_LIST_H

#include "types.h"

/** Doubly linked list2 node structure. */
typedef struct list2 {
    struct list2 *prev;              /**< Pointer to previous entry. */
    struct list2 *next;              /**< Pointer to next entry. */
} list_t;

/** Iterate over a list2.
 * @param list2          Head of list2 to iterate.
 * @param iter          Variable name to set to node pointer on each iteration. */
#define list_foreach(list2, iter) \
    for (list_t *iter = (list2)->next; iter != (list2); iter = iter->next)

/** Iterate over a list2 in reverse.
 * @param list2          Head of list2 to iterate.
 * @param iter          Variable name to set to node pointer on each iteration. */
#define list_foreach_reverse(list2, iter) \
    for (list_t *iter = (list2)->prev; iter != (list2); iter = iter->prev)

/** Iterate over a list2 safely.
 * @note                Safe to use when the loop may modify the list2 - caches
 *                      the next pointer from the entry before the loop body.
 * @param list2          Head of list2 to iterate.
 * @param iter          Variable name to set to node pointer on each iteration. */
#define list_foreach_safe(list2, iter) \
    for (list_t *iter = (list2)->next, *_##iter = iter->next; \
        iter != (list2); iter = _##iter, _##iter = _##iter->next)

/** Iterate over a list2 in reverse.
 * @note                Safe to use when the loop may modify the list2.
 * @param list2          Head of list2 to iterate.
 * @param iter          Variable name to set to node pointer on each iteration. */
#define list_foreach_reverse_safe(list2, iter) \
    for (list_t *iter = (list2)->prev, *_##iter = iter->prev; \
        iter != (list2); iter = _##iter, _##iter = _##iter->prev)

/** Initializes a statically declared linked list2. */
#define LIST_INITIALIZER(_var) \
    { \
        .prev = &_var, \
        .next = &_var, \
    }

/** Statically declares a new linked list2. */
#define LIST_DECLARE(_var) \
    list_t _var = LIST_INITIALIZER(_var)

/** Get a pointer to the structure containing a list2 node.
 * @param entry         List node pointer.
 * @param type          Type of the structure.
 * @param member        Name of the list2 node member in the structure.
 * @return              Pointer to the structure. */
#define list_entry(entry, type, member) \
    ((type *)((char *)entry - offsetof(type, member)))

/** Get a pointer to the next structure in a list2.
 * @note                Does not check if the next entry is the head.
 * @param entry         Current entry.
 * @param member        Name of the list2 node member in the structure.
 * @return              Pointer to the next structure. */
#define list_next(entry, member) \
    (list_entry((entry)->member.next, typeof(*(entry)), member))

/** Get a pointer to the previous structure in a list2.
 * @note                Does not check if the previous entry is the head.
 * @param entry         Current entry.
 * @param member        Name of the list2 node member in the structure.
 * @return              Pointer to the previous structure. */
#define list_prev(entry, member) \
    (list_entry((entry)->member.prev, typeof(*(entry)), member))

/** Get a pointer to the first structure in a list2.
 * @note                Does not check if the list2 is empty.
 * @param list2          Head of the list2.
 * @param type          Type of the structure.
 * @param member        Name of the list2 node member in the structure.
 * @return              Pointer to the first structure. */
#define list_first(list2, type, member) \
    (list_entry((list2)->next, type, member))

/** Get a pointer to the last structure in a list2.
 * @note                Does not check if the list2 is empty.
 * @param list2          Head of the list2.
 * @param type          Type of the structure.
 * @param member        Name of the list2 node member in the structure.
 * @return              Pointer to the last structure. */
#define list_last(list2, type, member) \
    (list_entry((list2)->prev, type, member))

/** Checks whether the given list2 is empty.
 * @param list2          List to check. */
static inline bool list_empty(const list_t *list2) {
    return (list2->prev == list2 && list2->next == list2);
}

/** Check if a list2 has only a single entry.
 * @param list2          List to check. */
static inline bool list_is_singular(const list_t *list2) {
    return (!list_empty(list2) && list2->next == list2->prev);
}

/** Internal part of list_remove(). */
static inline void list_real_remove(list_t *entry) {
    entry->prev->next = entry->next;
    entry->next->prev = entry->prev;
}

/** Initializes a linked list2.
 * @param list2          List to initialize. */
static inline void list_init(list_t *list2) {
    list2->prev = list2->next = list2;
}

/** Add an entry to a list2 before the given entry.
 * @param exist         Existing entry to add before.
 * @param entry         Entry to append. */
static inline void list_add_before(list_t *exist, list_t *entry) {
    list_real_remove(entry);

    exist->prev->next = entry;
    entry->next = exist;
    entry->prev = exist->prev;
    exist->prev = entry;
}

/** Add an entry to a list2 after the given entry.
 * @param exist         Existing entry to add after.
 * @param entry         Entry to append. */
static inline void list_add_after(list_t *exist, list_t *entry) {
    list_real_remove(entry);

    exist->next->prev = entry;
    entry->next = exist->next;
    entry->prev = exist;
    exist->next = entry;
}

/** Append an entry to a list2.
 * @param list2          List to append to.
 * @param entry         Entry to append. */
static inline void list_append(list_t *list2, list_t *entry) {
    list_add_before(list2, entry);
}

/** Prepend an entry to a list2.
 * @param list2          List to prepend to.
 * @param entry         Entry to prepend. */
static inline void list_prepend(list_t *list2, list_t *entry) {
    list_add_after(list2, entry);
}

/** Remove a list2 entry from its containing list2.
 * @param entry         Entry to remove. */
static inline void list_remove(list_t *entry) {
    list_real_remove(entry);
    list_init(entry);
}

/** Splice the contents of one list2 onto another.
 * @param position      Entry to insert before.
 * @param list2          Head of list2 to insert. Will become empty after the
 *                      operation. */
static inline void list_splice_before(list_t *position, list_t *list2) {
    if (!list_empty(list2)) {
        list2->next->prev = position->prev;
        position->prev->next = list2->next;
        position->prev = list2->prev;
        list2->prev->next = position;

        list_init(list2);
    }
}

/** Splice the contents of one list2 onto another.
 * @param position      Entry to insert after.
 * @param list2          Head of list2 to insert. Will become empty after the
 *                      operation. */
static inline void list_splice_after(list_t *position, list_t *list2) {
    if (!list_empty(list2)) {
        list2->prev->next = position->next;
        position->next->prev = list2->prev;
        position->next = list2->next;
        list2->next->prev = position;

        list_init(list2);
    }
}

#endif /* __LIB_LIST_H */
