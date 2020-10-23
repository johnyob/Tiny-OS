////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// - Alistair O'Brien - 6/22/2020 - University of Cambridge
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      File:        list.c
//      Environment: Tiny OS
//      Description:
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <debug.h>

#include <lib/list.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CIRCULAR DOUBLY LINKED LISTS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tiny-OS' implementation of a circular doubly linked list uses a sentinel list node, referred to as nil.
// It is particularly useful when avoiding boundary conditions. Not only is the code simple, but no dynamic memory
// allocation is required (due to pointer magic :) )
//
// For example, an empty list looks like:
//
//                         |------|
//                        \/      |
//                      +-----+   |
//                      | nil |<--|
//                      +-----+
//
// A list with two elements looks like:
//
//             |---------------------------------|
//            \/                                 |
//         +------+     +-------+     +-------+  |
//        | nil  |<--->|   1   |<--->|   2   |<--|
//        +------+     +-------+     +-------+
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/*
 * Procedure: list_init
 * ---------------------
 *
 */
void list_init(list_t* list) {
    assert(list != null);

    list->nil.prev = &list->nil;
    list->nil.next = &list->nil;

    list->size = 0;
}


/*
 * Function:    list_head
 * ----------------------
 * This function returns the head of the list. This is given by L.nil.next.
 * If the list is empty, then a pointer to L.nil is returned. Undefined
 * behavior if [list] is null.
 *
 * @list_t* list:   The list
 *
 * @returns:        The pointer to the head of the list.
 */
list_node_t* list_head(list_t* list) {
    assert(list != null);
    return list->nil.next;
}

/*
 * Function:    list_tail
 * ----------------------
 * This function returns the tail of the list. This is given by L.nil.prev.
 * If the list is empty, then a pointer to L.nil is returned. Undefined
 * behavior if [list] is null.
 *
 * @list_t* list:   The list
 *
 * @returns:        The pointer to the tail of the list.
 */
list_node_t* list_tail(list_t* list) {
    assert(list != null);
    return list->nil.prev;
}


/*
 * Procedure:   list_insert_before
 * -------------------------------
 * This procedure inserts the list node [x] just before the list node [before].
 * The node [before] may either be an interior node or the nil node. The latter case
 * is equivalent to [list_push_tail].
 *
 * @list_node_t* before:    The list node that will be after the list node [x], once inserted.
 * @list_node_t* x:         The list node to be inserted
 *
 */
void list_insert_before(list_t* list, list_node_t* before, list_node_t* x) {
    assert(list != null && before != null && x != null);

    x->prev = before->prev;
    x->next = before;

    before->prev->next = x;
    before->prev = x;

    list->size++;
}


/*
 * Procedure:   list_insert_after
 * -------------------------------
 * This procedure inserts the list node [x] just after the list node [after].
 * The node [after] may either be an interior node or the nil node. The latter case
 * is equivalent to [list_push_head].
 *
 * @list_node_t* after:     The list node that will be before the list node [x], once inserted.
 * @list_node_t* x:         The list node to be inserted
 *
 */
void list_insert_after(list_t* list, list_node_t* after, list_node_t* x) {
    assert(after != null && list != null && x != null);

    x->prev = after;
    x->next = after->next;

    after->next->prev = x;
    after->next = x;

    list->size++;
}


/*
 * Procedure:   list_push_head
 * ---------------------------
 * This procedure pushes the element [x] onto the head/front of the list [list].
 *
 * @list_t* list:       The list.
 * @list_node_t* x:     The list node that is being pushed onto the head of the list.
 *
 */
void list_push_head(list_t* list, list_node_t* x) {
    list_insert_before(list, list_head(list), x);
}

/*
 * Procedure:   list_push_tail
 * ---------------------------
 * This procedure pushes the element [x] onto the tail/end of the list [list].
 *
 * @list_t* list:       The list.
 * @list_node_t* x:     The list node that is being pushed onto the tail of the list.
 *
 */
void list_push_tail(list_t* list, list_node_t* x) {
    list_insert_after(list, list_tail(list), x);
}

/*
 * Procedure:   list_delete
 * ------------------------
 * This procedure deletes the list node [x] from it's list.
 *
 * Note that we have undefined behavior for when [x] is the nil
 * node. (i.e. plz don't call this procedure with x = nil node).
 *
 * It is also not safe to treat x as an element in a list after deleting it.
 * In particular, using the prev and next fields of the node struct.
 * (be very careful with for loops, particular the update condition)
 *
 * @list_node_t* x:     The list node that is being removed from the list
 *
 */
void list_delete(list_t* list, list_node_t* x) {
    assert(x != null && list != null && list->size > 0 && &list->nil != x);

    x->prev->next = x->next;
    x->next->prev = x->prev;
    list->size--;
}


list_node_t* list_pop_head(list_t* list) {
    assert(list != null && list->size != 0);

    list_node_t* head = list_head(list);
    list_delete(list, head);
    return head;
}

list_node_t* list_pop_tail(list_t* list) {
    assert(list != null && list->size != 0);

    list_node_t* tail = list_tail(list);
    list_delete(list, tail);
    return tail;
}

inline size_t list_size(list_t* list) {
    return list->size;
}
