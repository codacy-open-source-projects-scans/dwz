/* Copyright (C) 2001-2021 Red Hat, Inc.
   Copyright (C) 2003 Free Software Foundation, Inc.
   Copyright (C) 2019-2021 SUSE LLC.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, 51 Franklin Street - Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* Wrapper around obstack.h that changes obstack_free (obstack_ptr, object) to
   also nullify object.  Since that breaks obstack_free (obstack_ptr, NULL), a
   new function obstack_destroy is added to support that functionaliy.  */

#include <obstack.h>

/* Required definitions for obstack use.  */
#define obstack_chunk_alloc malloc
#define obstack_chunk_free free

/* Free *OBJECT, and assign NULL to it.  */
static inline void
obstack_free_and_nullify (struct obstack *obstack_ptr, void **object)
{
  /* Don't attempt to free nullified *OBJECT.  */
  assert (*object != NULL);

  obstack_free (obstack_ptr, *object);

  /* Nullify *OBJECT to prevent use-after-free.  */
  *object = NULL;
}

/* Free all objects in OBSTACK_PTR, and leave the obstack uninitialized.  */
static inline void
obstack_destroy (struct obstack *obstack_ptr)
{
  obstack_free (obstack_ptr, NULL);
}

/* In case obstack_free is implemented as a macro, undefine it in order to be
   able to redefine it.  */
#undef obstack_free

/* Redefine obstack_free to nullify OBJECT.  */
#define obstack_free(obstack_ptr, object)			\
  obstack_free_and_nullify (obstack_ptr, (void **)&object)
