/* server.c --- SASL mechanism GSSAPI as defined in RFC 2222, server side.
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

#include "x-gssapi.h"

#include "shared.h"

struct _Gsasl_gssapi_server_state
{
  int step;
  gss_name_t client;
  gss_cred_id_t cred;
  gss_ctx_id_t context;
};
typedef struct _Gsasl_gssapi_server_state _Gsasl_gssapi_server_state;

int
_gsasl_gssapi_server_start (Gsasl_session_ctx * sctx, void **mech_data)
{
  _Gsasl_gssapi_server_state *state;
  Gsasl_server_callback_service cb_service;
  Gsasl_ctx *ctx;
  OM_uint32 maj_stat, min_stat;
  gss_name_t server;
  gss_buffer_desc bufdesc;
  size_t servicelen = 0;
  size_t hostnamelen = 0;
  int res;

  ctx = gsasl_server_ctx_get (sctx);
  if (ctx == NULL)
    return GSASL_CANNOT_GET_CTX;

  cb_service = gsasl_server_callback_service_get (ctx);
  if (cb_service == NULL)
    return GSASL_NEED_SERVER_SERVICE_CALLBACK;

  if (gsasl_server_callback_gssapi_get (ctx) == NULL)
    return GSASL_NEED_SERVER_GSSAPI_CALLBACK;

  res = cb_service (sctx, NULL, &servicelen, NULL, &hostnamelen);
  if (res != GSASL_OK)
    return res;

  bufdesc.length = servicelen + strlen ("@") + hostnamelen + 1;
  bufdesc.value = malloc (bufdesc.length);
  if (bufdesc.value == NULL)
    return GSASL_MALLOC_ERROR;

  res = cb_service (sctx, bufdesc.value, &servicelen,
		    (char *) bufdesc.value + 1 + servicelen, &hostnamelen);
  if (res != GSASL_OK)
    {
      free (bufdesc.value);
      return res;
    }
  ((char *) bufdesc.value)[servicelen] = '@';
  ((char *) bufdesc.value)[bufdesc.length - 1] = '\0';

  state = (_Gsasl_gssapi_server_state *) malloc (sizeof (*state));
  if (state == NULL)
    {
      free (bufdesc.value);
      return GSASL_MALLOC_ERROR;
    }

  maj_stat = gss_import_name (&min_stat, &bufdesc, GSS_C_NT_HOSTBASED_SERVICE,
			      &server);
  free (bufdesc.value);
  if (GSS_ERROR (maj_stat))
    {
      free (state);
      return GSASL_GSSAPI_IMPORT_NAME_ERROR;
    }

  maj_stat = gss_acquire_cred (&min_stat, server, 0,
			       GSS_C_NULL_OID_SET, GSS_C_ACCEPT,
			       &state->cred, NULL, NULL);
  gss_release_name (&min_stat, &server);

  if (GSS_ERROR (maj_stat))
    {
      free (state);
      return GSASL_GSSAPI_ACQUIRE_CRED_ERROR;
    }

  state->step = 0;
  state->context = GSS_C_NO_CONTEXT;
  state->client = NULL;
  *mech_data = state;

  return GSASL_OK;
}

int
_gsasl_gssapi_server_step (Gsasl_session_ctx * sctx,
			   void *mech_data,
			   const char *input,
			   size_t input_len,
			   char *output, size_t * output_len)
{
  _Gsasl_gssapi_server_state *state = mech_data;
  Gsasl_server_callback_gssapi cb_gssapi;
  gss_buffer_desc bufdesc1, bufdesc2;
  OM_uint32 maj_stat, min_stat;
  gss_buffer_desc client_name;
  gss_OID mech_type;
  Gsasl_ctx *ctx;
  char *username;
  int res;

  ctx = gsasl_server_ctx_get (sctx);
  if (ctx == NULL)
    return GSASL_CANNOT_GET_CTX;

  cb_gssapi = gsasl_server_callback_gssapi_get (ctx);
  if (cb_gssapi == NULL)
    return GSASL_NEED_SERVER_GSSAPI_CALLBACK;

  switch (state->step)
    {
    case 0:
      if (input_len == 0)
	{
	  *output_len = 0;
	  return GSASL_NEEDS_MORE;
	}
      state->step++;
      /* fall through */

    case 1:
      bufdesc1.value = /*XXX*/ (char *) input;
      bufdesc1.length = input_len;
      maj_stat = gss_accept_sec_context (&min_stat,
					 &state->context,
					 state->cred,
					 &bufdesc1,
					 GSS_C_NO_CHANNEL_BINDINGS,
					 &state->client,
					 &mech_type,
					 &bufdesc2, NULL, NULL, NULL);
      if (maj_stat != GSS_S_COMPLETE && maj_stat != GSS_S_CONTINUE_NEEDED)
	{
	  return GSASL_GSSAPI_ACCEPT_SEC_CONTEXT_ERROR;
	}

      if (*output_len < bufdesc2.length)
	{
	  maj_stat = gss_release_buffer (&min_stat, &bufdesc2);
	  return GSASL_TOO_SMALL_BUFFER;
	}

      if (maj_stat == GSS_S_COMPLETE)
	state->step++;

      memcpy (output, bufdesc2.value, bufdesc2.length);
      *output_len = bufdesc2.length;

      maj_stat = gss_release_buffer (&min_stat, &bufdesc2);
      if (GSS_ERROR (maj_stat))
	return GSASL_GSSAPI_RELEASE_BUFFER_ERROR;

      res = GSASL_NEEDS_MORE;
      break;

    case 2:
      if (*output_len < 4)
	return GSASL_TOO_SMALL_BUFFER;

      memset (output, 0xFF, 4);
      output[0] = GSASL_QOP_AUTH;
      bufdesc1.length = 4;
      bufdesc1.value = output;
      maj_stat = gss_wrap (&min_stat, state->context, 0, GSS_C_QOP_DEFAULT,
			   &bufdesc1, NULL, &bufdesc2);
      if (GSS_ERROR (maj_stat))
	return GSASL_GSSAPI_WRAP_ERROR;

      if (*output_len < bufdesc2.length)
	{
	  maj_stat = gss_release_buffer (&min_stat, &bufdesc2);
	  return GSASL_TOO_SMALL_BUFFER;
	}
      memcpy (output, bufdesc2.value, bufdesc2.length);
      *output_len = bufdesc2.length;

      maj_stat = gss_release_buffer (&min_stat, &bufdesc2);
      if (GSS_ERROR (maj_stat))
	return GSASL_GSSAPI_RELEASE_BUFFER_ERROR;

      state->step++;
      res = GSASL_NEEDS_MORE;
      break;

    case 3:
      bufdesc1.value = /*XXX*/ (char *) input;
      bufdesc1.length = input_len;
      maj_stat = gss_unwrap (&min_stat, state->context, &bufdesc1,
			     &bufdesc2, NULL, NULL);
      if (GSS_ERROR (maj_stat))
	return GSASL_GSSAPI_UNWRAP_ERROR;

      /* [RFC 2222 section 7.2.1]:
         The client passes this token to GSS_Unwrap and interprets the
         first octet of resulting cleartext as a bit-mask specifying
         the security layers supported by the server and the second
         through fourth octets as the maximum size output_message to
         send to the server.  The client then constructs data, with
         the first octet containing the bit-mask specifying the
         selected security layer, the second through fourth octets
         containing in network byte order the maximum size
         output_message the client is able to receive, and the
         remaining octets containing the authorization identity.  The
         client passes the data to GSS_Wrap with conf_flag set to
         FALSE, and responds with the generated output_message.  The
         client can then consider the server authenticated. */

      if ((((char *) bufdesc2.value)[0] & GSASL_QOP_AUTH) == 0)
	{
	  /* Integrity or privacy unsupported */
	  maj_stat = gss_release_buffer (&min_stat, &bufdesc2);
	  return GSASL_GSSAPI_UNSUPPORTED_PROTECTION_ERROR;
	}

      username = malloc (bufdesc2.length - 4 + 1);
      if (username == NULL)
	{
	  gss_release_buffer (&min_stat, &bufdesc2);
	  return GSASL_MALLOC_ERROR;
	}

      memcpy (username, (char *) bufdesc2.value + 4, bufdesc2.length - 4);
      username[bufdesc2.length - 4] = '\0';
      maj_stat = gss_release_buffer (&min_stat, &bufdesc2);
      if (GSS_ERROR (maj_stat))
	return GSASL_GSSAPI_RELEASE_BUFFER_ERROR;

      maj_stat = gss_display_name (&min_stat, state->client,
				   &client_name, &mech_type);
      if (GSS_ERROR (maj_stat))
	{
	  free (username);
	  return GSASL_GSSAPI_DISPLAY_NAME_ERROR;
	}

      res = cb_gssapi (sctx, client_name.value, username);
      free (username);

      *output_len = 0;
      state->step++;
      break;

    default:
      res = GSASL_MECHANISM_CALLED_TOO_MANY_TIMES;
      break;
    }

  return res;
}

int
_gsasl_gssapi_server_finish (Gsasl_session_ctx * sctx, void *mech_data)
{
  _Gsasl_gssapi_server_state *state = mech_data;
  OM_uint32 min_stat;

  if (state->context != GSS_C_NO_CONTEXT)
    gss_delete_sec_context (&min_stat, &state->context, GSS_C_NO_BUFFER);

  if (state->cred != GSS_C_NO_CREDENTIAL)
    gss_release_cred (&min_stat, &state->cred);

  free (state);

  return GSASL_OK;
}