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

 /* a chained hashtable implementation */

#ifndef __UTIL_HASHTABLE_H__
#define __UTIL_HASHTABLE_H__

#include "util/list.h"

/* TODO:
 *  - make this a doubly-hashed, dynamically groweable hashtable */

typedef struct htable {
  list_t *ht_hash;				   /* table entries */
  unsigned int ht_size;      /* table size */
  unsigned int ht_cap;		   /* table capacity */
} htable_t;

typedef struct htable_node {
  list_t hn_link;            /* link */
  unsigned int hn_id;        /* hash id */
  void *hn_data;             /* data */
} htable_node_t;

void htable_init( htable_t *ht, unsigned int cap );
void htable_destroy( htable_t *ht );
void *htable_get( htable_t *ht, unsigned int id );
void *htable_put( htable_t *ht, unsigned int id, void *data );
void *htable_remove( htable_t *ht, unsigned int id );
void htable_dump( htable_t *ht );

#define htable_iterate_begin( ht, key, var, type )  \
do {                                                \
  unsigned int ___i;                                \
  htable_t *__ht = (ht);                            \
  htable_node_t *__hnode;                           \
  for(___i = 0;___i < __ht->ht_cap;___i++ ) {       \
    list_iterate_begin( &__ht->ht_hash[___i ],      \
      __hnode, htable_node_t, hn_link )             \
    {                                               \
      (var) = (type*) __hnode->hn_data;             \
      (key) = __hnode->hn_id;                       \
      do

#define htable_iterate_end()  \
      while( 0 );             \
    } list_iterate_end();     \
  }                           \
} while( 0 )

#endif // __UTIL_HASHTABLE_H__
