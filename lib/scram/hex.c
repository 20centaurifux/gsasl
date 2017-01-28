/* Get strchr, */
#include <string.h>

/* Get bool. */
#include <stdbool.h>

static char
hexdigit_to_char (char hexdigit)
{
  if (hexdigit >= '0' && hexdigit <= '9')
    return hexdigit - '0';
  if (hexdigit >= 'a' && hexdigit <= 'f')
    return hexdigit - 'a' + 10;
  return 0;
}

static char
hex_to_char (char u, char l)
{
  return (char) (((unsigned char) hexdigit_to_char (u)) * 16
		 + hexdigit_to_char (l));
}

void
sha1_hex_to_byte (char *saltedpassword, const char *p)
{
  while (*p)
    {
      *saltedpassword = hex_to_char (p[0], p[1]);
      p += 2;
      saltedpassword++;
    }
}

bool
hex_p (const char *hexstr)
{
  static const char hexalpha[] = "0123456789abcdef";

  for (; *hexstr; hexstr++)
    if (strchr (hexalpha, *hexstr) == NULL)
      return false;

  return true;
}
