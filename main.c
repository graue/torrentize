#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include "err.h"
#include "xm.h"
#include "torrent.h"

#define DEFAULT_PIECESIZE 256

const struct option opts[] =
{
	{ "piece-size",		required_argument,	NULL, 'b' },
	{ "sort-by-extensions",	no_argument,		NULL, 'E' },
	{ "ignore",		required_argument,	NULL, 'i' },
	{ "output-name",	required_argument,	NULL, 'o' },
	{ "private",		no_argument,		NULL, 'p' },
	{ "quiet",		no_argument,		NULL, 'q' },
	{ "rename",		required_argument,	NULL, 'R' }
};

static void usage(void)
{
	fprintf(stderr,
		"usage: torrentize [options] tracker_URL ... file ...\n"
		"\n"
		"-b, --piece-size KB: Set piece size in kilobytes.\n"
		"-E, --sort-by-extensions: Sort by file extensions.\n"
		"-i, --ignore pattern: Ignore wildcard pattern.\n"
		"-o, --output-name file: Set output filename or directory.\n"
		"-p, --private: Mark torrent private.\n"
		"-q, --quiet: Don't print progress indicator.\n"
		"-R, --rename name: Rename file or top dir for torrent.\n"
	);
	exit(1);
}

#define MAX_IGNORE_PATTERNS 256

static int piecesize = DEFAULT_PIECESIZE;
static int mark_private = 0;
static int quiet = 0;
static int sort_by_ext = 0;
static char *newname = NULL;
static char *outpath = NULL;
static char *ignore_patterns[MAX_IGNORE_PATTERNS];
static int num_ignore_patterns = 0;

static char **tracker_urls = NULL;
static int num_tracker_urls = 0;
static char **input_files = NULL;
static int num_input_files = 0;

static void read_options(int argc, char *argv[])
{
	int ret;

	while ((ret = getopt_long(argc, argv, "b:Ei:o:pqR:", opts, NULL))
		!= -1)
	{
		if (ret == 'b') // set piece size in KB
		{
			piecesize = atoi(optarg);
			if (piecesize < 1)
			{
				errx(1, "impossible piece size: %d KB\n",
					piecesize);
			}
		}
		else if (ret == 'E') // sort by extensions
			sort_by_ext = 1;
		else if (ret == 'i') // ignore pattern
		{
			if (num_ignore_patterns == MAX_IGNORE_PATTERNS)
				errx(1, "too many ignore patterns");
			ignore_patterns[num_ignore_patterns++] = optarg;
		}
		else if (ret == 'o') // output file/dir
			outpath = optarg;
		else if (ret == 'p') // mark torrent as private
			mark_private = 1;
		else if (ret == 'q') // quiet: no progress indicator
			quiet = 1;
		else if (ret == 'R') // rename topdir or file
			newname = optarg;
		else // ':' or '?'
			usage();
	}
}

static void read_args(int argc, char *argv[])
{
	argc -= optind;
	argv += optind;

	tracker_urls = argv;
	while (argc > 0 && (strncmp(*argv, "http://", 7) == 0
			|| strncmp(*argv, "udp://", 6) == 0))
	{
		num_tracker_urls++;
		argv++;
		argc--;
	}

	if (num_tracker_urls == 0)
	{
		warnx("no tracker URL given");
		usage();
	}

	input_files = argv;
	num_input_files = argc;

	if (num_input_files == 0)
	{
		warnx("no input file given");
		usage();
	}
}

static void do_torrent(const char *inputfile)
{
	char *outfile;
	char *realinputfile;
	char *p;
	const char *renamedname;
	struct stat info;

	// duplicate input file name, removing any trailing slashes
	realinputfile = xsd(inputfile);
	p = realinputfile + strlen(realinputfile) - 1;
	while (p > realinputfile && *p == '/')
	{
		*p = '\0';
		p--;
	}
	if (realinputfile[0] == '\0')
	{
		warnx("ignoring empty argument");
		return;
	}
	if (realinputfile[0] == '/' && realinputfile[1] == '\0')
		errx(1, "won't torrent the root directory");

	if (newname != NULL)
		renamedname = newname;
	else
	{
		renamedname = strrchr(realinputfile, '/');
		if (renamedname == NULL)
			renamedname = realinputfile;
		else
		{
			renamedname++;
			assert(renamedname[0] != '\0');
		}
	}

	if (outpath == NULL)
	{
		outfile = xm(1, strlen(renamedname) + strlen(".torrent") + 1);
		strcpy(outfile, renamedname);
		strcat(outfile, ".torrent");
	}
	else if (num_input_files == 1 && (stat(outpath, &info) == -1
		|| S_ISREG(info.st_mode)))
	{
		// One input file, and outpath is either nonexistent thus far,
		// or a regular file to be overwritten. Thus treat it as a
		// filename.
		outfile = xsd(outpath);
	}
	else // outpath is a directory to place torrent files in
	{
		const char *inputfile_nameonly;

		inputfile_nameonly = strrchr(renamedname, '/');
		if (inputfile_nameonly == NULL)
			inputfile_nameonly = renamedname;
		else
			inputfile_nameonly++;

		outfile = xm(1, strlen(outpath) + 1
			+ strlen(inputfile_nameonly)
			+ strlen(".torrent") + 1);
		strcpy(outfile, outpath);
		if (outfile[strlen(outfile) - 1] != '/')
			strcat(outfile, "/");
		strcat(outfile, inputfile_nameonly);
		strcat(outfile, ".torrent");
	}

	if (!quiet)
		fprintf(stderr, "%s:\n", outfile);

	create_torrent(outfile, inputfile, renamedname, piecesize, mark_private,
		quiet, sort_by_ext,
		num_tracker_urls, (const char *const *)tracker_urls,
		num_ignore_patterns, (const char *const *)ignore_patterns);

	free(outfile);
	free(realinputfile);
}

int main(int argc, char *argv[])
{
	int ix;

	if (argc == 1)
		usage();

	read_options(argc, argv);
	read_args(argc, argv);

	for (ix = 0; ix < num_input_files; ix++)
	{
		do_torrent(input_files[ix]);
		if (ix < num_input_files - 1)
			putc('\n', stderr);
	}

	return 0;
}
