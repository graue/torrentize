Torrentize creates BitTorrent metainformation files.



Usage:

torrentize [options] tracker_URL ... file ...


Each file may be either a single file or a directory. The
tracker_URLs and files are distinguished by the fact that a
tracker_URL always starts with http://, https:// or udp://.

The default name for a torrent file is the source file or
directory with ".torrent" appended.



Options:


-b, --piece-size KB:
  Set piece size in kilobytes. The default is 256 KB.


(NOT IMPLEMENTED YET. IGNORE THIS ONE FOR NOW.)
-i, --ignore pattern:
  Ignore files matching the given wildcard pattern (for
example, *.txt). Matching is done case-sensitively. This
option only applies to torrentizing directories, not single
files.


-o, --output-name file:
  Set output path. If only one input file is given, and this
path is not a preexisting directory, it is used as the
filename for the torrent metafile created. Otherwise, this
is taken to be a directory to place all created torrent files
in.


-p, --private:
  Mark torrent as private. This will prevent clients from
using Peer Exchange or DHT; instead they will get clients
only from the tracker(s) given in the metainfo file.


-q, --quiet:
  Don't print a progress indicator.


-R, --rename name:
  Rename file or (if the input file is a directory) top dir
for torrent. By default the real on-disk file name or
directory name is used. This value in the .torrent file is
purely informational.



Info:


Torrentize was written by Graue <graue@oceanbase.org>. It is
in the public domain.
