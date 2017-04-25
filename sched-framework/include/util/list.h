/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __UTIL_LIST_H__
#define __UTIL_LIST_H__

#include <stddef.h>

/*
** Generic circular doubly linked list implementation.
**
** list_t is the head of the list.
** list_link_t should be included in structures which want to be
**     linked on a list_t.
**
** All of the list functions take pointers to list_t and list_link_t
** types, unless otherwise specified.
**
** list_init(list) initializes a list_t to an empty list.
**
** list_empty(list) returns 1 iff the list is empty.
**
** Insertion functions.
**   list_insert_head(list, link) inserts at the front of the list.
**   list_insert_tail(list, link) inserts at the end of the list.
**   list_insert_before(olink, nlink) inserts nlink before olink in list.
**
** Removal functions.
** Head is list->l_next.  Tail is list->l_prev.
** The following functions should only be called on non-empty lists.
**   list_remove(link) removes a specific element from the list.
**   list_remove_head(list) removes the first element.
**   list_remove_tail(list) removes the last element.
**
** Item accessors.
**   list_item(link, type, member) given a list_link_t* and the name
**      of the type of structure which contains the list_link_t and
**      the name of the member corresponding to the list_link_t,
**      returns a pointer (of type "type*") to the item.
**
** To iterate over a list,
**
**    list_link_t *link;
**    for (link = list->l_next;
**         link != list; link = link->l_next)
**       ...
** 
** Or, use the macros, which will work even if you list_remove() the
** current link:
** 
**    type iterator;
**    list_iterate_begin(list, iterator, type, member) {
**        ... use iterator ...
**    } list_iterate_end;
**
** To delete/free/destroy all elements of a list, leaving the list empty,
** use clear_list(list, fn, type, member), which iterates over a list of type
** type and calls list_remove and then fn() on each element. As an example,
** clear_list(&queue, free, rip_raw_message_t, rw_link)
**
*/

typedef struct llist {
  struct llist *l_next;
  struct llist *l_prev;
} list_t, list_link_t;

#define list_init(list) \
    (list)->l_next = (list)->l_prev = (list);

#define list_link_init(link) \
    (link)->l_next = (link)->l_prev = NULL;

#define list_empty(list) \
  ((list)->l_next == (list))

#define list_link_in_list(link) \
  ((link)->l_next != NULL || (link)->l_prev != NULL)

#define list_insert_before(old, new) \
  do { \
    list_link_t *prev = (new); \
    list_link_t *next = (old); \
    prev->l_next = next; \
    prev->l_prev = next->l_prev; \
    next->l_prev->l_next = prev; \
    next->l_prev = prev; \
  } while(0)

#define list_insert_head(list, link) \
  list_insert_before((list)->l_next, link)

#define list_insert_tail(list, link) \
  list_insert_before(list, link)

#define list_remove(link) \
  do { \
    list_link_t *ll = (link); \
    list_link_t *prev = ll->l_prev; \
    list_link_t *next = ll->l_next; \
    prev->l_next = next; \
    next->l_prev = prev; \
    ll->l_next = ll->l_prev = 0; \
  } while(0)

#define list_remove_head(list) \
  list_remove((list)->l_next)

#define list_remove_tail(list) \
  list_remove((list)->l_prev)

#define list_item(link, type, member) \
  (type*)((char*)(link) - offsetof(type, member))

#define list_head(list, type, member) \
  list_item((list)->l_next, type, member)

#define list_tail(list, type, member) \
  list_item((list)->l_prev, type, member)

#define list_iterate_begin(list, var, type, member) \
  do { \
    list_link_t *__link; \
    list_link_t *__next; \
    for (__link = (list)->l_next; \
         __link != (list); \
         __link = __next) { \
      var = list_item(__link, type, member); \
      __next = __link->l_next;

#define list_iterate_end() \
        } \
      } while(0)

#define list_iterate_reverse_begin(list, var, type, member) \
    do { \
        list_link_t *__link; \
        list_link_t *__prev; \
        for (__link = (list)->l_prev; \
             __link != (list); \
             __link = __prev) { \
            var = list_item(__link, type, member); \
            __prev = __link->l_prev;

#define list_iterate_reverse_end() \
        } \
    } while(0)
    
#define clear_list(list, del, type, member) \
  type *__to_del;	\
  list_iterate_begin(list, __to_del, type, member) { \
    list_remove(__link);	\
    del(__to_del);	\
  } list_iterate_end();

#endif //__UTIL_LIST_H__
