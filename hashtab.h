/* An expandable hash tables datatype.
   Copyright (C) 1999, 2000, 2002, 2003, 2004, 2005, 2009, 2010
   Free Software Foundation, Inc.
   Contributed by Vladimir Makarov (vmakarov@cygnus.com).

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

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
   the old table to the new table.  */

#ifndef __HASHTAB_H__
#define __HASHTAB_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* The type for a hash code.  */
typedef unsigned int hashval_t;

/* Callback function pointer types.  */

/* Calculate hash of a table entry.  */
typedef hashval_t (*htab_hash) (const void *);

/* Compare a table entry with a possible entry.  The entry already in
   the table always comes first, so the second element can be of a
   different type (but in this case htab_find and htab_find_slot
   cannot be used; instead the variants that accept a hash value
   must be used).  */
typedef int (*htab_eq) (const void *, const void *);

/* Cleanup function called whenever a live element is removed from
   the hash table.  */
typedef void (*htab_del) (void *);

/* Function called by htab_traverse for each live element.  The first
   arg is the slot of the element (which can be passed to htab_clear_slot
   if desired), the second arg is the auxiliary pointer handed to
   htab_traverse.  Return 1 to continue scan, 0 to stop.  */
typedef int (*htab_trav) (void **, void *);

/* Hash tables are of the following type.  The structure
   (implementation) of this type is not needed for using the hash
   tables.  All work with hash table should be executed only through
   functions mentioned below. */

struct htab
{
  /* Pointer to hash function.  */
  htab_hash hash_f;

  /* Pointer to comparison function.  */
  htab_eq eq_f;

  /* Pointer to cleanup function.  */
  htab_del del_f;

  /* Table itself.  */
  void **entries;

  /* Current size (in entries) of the hash table */
  size_t size;

  /* Current number of elements including also deleted elements */
  size_t n_elements;

  /* Current number of deleted elements in the table */
  size_t n_deleted;

  /* The following member is used for debugging. Its value is number
     of all calls of `htab_find_slot' for the hash table. */
  unsigned int searches;

  /* The following member is used for debugging.  Its value is number
     of collisions fixed for time of work with the hash table. */
  unsigned int collisions;

  /* This is non-zero if we are allowed to return NULL for function calls
     that allocate memory.  */
  int return_allocation_failure;
};

typedef struct htab *htab_t;

/* An enum saying whether we insert into the hash table or not.  */
enum insert_option {NO_INSERT, INSERT};

/* The prototypes of the package functions. */

/* This function is like htab_create, but may return NULL if memory
   allocation fails, and also signals that htab_find_slot_with_hash and
   htab_find_slot are allowed to return NULL when inserting.  */
extern htab_t	htab_try_create	(size_t, htab_hash, htab_eq, htab_del);
extern void	htab_delete	(htab_t);
extern void	htab_empty	(htab_t);

extern void    *htab_find	(htab_t, const void *);
extern void    **htab_find_slot	(htab_t, const void *, enum insert_option);
extern void    *htab_find_with_hash (htab_t, const void *, hashval_t);
extern void    **htab_find_slot_with_hash (htab_t, const void *, hashval_t,
					  enum insert_option);
extern void	htab_clear_slot	(htab_t, void **);
extern void	htab_remove_elt	(htab_t, void *);

extern void	htab_traverse	(htab_t, htab_trav, void *);

extern size_t	htab_size	(htab_t);
extern size_t	htab_elements	(htab_t);
extern double	htab_collisions	(htab_t);

/* Utility function.  */
unsigned long higher_prime_number (unsigned long);

/* A hash function for pointers.  */
extern htab_hash htab_hash_pointer;

/* An equality function for pointers.  */
extern htab_eq htab_eq_pointer;

#ifndef NDEBUG

#include <stdio.h>

typedef void (*htab_dumpfn) (FILE *, const void *);
typedef void *(*htab_restorefn) (FILE *);

extern void	htab_dump (htab_t, const char *, htab_dumpfn);
extern void	htab_restore (htab_t, const char *, htab_restorefn);

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __HASHTAB_H */
