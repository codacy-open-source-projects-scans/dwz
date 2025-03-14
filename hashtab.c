/* An expandable hash tables datatype.
   Copyright (C) 1999-2016 Free Software Foundation, Inc.
   Contributed by Vladimir Makarov (vmakarov@cygnus.com).

This file is part of the libiberty library.
Libiberty is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

Libiberty is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING.  If not, write to
the Free Software Foundation, 51 Franklin Street - Fifth Floor,
Boston, MA 02110-1301, USA.  */

/* This package implements basic hash table functionality.  It is possible
   to search for an entry, create an entry and destroy an entry.

   Elements in the table are generic pointers.

   The size of the table is not fixed; if the occupancy of the table
   grows too high the hash table will be expanded.

   The abstract data implementation is based on generalized Algorithm D
   from Knuth's book "The art of computer programming".  Hash table is
   expanded by creation of new hash table and transferring elements from
   the old table to the new table. */

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "hashtab.h"

#include <endian.h>
#if __BYTE_ORDER == __BIG_ENDIAN
# define WORDS_BIGENDIAN 1
#endif

/* This macro defines reserved value for empty table entry. */

#define EMPTY_ENTRY    ((void *) 0)

/* This macro defines reserved value for table entry which contained
   a deleted element. */

#define DELETED_ENTRY  ((void *) 1)

static hashval_t hash_pointer (const void *);
static int eq_pointer (const void *, const void *);
static int htab_expand (htab_t);
static void **find_empty_slot_for_expand  (htab_t, hashval_t);

/* At some point, we could make these be NULL, and modify the
   hash-table routines to handle NULL specially; that would avoid
   function-call overhead for the common case of hashing pointers.  */
htab_hash htab_hash_pointer = hash_pointer;
htab_eq htab_eq_pointer = eq_pointer;

/* The following function returns a nearest prime number which is
   greater than N, and near a power of two. */

unsigned long
higher_prime_number (n)
     unsigned long n;
{
  /* These are primes that are near, but slightly smaller than, a
     power of two.  */
  static unsigned long primes[] = {
    (unsigned long) 7,
    (unsigned long) 13,
    (unsigned long) 31,
    (unsigned long) 61,
    (unsigned long) 127,
    (unsigned long) 251,
    (unsigned long) 509,
    (unsigned long) 1021,
    (unsigned long) 2039,
    (unsigned long) 4093,
    (unsigned long) 8191,
    (unsigned long) 16381,
    (unsigned long) 32749,
    (unsigned long) 65521,
    (unsigned long) 131071,
    (unsigned long) 262139,
    (unsigned long) 524287,
    (unsigned long) 1048573,
    (unsigned long) 2097143,
    (unsigned long) 4194301,
    (unsigned long) 8388593,
    (unsigned long) 16777213,
    (unsigned long) 33554393,
    (unsigned long) 67108859,
    (unsigned long) 134217689,
    (unsigned long) 268435399,
    (unsigned long) 536870909,
    (unsigned long) 1073741789,
    (unsigned long) 2147483647,
					/* 4294967291L */
    ((unsigned long) 2147483647) + ((unsigned long) 2147483644),
  };

  unsigned long* low = &primes[0];
  unsigned long* high = &primes[sizeof(primes) / sizeof(primes[0])];

  while (low != high)
    {
      unsigned long* mid = low + (high - low) / 2;
      if (n > *mid)
	low = mid + 1;
      else
	high = mid;
    }

  /* If we've run out of primes, abort.  */
  if (n > *low)
    {
      fprintf (stderr, "Cannot find prime bigger than %lu\n", n);
      abort ();
    }

  return *low;
}

/* Returns a hash code for P.  */

static hashval_t
hash_pointer (p)
     const void * p;
{
  return (hashval_t) ((long)p >> 3);
}

/* Returns non-zero if P1 and P2 are equal.  */

static int
eq_pointer (p1, p2)
     const void * p1;
     const void * p2;
{
  return p1 == p2;
}

