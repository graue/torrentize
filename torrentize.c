#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "err.h"

#define DEFAULT_BLOCKSIZE 256

const struct option opts[] =
{
	{ "block-size",		required_argument,	NULL, 'b' },
	{ "ignore",		required_argument,	NULL, 'i' },
	{ "output-name",	required_argument,	NULL, 'o' },
	{ "private",		no_argument,		NULL, 'p' },
	{ "quiet",		no_argument,		NULL, 'q' },
	{ "rename-topdir",	required_argument,	NULL, 'R' }
};

static void usage(void)
{
	fprintf(stderr,
		"usage: torrentize [options] tracker_URL ... file ...\n"
		"\n"
		"-b, --block-size KB: Set block size in kilobytes.\n"
		"-i, --ignore pattern: Ignore wildcard pattern.\n"
		"-o, --output-name file: Set output filename or directory.\n"
		"-p, --private: Mark torrent private.\n"
		"-q, --quiet: Don't print progress indicator.\n"
		"-R, --rename-topdir name: Rename torrent top dir.\n"
	);
}

#define MAX_IGNORE_PATTERNS 256

static int blocksize = DEFAULT_BLOCKSIZE;
static int mark_private = 0;
static int quiet = 0;
static char *topdir_name = NULL;
static char *outpath = NULL;
static char *ignore[MAX_IGNORE_PATTERNS];
static int num_ignore_patterns = 0;

static void read_options(int argc, char *argv[])
{
	int ret;

	while ((ret = getopt_long(argc, argv, "b:i:o:pqR:", opts, NULL))
		!= -1)
	{
		if (ret == 'b') // set block size in KB
		{
			blocksize = atoi(optarg);
			if (blocksize < 1)
			{
				errx(1, "impossible block size: %d KB\n",
					blocksize);
			}
		}
		else if (ret == 'i') // ignore pattern
		{
			if (num_ignore_patterns == MAX_IGNORE_PATTERNS)
				errx(1, "too many ignore patterns");
			ignore[num_ignore_patterns++] = optarg;
		}
		else if (ret == 'o') // output file/dir
			outpath = optarg;
		else if (ret == 'p') // mark torrent as private
			mark_private = 1;
		else if (ret == 'q') // quiet: no progress indicator
			quiet = 1;
		else if (ret == 'R') // rename topdir
			topdir_name = optarg;
		else // ':' or '?'
			usage();
	}
}

int main(int argc, char *argv[])
{
	read_options(argc, argv);

	argc -= optind;
	argv += optind;

	
}
