/*====================================================================*
 *
 *   Copyright (c) 2013 Qualcomm Atheros, Inc.
 *
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or
 *   without modification, are permitted (subject to the limitations
 *   in the disclaimer below) provided that the following conditions
 *   are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials
 *     provided with the distribution.
 *
 *   * Neither the name of Qualcomm Atheros nor the names of
 *     its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written
 *     permission.
 *
 *   NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *   GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE
 *   COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 *   IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *   PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 *   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *   NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *--------------------------------------------------------------------*/

/*====================================================================*
 *
 *   pibdump.c - Qualcomm Atheros Parameter Information Block Dump Utility
 *
 *
 *   Contributor(s):
 *	Charles Maier
 *
 *--------------------------------------------------------------------*/

/*====================================================================*
 *   system header files;
 *--------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>

/*====================================================================*
 *   custom header files;
 *--------------------------------------------------------------------*/

#include "../tools/getoptv.h"
#include "../tools/putoptv.h"
#include "../tools/version.h"
#include "../tools/memory.h"
#include "../tools/number.h"
#include "../tools/error.h"
#include "../tools/chars.h"
#include "../tools/flags.h"
#include "../tools/sizes.h"
#include "../tools/files.h"
#include "../pib/pib.h"
#include "../nvm/nvm.h"

/*====================================================================*
 *   custom source files;
 *--------------------------------------------------------------------*/

#ifndef MAKEFILE
#include "../tools/getoptv.c"
#include "../tools/putoptv.c"
#include "../tools/version.c"
#include "../tools/hexview.c"
#include "../tools/hexoffset.c"
#include "../tools/error.c"
#include "../tools/checksum32.c"
#include "../tools/fdchecksum32.c"
#endif

#ifndef MAKEFILE
#include "../nvm/nvmseek2.c"
#include "../nvm/nvm.c"
#endif

/*====================================================================*
 *
 *   void pibdump (char const * filename, flag_t flags);
 *
 *   open the named file and determine if it is a Qualcomm Atheros
 *   parameter information block based on file header information;
 *   read object definitions from stdin and print an object driven
 *   dump on stdout;
 *
 *
 *--------------------------------------------------------------------*/

static void pibdump (char const * filename, flag_t flags)