/* This function creates table with length slightly longer than given
   source length.  The created hash table is initiated as empty (all the
   hash table entries are EMPTY_ENTRY).  The function returns the created
   hash table.  Memory allocation may fail; it may return NULL.  */

htab_t
htab_try_create (size, hash_f, eq_f, del_f)
     size_t size;
     htab_hash hash_f;
     htab_eq eq_f;
     htab_del del_f;
{
  htab_t result;

  size = higher_prime_number (size);
  result = (htab_t) calloc (1, sizeof (struct htab));
  if (result == NULL)
    return NULL;

  result->entries = (void **) calloc (size, sizeof (void *));
  if (result->entries == NULL)
    {
      free (result);
      return NULL;
    }

  result->size = size;
  result->hash_f = hash_f;
  result->eq_f = eq_f;
  result->del_f = del_f;
  result->return_allocation_failure = 1;
  return result;
}

/* This function frees all memory allocated for given hash table.
   Naturally the hash table must already exist. */

void
htab_delete (htab)
     htab_t htab;
{
  int i;

  if (htab->del_f)
    for (i = htab->size - 1; i >= 0; i--)
      if (htab->entries[i] != EMPTY_ENTRY
	  && htab->entries[i] != DELETED_ENTRY)
	(*htab->del_f) (htab->entries[i]);

  free (htab->entries);
  free (htab);
}

/* This function clears all entries in the given hash table.  */

void
htab_empty (htab)
     htab_t htab;
{
  int i;

  if (htab->del_f)
    for (i = htab->size - 1; i >= 0; i--)
      if (htab->entries[i] != EMPTY_ENTRY
	  && htab->entries[i] != DELETED_ENTRY)
	(*htab->del_f) (htab->entries[i]);

  memset (htab->entries, 0, htab->size * sizeof (void *));
  htab->n_deleted = 0;
  htab->n_elements = 0;
}

/* Similar to htab_find_slot, but without several unwanted side effects:
    - Does not call htab->eq_f when it finds an existing entry.
    - Does not change the count of elements/searches/collisions in the
      hash table.
   This function also assumes there are no deleted entries in the table.
   HASH is the hash value for the element to be inserted.  */

static void **
find_empty_slot_for_expand (htab, hash)
     htab_t htab;
     hashval_t hash;
{
  size_t size = htab->size;
  unsigned int index = hash % size;
  void **slot = htab->entries + index;
  hashval_t hash2;

  if (*slot == EMPTY_ENTRY)
    return slot;
  else if (*slot == DELETED_ENTRY)
    abort ();

  hash2 = 1 + hash % (size - 2);
  for (;;)
    {
      index += hash2;
      if (index >= size)
	index -= size;

      slot = htab->entries + index;
      if (*slot == EMPTY_ENTRY)
	return slot;
      else if (*slot == DELETED_ENTRY)
	abort ();
    }
}

/* The following function changes size of memory allocated for the
   entries and repeatedly inserts the table elements.  The occupancy
   of the table after the call will be about 50%.  Naturally the hash
   table must already exist.  Remember also that the place of the
   table entries is changed.  If memory allocation failures are allowed,
   this function will return zero, indicating that the table could not be
   expanded.  If all goes well, it will return a non-zero value.  */

static int
htab_expand (htab)
     htab_t htab;
{
  void **oentries;
  void **olimit;
  void **p;

  oentries = htab->entries;
  olimit = oentries + htab->size;

  htab->size = higher_prime_number (htab->size * 2);

  if (htab->return_allocation_failure)
    {
      void **nentries = (void **) calloc (htab->size, sizeof (void **));
      if (nentries == NULL)
	return 0;
      htab->entries = nentries;
    }

  htab->n_elements -= htab->n_deleted;
  htab->n_deleted = 0;

  p = oentries;
  do
    {
      void * x = *p;

      if (x != EMPTY_ENTRY && x != DELETED_ENTRY)
	{
	  void **q = find_empty_slot_for_expand (htab, (*htab->hash_f) (x));

	  *q = x;
	}

      p++;
    }
  while (p < olimit);

  free (oentries);
  return 1;
}

