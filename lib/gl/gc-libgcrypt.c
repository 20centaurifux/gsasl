/* gc-libgcrypt.c --- Crypto wrappers around Libgcrypt for GC.
 * Copyright (C) 2002, 2003, 2004, 2005  Simon Josefsson
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1, or (at your
 * option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this file; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

/* Note: This file is only built if GC uses Libgcrypt. */

#if HAVE_CONFIG_H
# include <config.h>
#endif

/* Get prototype. */
#include "gc.h"

/* Get libgcrypt API. */
#include <gcrypt.h>

#include <assert.h>

/* Initialization. */

int
gc_init (void)
{
  gcry_error_t err;

  err = gcry_control (GCRYCTL_ANY_INITIALIZATION_P);
  if (err == GPG_ERR_NO_ERROR)
    {
      if (gcry_check_version (GCRYPT_VERSION) == NULL)
	return GC_INIT_ERROR;

      err = gcry_control (GCRYCTL_INITIALIZATION_FINISHED, NULL, 0);
      if (err != GPG_ERR_NO_ERROR)
	return GC_INIT_ERROR;
    }

  return GC_OK;
}

void
gc_done (void)
{
  return;
}

/* Randomness. */

int
gc_nonce (char *data, size_t datalen)
{
  gcry_create_nonce ((unsigned char *) data, datalen);
  return GC_OK;
}

int
gc_pseudo_random (char *data, size_t datalen)
{
  gcry_randomize ((unsigned char *) data, datalen, GCRY_STRONG_RANDOM);
  return GC_OK;
}

int
gc_random (char *data, size_t datalen)
{
  gcry_randomize ((unsigned char *) data, datalen, GCRY_VERY_STRONG_RANDOM);
  return GC_OK;
}

/* Memory allocation. */

void
gc_set_allocators (gc_malloc_t func_malloc,
		   gc_malloc_t secure_malloc,
		   gc_secure_check_t secure_check,
		   gc_realloc_t func_realloc, gc_free_t func_free)
{
  gcry_set_allocation_handler (func_malloc, secure_malloc, secure_check,
			       func_realloc, func_free);
}

/* One-call interface. */

int
gc_md5 (const void *in, size_t inlen, void *resbuf)
{
  size_t outlen = gcry_md_get_algo_dlen (GCRY_MD_MD5);
  gcry_md_hd_t hd;
  gpg_error_t err;
  unsigned char *p;

  assert (outlen == 16);

  err = gcry_md_open (&hd, GCRY_MD_MD5, 0);
  if (err != GPG_ERR_NO_ERROR)
    return GC_INVALID_HASH;

  gcry_md_write (hd, in, inlen);

  p = gcry_md_read (hd, GCRY_MD_MD5);
  if (p == NULL)
    return GC_INVALID_HASH;

  memcpy (resbuf, p, outlen);

  gcry_md_close (hd);

  return GC_OK;
}

int
gc_hmac_md5 (const void *key, size_t keylen,
	     const void *in, size_t inlen, char *resbuf)
{
  size_t hlen = gcry_md_get_algo_dlen (GCRY_MD_MD5);
  gcry_md_hd_t mdh;
  unsigned char *hash;
  gpg_error_t err;

  assert (hlen == 16);

  err = gcry_md_open (&mdh, GCRY_MD_MD5, GCRY_MD_FLAG_HMAC);
  if (err != GPG_ERR_NO_ERROR)
    return GC_INVALID_HASH;

  err = gcry_md_setkey (mdh, key, keylen);
  if (err != GPG_ERR_NO_ERROR)
    return GC_INVALID_HASH;

  gcry_md_write (mdh, in, inlen);

  hash = gcry_md_read (mdh, GCRY_MD_MD5);
  if (hash == NULL)
    return GC_INVALID_HASH;

  memcpy (resbuf, hash, hlen);

  gcry_md_close (mdh);

  return GC_OK;
}