{
	uint32_t version;
	unsigned object = 0;
	unsigned lineno = 0;
	off_t origin = 0;
	off_t offset = 0;
	off_t extent = 0;
	unsigned length = 0;
	char memory [_ADDRSIZE + 1];
	char symbol [_NAMESIZE];
	char string [_LINESIZE];
	char * sp;
	signed fd;
	signed c;
	if ((fd = open (filename, O_BINARY|O_RDONLY)) == -1)
	{
		error (1, errno, "%s", filename);
	}
	if (read (fd, &version, sizeof (version)) != sizeof (version))
	{
		error (1, errno, FILE_CANTREAD, filename);
	}
	if (lseek (fd, 0, SEEK_SET))
	{
		error (1, errno, FILE_CANTHOME, filename);
	}
	if (LE32TOH (version) == 0x60000000)
	{
		error (1, 0, "%s is not a PIB file", filename);
	}
	else if (LE32TOH (version) == 0x00010001)
	{
		struct nvm_header2 nvm_header;
		if (nvmseek2 (fd, filename, &nvm_header, NVM_IMAGE_PIB))
		{
			error (1, 0, "%s is not a PIB file", filename);
		}
		extent = LE32TOH (nvm_header.ImageLength);
	}
	else
	{
		struct simple_pib pib_header;
		if (read (fd, &pib_header, sizeof (pib_header)) != sizeof (pib_header))
		{
			error (1, errno, FILE_CANTREAD, filename);
		}
		if (lseek (fd, 0, SEEK_SET))
		{
			error (1, errno, FILE_CANTHOME, filename);
		}
		extent = LE16TOH (pib_header.PIBLENGTH);
	}
	while ((c = getc (stdin)) != EOF)
	{
		if ((c == '#') || (c == ';'))
		{
			do
			{
				c = getc (stdin);
			}
			while (nobreak (c));
			lineno++;
			continue;
		}
		if (isspace (c))
		{
			if (c == '\n')
			{
				lineno++;
			}
			continue;
		}
		if (c == '+')
		{
			do { c = getc (stdin); } while (isblank (c));
		}
		length = 0;
		while (isdigit (c))
		{
			length *= 10;
			length += c - '0';
			c = getc (stdin);
		}
		while (isblank (c))
		{
			c = getc (stdin);
		}
		sp = symbol;
		if (isalpha (c) || (c == '_'))
		{
			do
			{
				*sp++ = (char)(c);
				c = getc (stdin);
			}
			while (isident (c));
		}
		while (isblank (c))
		{
			c = getc (stdin);
		}
		if (c == '[')
		{
			*sp++ = (char)(c);
			c = getc (stdin);
			while (isblank (c))
			{
				c = getc (stdin);
			}
			while (isdigit (c))
			{
				*sp++ = (char)(c);
				c = getc (stdin);
			}
			while (isblank (c))
			{
				c = getc (stdin);
			}
			*sp = (char)(0);
			if (c != ']')
			{
				error (1, EINVAL, "Have '%s' without ']' on line %d", symbol, lineno);
			}
			*sp++ = (char)(c);
			c = getc (stdin);
		}
		*sp = (char)(0);
		while (isblank (c))
		{
			c = getc (stdin);
		}
		sp = string;
		while (nobreak (c))
		{
			*sp++ = (char)(c);
			c = getc (stdin);
		}
		*sp = (char)(0);
		if (length)
		{

#if defined (WIN32)

			char * buffer = (char *)(emalloc (length));

#else

			byte buffer [length];

#endif

			if (read (fd, buffer, length) == (signed)(length))
			{
				if (!object++)
				{
					for (c = 0; c < _ADDRSIZE + 65; c++)
					{
						putc ('-', stdout);
					}
					putc ('\n', stdout);
				}
				printf ("%s %u %s\n", hexoffset (memory, sizeof (memory), offset), length, symbol);
				hexview (buffer, offset, length, stdout);
				for (c = 0; c < _ADDRSIZE + 65; c++)
				{
					putc ('-', stdout);
				}
				putc ('\n', stdout);
			}

#if defined (WIN32)

			free (buffer);

#endif

		}
		offset += length;
		lineno++;
	}
	offset += origin;
	if (_allclr (flags, PIB_SILENCE))
	{
		if (offset < extent)
		{
			error (0, 0, "file %s exceeds definition by " OFF_T_SPEC " bytes", filename, extent - offset);
		}
		if (offset > extent)
		{
			error (0, 0, "definition exceeds file %s by " OFF_T_SPEC " bytes", filename, offset - extent);
		}
		if (offset != extent)
		{
			error (0, 0, "file %s is " OFF_T_SPEC " bytes not " OFF_T_SPEC " bytes", filename, extent, offset);
		}
	}
	close (fd);
	return;
}


/*====================================================================*
 *
 *   int main (int argc, char const * argv []);
 *
 *
 *--------------------------------------------------------------------*/

int main (int argc, char const * argv [])

{
	static char const * optv [] =
	{
		"f:qv",
		"file",
		"Qualcomm Atheros Parameter Information Block Dump Utility",
		"f f\tobject definition file",
		"q\tquiet mode",
		"v\tverbose mode",
		(char const *)(0)
	};
	flag_t flags = (flag_t)(0);
	signed c;
	optind = 1;
	while ((c = getoptv (argc, argv, optv)) != -1)
	{
		switch (c)
		{
		case 'f':
			if (!freopen (optarg, "rb", stdin))
			{
				error (1, errno, "%s", optarg);
			}
			break;
		case 'q':
			_setbits (flags, PIB_SILENCE);
			break;
		case 'v':
			_setbits (flags, PIB_VERBOSE);
			break;
		default:
			break;
		}
	}
	argc -= optind;
	argv += optind;
	if (argc > 1)
	{
		error (1, ENOTSUP, ERROR_TOOMANY);
	}
	if ((argc) && (* argv))
	{
		pibdump (*argv, flags);
	}
	return (0);
}