/* This function searches for a hash table entry equal to the given
   element.  It cannot be used to insert or delete an element.  */

void *
htab_find_with_hash (htab, element, hash)
     htab_t htab;
     const void * element;
     hashval_t hash;
{
  unsigned int index;
  hashval_t hash2;
  size_t size;
  void * entry;

  htab->searches++;
  size = htab->size;
  index = hash % size;

  entry = htab->entries[index];
  if (entry == EMPTY_ENTRY
      || (entry != DELETED_ENTRY && (*htab->eq_f) (entry, element)))
    return entry;

  hash2 = 1 + hash % (size - 2);

  for (;;)
    {
      htab->collisions++;
      index += hash2;
      if (index >= size)
	index -= size;

      entry = htab->entries[index];
      if (entry == EMPTY_ENTRY
	  || (entry != DELETED_ENTRY && (*htab->eq_f) (entry, element)))
	return entry;
    }
}

/* Like htab_find_slot_with_hash, but compute the hash value from the
   element.  */

void *
htab_find (htab, element)
     htab_t htab;
     const void * element;
{
  return htab_find_with_hash (htab, element, (*htab->hash_f) (element));
}

/* This function searches for a hash table slot containing an entry
   equal to the given element.  To delete an entry, call this with
   INSERT = 0, then call htab_clear_slot on the slot returned (possibly
   after doing some checks).  To insert an entry, call this with
   INSERT = 1, then write the value you want into the returned slot.
   When inserting an entry, NULL may be returned if memory allocation
   fails.  */

void **
htab_find_slot_with_hash (htab, element, hash, insert)
     htab_t htab;
     const void * element;
     hashval_t hash;
     enum insert_option insert;
{
  void **first_deleted_slot;
  unsigned int index;
  hashval_t hash2;
  size_t size;
  void * entry;

  if (insert == INSERT && htab->size * 3 <= htab->n_elements * 4
      && htab_expand (htab) == 0)
    return NULL;

  size = htab->size;
  index = hash % size;

  htab->searches++;
  first_deleted_slot = NULL;

  entry = htab->entries[index];
  if (entry == EMPTY_ENTRY)
    goto empty_entry;
  else if (entry == DELETED_ENTRY)
    first_deleted_slot = &htab->entries[index];
  else if ((*htab->eq_f) (entry, element))
    return &htab->entries[index];

  hash2 = 1 + hash % (size - 2);
  for (;;)
    {
      htab->collisions++;
      index += hash2;
      if (index >= size)
	index -= size;

      entry = htab->entries[index];
      if (entry == EMPTY_ENTRY)
	goto empty_entry;
      else if (entry == DELETED_ENTRY)
	{
	  if (!first_deleted_slot)
	    first_deleted_slot = &htab->entries[index];
	}
      else if ((*htab->eq_f) (entry, element))
	return &htab->entries[index];
    }

 empty_entry:
  if (insert == NO_INSERT)
    return NULL;

  htab->n_elements++;

  if (first_deleted_slot)
    {
      *first_deleted_slot = EMPTY_ENTRY;
      return first_deleted_slot;
    }

  return &htab->entries[index];
}

/* Like htab_find_slot_with_hash, but compute the hash value from the
   element.  */

void **
htab_find_slot (htab, element, insert)
     htab_t htab;
     const void * element;
     enum insert_option insert;
{
  return htab_find_slot_with_hash (htab, element, (*htab->hash_f) (element),
				   insert);
}

/* This function deletes an element with the given value from hash
   table.  If there is no matching element in the hash table, this
   function does nothing.  */

void
htab_remove_elt (htab, element)
     htab_t htab;
     void * element;
{
  void **slot;

  slot = htab_find_slot (htab, element, NO_INSERT);
  if (*slot == EMPTY_ENTRY)
    return;

  if (htab->del_f)
    (*htab->del_f) (*slot);

  *slot = DELETED_ENTRY;
  htab->n_deleted++;
}

