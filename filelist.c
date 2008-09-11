#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <assert.h>
#include "err.h"
#include "xm.h"

static char **fnams;
static int nfnams, sfnams;

static const char **ignores;
static int num_ignores;

// Recursively add a directory. prefix is the prefix used to store
// each name, as opposed to the real filename given by starting with
// dirname.
static void add_dir(const char *dirname, const char *prefix)
{
	DIR *dh;
	struct dirent *de;
	char *newprefix;
	char *newdirname;
	char *fname;

	if ((dh = opendir(dirname)) == NULL)
		err(1, "cannot open directory %s", dirname);

	while ((de = readdir(dh)) != NULL)
	{
		size_t namlen;
		int filetype;

		// TODO: check for wildcard match & skip

		filetype = de->d_type;
		namlen = strlen(de->d_name);

		// Handle symlinks to regular files (but not directories,
		// since that could get us stuck in a loop), and handle
		// "unknown" entries. On OpenBSD, I get "unknown" entries
		// when working with an ext2fs partition.

		// XXX: if the entry is "unknown" and turns out to be a
		// symlink, it won't be followed

		if (filetype == DT_UNKNOWN || filetype == DT_LNK)
		{
			struct stat info;

			fname = xm(1, strlen(dirname) + 1 + namlen + 1);
			sprintf(fname, "%s%s%s", dirname,
				strlen(dirname) == 0 ? "" : "/", de->d_name);

			if (stat(fname, &info) == -1)
				warnx("\rcan't stat file %s, skipping", fname);
			else if (filetype == DT_LNK && S_ISDIR(info.st_mode))
			{
				warnx("\rskipping symlinked directory %s",
					fname);
			}
			else if (S_ISDIR(info.st_mode))
				filetype = DT_DIR; // XXX
			else if (S_ISREG(info.st_mode))
				filetype = DT_REG; // XXX
			else
				warnx("\rskipping non-regular file %s", fname);

			free(fname);
			fname = NULL;
		}

		if (filetype == DT_REG)
		{
			fname = xm(1, strlen(prefix) + 1 + namlen + 1);
			sprintf(fname, "%s%s%s", prefix,
				strlen(prefix) == 0 ? "" : "/", de->d_name);
			XPND(fnams, nfnams, sfnams);
			fnams[nfnams++] = fname;
		}
		else if (filetype == DT_DIR)
		{
			if (strcmp(de->d_name, ".") == 0
				|| strcmp(de->d_name, "..") == 0)
			{
				continue;
			}

			newprefix = xm(1, strlen(prefix) + 1
				+ namlen + 1);
			sprintf(newprefix, "%s%s%s", prefix,
				strlen(prefix) == 0 ? "" : "/", de->d_name);

			newdirname = xm(1, strlen(dirname) + 1
				+ namlen + 1);
			sprintf(newdirname, "%s/%s", dirname, de->d_name);

			add_dir(newdirname, newprefix);
			free(newprefix);
			free(newdirname);
		}
		else
		{
			warnx("\rskipping non-regular file %s%s%s (type %d)",
				dirname,
				strlen(dirname) == 0 ? "" : "/",
				de->d_name,
				filetype);
		}
	}

	if (closedir(dh) == -1)
		err(1, "cannot close directory %s", dirname);
}

int mystrcmp(const void *one, const void *two)
{
	return strcmp(*(const char **)one, *(const char **)two);
}

#undef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))

// Version that compares extensions before rest of filenames.
int extstrcmp(const void *one, const void *two)
{
	const char *s1, *s2;
	const char *ext1, *ext2;
	const char *p;
	int ret;
	int x1, x2;

	s1 = *(const char **)one;
	s2 = *(const char **)two;

	// Compare paths first. x1, x2 = length of path component.

	x1 = 0;
	p = strrchr(s1, '/');
	if (p != NULL)
		x1 = p - s1 + 1;

	x2 = 0;
	p = strrchr(s1, '/');
	if (p != NULL)
		x2 = p - s1 + 1;

	ret = strncmp(s1, s2, MIN(x1, x2));
	if (ret != 0)
		return ret;

	// Path components match, so compare file extensions.

	ext1 = strrchr(s1, '.');
	if (ext1 == NULL) ext1 = s1;
	ext2 = strrchr(s2, '.');
	if (ext2 == NULL) ext2 = s2;

	ret = strcmp(ext1, ext2);
	if (ret != 0)
		return ret;

	// Extensions match, so compare whole filename.
	return strcmp(s1, s2);
}

void getfilelist(const char ***files, int *numfiles, const char *dirname,
	int sort_by_ext,
	const char **ignore_patterns, int num_ignore_patterns)
{
	assert(fnams == NULL);
	assert(nfnams == 0);
	assert(sfnams == 0);

	ignores = ignore_patterns;
	num_ignores = num_ignore_patterns;

	add_dir(dirname, "");

	qsort(fnams, nfnams, sizeof fnams[0],
		sort_by_ext ? extstrcmp : mystrcmp);

	*files = (const char **)fnams;
	*numfiles = nfnams;
}

void freefilelist(void)
{
	int ix;
	for (ix = 0; ix < nfnams; ix++)
		free(fnams[ix]);
	free(fnams);
	nfnams = sfnams = 0;
	fnams = NULL;
}
