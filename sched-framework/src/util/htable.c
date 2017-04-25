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

#include "util/htable.h"

#include <stdlib.h>

#include "debug/dbg.h"
#include "util/list.h"

static htable_node_t *__htable_lookup( htable_t *ht, unsigned int id );

/* initialize the hash table */
void htable_init( htable_t *ht, unsigned int cap ) {
  unsigned int i;

  ht->ht_hash = (list_t*) malloc( sizeof( list_t ) * cap );
  ht->ht_cap = cap;
  ht->ht_size = 0;

  for( i = 0; i < cap; i++ )
    list_init( &ht->ht_hash[ i ] );
}

/* free the htable_node_t's from the hashtable. calling this implies
   that one has already freed the data that these nodes point to, 
   presumably through htable_iterate */
void htable_destroy( htable_t *ht ) {
  unsigned int i;
  htable_node_t *hn;

  for( i = 0; i < ht->ht_cap; i++ ) {
    list_iterate_begin( &ht->ht_hash[ i ], hn, htable_node_t, hn_link ) {
      free( hn );
    } list_iterate_end();
  }

  free( ht->ht_hash );
}

/* get an entry from the hashtable */
void *htable_get( htable_t *ht, unsigned int id ) {
  htable_node_t *hn;

  if( ( hn = __htable_lookup( ht, id ) ) )
    return hn->hn_data;
  else
    return NULL;
}

/* put an entry into the hashtable */
void *htable_put( htable_t *ht, unsigned int id, void *data ) { 
  htable_node_t *hn;
  void *old = NULL;

  if( !( hn = __htable_lookup( ht, id ) ) ) {
    hn = (htable_node_t*) malloc( sizeof( htable_node_t ) );
    hn->hn_id = id;
    
    list_insert_head( &ht->ht_hash[ id % ht->ht_cap ], &hn->hn_link );
    ht->ht_size++;
  }
  else
    old = hn->hn_data;

  hn->hn_data = data;

  return old;
}

/* removes an entry from the hashtable */
void *htable_remove( htable_t *ht, unsigned int id ) {
  htable_node_t *hn;

  if( ( hn = __htable_lookup( ht, id ) ) ) {
    void *data = hn->hn_data;

    list_remove( &hn->hn_link );
    free( hn );
    ht->ht_size--;

    return data;
  }
  else
    return NULL;
}

/* dump the contents of a hashtable to stdout */
void htable_dump( htable_t *ht ) {
  unsigned int i;
  list_t *l;
  htable_node_t *hn;

  DEBUG(DBG_HASH, "[ hashtable ]\n");

  for( i = 0, l = &ht->ht_hash[ i ]; 
     i < ht->ht_cap; 
     i++, l = &ht->ht_hash[ i ] ) {
    if( !list_empty( l ) )
      printf( "  [ hash index %d ]\n", i );

    list_iterate_begin( l, hn, htable_node_t, hn_link )
      printf( "    %u: %p\n", hn->hn_id, hn->hn_data );
    list_iterate_end();
  }
}

/* look up a hashtable entry */
htable_node_t *__htable_lookup( htable_t *ht, unsigned int id ) {
  htable_node_t *hn;

  list_iterate_begin( &ht->ht_hash[ id % ht->ht_cap ], hn, 
    htable_node_t, hn_link ) 
  {
    if( hn->hn_id == id )
      return hn;
  } list_iterate_end();

  return NULL;
}