/* This function clears a specified slot in a hash table.  It is
   useful when you've already done the lookup and don't want to do it
   again.  */

void
htab_clear_slot (htab, slot)
     htab_t htab;
     void **slot;
{
  if (slot < htab->entries || slot >= htab->entries + htab->size
      || *slot == EMPTY_ENTRY || *slot == DELETED_ENTRY)
    abort ();

  if (htab->del_f)
    (*htab->del_f) (*slot);

  *slot = DELETED_ENTRY;
  htab->n_deleted++;
}

/* This function scans over the entire hash table calling
   CALLBACK for each live entry.  If CALLBACK returns false,
   the iteration stops.  INFO is passed as CALLBACK's second
   argument.  */

void
htab_traverse (htab, callback, info)
     htab_t htab;
     htab_trav callback;
     void * info;
{
  void **slot = htab->entries;
  void **limit = slot + htab->size;

  do
    {
      void * x = *slot;

      if (x != EMPTY_ENTRY && x != DELETED_ENTRY)
	if (!(*callback) (slot, info))
	  break;
    }
  while (++slot < limit);
}

/* Return the current size of given hash table. */

size_t
htab_size (htab)
     htab_t htab;
{
  return htab->size;
}

/* Return the current number of elements in given hash table. */

size_t
htab_elements (htab)
     htab_t htab;
{
  return htab->n_elements - htab->n_deleted;
}

/* Return the fraction of fixed collisions during all work with given
   hash table. */

double
htab_collisions (htab)
     htab_t htab;
{
  if (htab->searches == 0)
    return 0.0;

  return (double) htab->collisions / (double) htab->searches;
}

#ifndef NDEBUG
void
htab_dump (htab, name, dumpfn)
     htab_t htab;
     const char *name;
     htab_dumpfn dumpfn;
{
  FILE *f = fopen (name, "w");
  size_t i, j;

  if (f == NULL)
    abort ();
  fprintf (f, "size %zd n_elements %zd n_deleted %zd\n",
	   htab->size, htab->n_elements, htab->n_deleted);
  for (i = 0; i < htab->size; ++i)
    {
      if (htab->entries [i] == EMPTY_ENTRY
	  || htab->entries [i] == DELETED_ENTRY)
	{
	  for (j = i + 1; j < htab->size; ++j)
	    if (htab->entries [j] != htab->entries [i])
	      break;
	  fprintf (f, "%c%zd\n",
		   htab->entries [i] == EMPTY_ENTRY ? 'E' : 'D',
		   j - i);
	  i = j - 1;
	}
      else
	{
	  fputc ('V', f);
	  (*dumpfn) (f, htab->entries [i]);
	}
    }
  fclose (f);
}

void
htab_restore (htab, name, restorefn)
     htab_t htab;
     const char *name;
     htab_restorefn restorefn;
{
  FILE *f = fopen (name, "r");
  size_t size, n_elements, n_deleted, i, j, k;
  int c;

  if (f == NULL)
    abort ();
  if (fscanf (f, "size %zd n_elements %zd n_deleted %zd\n",
	      &size, &n_elements, &n_deleted) != 3)
    abort ();
  htab_empty (htab);
  free (htab->entries);
  htab->entries = (void **) calloc (size, sizeof (void *));
  if (htab->entries == NULL)
    abort ();
  htab->size = size;
  htab->n_elements = n_elements;
  htab->n_deleted = n_deleted;
  for (i = 0; i < htab->size; ++i)
    {
      switch ((c = fgetc (f)))
	{
	case 'E':
	case 'D':
	  if (fscanf (f, "%zd\n", &j) != 1)
	    abort ();
	  if (i + j > htab->size)
	    abort ();
	  if (c == 'D')
	    for (k = i; k < i + j; ++k)
	      htab->entries [k] = DELETED_ENTRY;
	  i += j - 1;
	  break;
	case 'V':
	  htab->entries [i] = (*restorefn) (f);
	  break;
	default:
	  abort ();
	}
    }
  fclose (f);
}
#endif
