#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "err.h"
#include "xm.h"
#include "sha1lib.h"
#include "filelist.h"

static unsigned char **pieces;
static int npieces, spieces;

static FILE *out;

// Macros for writing strings and ints, bencoded, into the output file.
#define fbenc_str(s) fprintf(out, "%d:%s", (int)strlen(s), s)
#define fbenc_int(ll) fprintf(out, "i%llde", (long long)ll)

// For raw writing into the output file.
#define fwr(s) fputs(s, out)

// Start a list; start a dictionary; end a list or dictionary.
#define fbenc_list fwr("l")
#define fbenc_dict fwr("d")
#define fbenc_end fwr("e")

// The filename of the torrent we're currently writing,
// for diagnostic purposes.
static const char *activeoutfile;

static int mark_private;
static int be_quiet;
static int sort_by_ext;
static int piece_bytes;
static const char *newname;

// Length so far of the piece being constructed.
static int thispiece_len;

// Piece being constructed so far.
// Can be NULL if thispiece_len == 0.
static unsigned char *thispiece;

// Write a path as a list of the components.
static void fbenc_path(const char *path)
{
	char *copy;
	char *next;
	char *p;

	p = copy = xsd(path);

	fbenc_list;
	while ((next = strchr(p, '/')) != NULL)
	{
		*next = '\0';
		fbenc_str(p);
		p = next + 1;
	}
	fbenc_str(p);
	fbenc_end;

	free(copy);
}

// Add hash of the piece being constructed in memory.
static void add_this_piece(void)
{
	unsigned char *digest;

	XPND(pieces, npieces, spieces);
	digest = xm(1, SHA1_DIGEST_LENGTH);
	SHA1Data(digest, thispiece, thispiece_len);
	pieces[npieces++] = digest;
}

// Add pieces from a file.
// The second filename is only for display purposes.
static void add_pieces_from_file(const char *filename,
	const char *displayfilename)
{
	FILE *infp;
	int wantedbytes;
	int ret;

	if (thispiece == NULL)
	{
		assert(thispiece_len == 0);
		thispiece = xm(1, piece_bytes);
	}

	infp = fopen(filename, "rb");
	if (infp == NULL)
		err(1, "cannot open %s", filename);

	if (!be_quiet)
		fprintf(stderr, "  adding: %s", displayfilename);

	wantedbytes = piece_bytes - thispiece_len;
	while ((ret = fread(&thispiece[thispiece_len], 1, wantedbytes, infp))
		> 0)
	{
		assert(ret <= wantedbytes);

		thispiece_len += ret;
		wantedbytes -= ret;

		if (thispiece_len == piece_bytes)
		{
			add_this_piece();
			thispiece_len = 0;
			wantedbytes = piece_bytes;
		}
	}

	if (!be_quiet)
		putc('\n', stderr);

	if (ferror(infp))
		err(1, "error reading %s", filename);
	fclose(infp);
}

// Add the final piece, in case the torrent isn't an exact multiple
// of the piece size.
static void finalize_pieces(void)
{
	if (thispiece_len > 0)
		add_this_piece();
	if (thispiece != NULL)
	{
		free(thispiece);
		thispiece = NULL;
	}
	thispiece_len = 0;
}

// Write the pieces' hashes to the torrent file.
static void write_pieces(void)
{
	int ix;
	char buf[50];

	fbenc_str("pieces");
	snprintf(buf, sizeof buf, "%lld:", (long long)npieces * 20);
	fwr(buf);
	for (ix = 0; ix < npieces; ix++)
	{
		if (fwrite(pieces[ix], 20, 1, out) < 1)
			err(1, "error writing to %s", activeoutfile);
	}
}

// Free/reset the pieces.
static void free_pieces(void)
{
	int ix;

	for (ix = 0; ix < npieces; ix++)
		free(pieces[ix]);

	free(pieces);
	npieces = spieces = 0;
	pieces = NULL;
}

// Write info dictionary for a single file. The struct stat is passed along
// for convenience.
static void write_singlefile_info(const char *filename, const struct stat *sb)
{
	fbenc_dict;

	fbenc_str("length");
	fbenc_int(sb->st_size);

	fbenc_str("name");
	fbenc_str(newname);

	fbenc_str("piece length");
	fbenc_int(piece_bytes);

	add_pieces_from_file(filename, filename);
	finalize_pieces();
	write_pieces();
	free_pieces();

	if (mark_private)
	{
		fbenc_str("private");
		fbenc_int(1);
	}

	fbenc_end;
}

// Write info dictionary for a multi-file torrent.
static void write_multifile_info(const char *dirname,
	int num_ignore_patterns, const char **ignore_patterns)
{
	const char **files;
	int numfiles;
	int ix;
	struct stat info;
	char *fullfilename;

	getfilelist(&files, &numfiles, dirname, sort_by_ext,
		ignore_patterns, num_ignore_patterns);

	fbenc_dict;

	fbenc_str("files");
	fbenc_list;
	for (ix = 0; ix < numfiles; ix++)
	{
		fullfilename = xm(1, strlen(dirname) + 1 + strlen(files[ix])
			+ 1);
		strcpy(fullfilename, dirname);
		strcat(fullfilename, "/");
		strcat(fullfilename, files[ix]);

		if (stat(fullfilename, &info) != 0)
			err(1, "cannot stat %s", fullfilename);

		fbenc_dict;

		fbenc_str("length");
		fbenc_int(info.st_size);

		fbenc_str("path");
		fbenc_path(files[ix]);

		fbenc_end;

		add_pieces_from_file(fullfilename, files[ix]);
		free(fullfilename);
	}
	fbenc_end; // end the list of files

	freefilelist();

	fbenc_str("name");
	fbenc_str(newname);

	fbenc_str("piece length");
	fbenc_int(piece_bytes);

	finalize_pieces();
	write_pieces();
	free_pieces();

	if (mark_private)
	{
		fbenc_str("private");
		fbenc_int(1);
	}

	fbenc_end;
}

void create_torrent(const char *filename, const char *inputfile,
	const char *rename, int piecesize, int private, int quiet,
	int sortext,
	int num_tracker_urls, const char **tracker_urls,
	int num_ignore_patterns, const char **ignore_patterns)
{
	struct stat info;
	int ix;

	activeoutfile = filename;
	mark_private = private;
	be_quiet = quiet;
	sort_by_ext = sortext;
	piece_bytes = piecesize * 1024;
	newname = rename != NULL ? rename : inputfile;

	out = fopen(filename, "wb");
	if (out == NULL)
		err(1, "cannot create %s", filename);

	fbenc_dict;

	fbenc_str("announce");
	fbenc_str(tracker_urls[0]);

	if (num_tracker_urls > 1)
	{
		// XXX only support each tracker being put in its own tier
		// for the moment

		fbenc_str("announce-list");
		fbenc_list;
		for (ix = 0; ix < num_tracker_urls; ix++)
		{
			fbenc_list;
			fbenc_str(tracker_urls[ix]);
			fbenc_end;
		}
		fbenc_end;
	}

	fbenc_str("info");

	if (stat(inputfile, &info) != 0)
		err(1, "cannot stat %s", inputfile);

	if (!S_ISDIR(info.st_mode))
		write_singlefile_info(inputfile, &info);
	else
	{
		write_multifile_info(inputfile, num_ignore_patterns,
			ignore_patterns);
	}

	fbenc_end;

	fclose(out);
}
