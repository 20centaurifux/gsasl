/* mechinfo.c --- Definition of SECURID mechanism.
 * Copyright (C) 2002, 2003, 2004  Simon Josefsson
 *
 * This file is part of GNU SASL Library.
 *
 * GNU SASL Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * GNU SASL Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GNU SASL Library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307 USA
 *
 */

#if HAVE_CONFIG_H
# include "config.h"
#endif

/* Get specification. */
#include "securid.h"

Gsasl_mechanism gsasl_securid_mechanism = {
  GSASL_SECURID_NAME,
  {
    NULL,
    NULL,
#ifdef USE_CLIENT
    _gsasl_securid_client_start,
#else
    NULL,
#endif
#ifdef USE_CLIENT
    _gsasl_securid_client_step,
#else
    NULL,
#endif
#ifdef USE_CLIENT
    _gsasl_securid_client_finish,
#else
    NULL,
#endif
    NULL,
    NULL
  },
  {
    NULL,
    NULL,
    NULL,
#ifdef USE_SERVER
    _gsasl_securid_server_step,
#else
    NULL,
#endif
    NULL,
    NULL,
    NULL
  }
};