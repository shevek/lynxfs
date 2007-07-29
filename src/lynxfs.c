#define FUSE_USE_VERSION 26
#include <fuse.h>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


#include "lynxfs.h"

#define LYNXFS_BLOCK_SIZE	0x200
#define LYNXFS_INODE_BASE	0x400	/* Address of inode zero. */
#define LYNXFS_INODE_SIZE	0x80

#define LYNXFS_NAME_MAX		64		/* No fucking clue. */

struct lynxfs {
	char	*path;
	char	*data;
};

static long
lynxfs_read_raw(struct lynxfs *fs, void *buf, off_t off, size_t len)
{
	if (off < 0)
		return -EIO;
//	printf("Read raw fs %p, buf %p, off 0x%x, length %d\n",
//					fs, buf, off, len);
	memcpy(buf, &(fs->data[off]), len);
	return len;
}

struct lynxfs_inode {
/* data start */
    uint16_t     i_mode;
    uint16_t     i_links;
    uint16_t     unknown0;    /* usually 0 */
    uint16_t     unknown1;    /* usually 1 */
    uint32_t     unknown2;		/* owner/group? */
    uint32_t     i_length;

    /* atime, mtime, ctime in unknown order - a good guess */
    uint32_t     i_atime;
    uint32_t     i_mtime;
    uint32_t     i_ctime;

    uint32_t     i_blocks;

	char		 i_bdata[16 * 6];
/* data end */

/* metadata start */
	uint32_t	 i_num;		/* Not stored on disk. */
/* metadata end */

} __attribute__((packed));	/* Just in case we're 64-bit. */


#define LYNXFS_BLOCKS_DIRECT	10
#define LYNXFS_BLOCKS_INDIRECT	128
#define LYNXFS_ADDRS_PER_BLOCK	(LYNXFS_BLOCK_SIZE / sizeof(uint32_t))

#define _lynxfs_inode_directblock(in, index) \
	htons( *(uint16_t *)(&((in)->i_bdata[(index) * 3 + 1])) )

	/* XXX Fairly sure these two are wrong, but I don't know. */
#define _lynxfs_inode_majnum(in) \
	( *(uint8_t *)(&((in)->i_bdata[1])) )
#define _lynxfs_inode_minnum(in) \
	( *(uint8_t *)(&((in)->i_bdata[2])) )

static uint16_t
lynxfs_inode_blocknum(struct lynxfs *fs,
				struct lynxfs_inode *in, int index)
{
	uint16_t	block;

	if (index < LYNXFS_BLOCKS_DIRECT) {
//		printf("  index is %d, block is direct\n", index);
		block = _lynxfs_inode_directblock(in, index);
	}
	else if (index < LYNXFS_BLOCKS_DIRECT + LYNXFS_BLOCKS_INDIRECT) {
//		printf("  index is %d, block is INdirect\n", index);
		/* I assume the block numbers are uint32_t, rather than
		 * some magic followed by a uint16_t. */
		uint32_t	buf[LYNXFS_BLOCK_SIZE / sizeof(uint32_t)];
		int			index0;	/* Index in inode itself. */
		int			block0;
		int			index1;	/* Index in first indirect block. */
		long		ret;

		index = index - LYNXFS_BLOCKS_DIRECT;

		index0 = LYNXFS_BLOCKS_DIRECT +
						(index / LYNXFS_ADDRS_PER_BLOCK);
		index1 = index % LYNXFS_ADDRS_PER_BLOCK;
//		printf("  direct index is %d\n", index0);
//		printf("  index in indirect block is %x\n", index1);

		block0 = _lynxfs_inode_directblock(in, index0);
//		printf("  indirect block is %x\n", block0);

		ret = lynxfs_read_raw(fs, buf,
				block0 * LYNXFS_BLOCK_SIZE,
				LYNXFS_BLOCK_SIZE);
		if (ret < 0)
			return ret;

		block = htonl(buf[index1]);
	}
	else {
		uint32_t	buf[LYNXFS_BLOCK_SIZE / sizeof(uint32_t)];
		int			index0;	/* Index in inode itself. */
		int			index1;	/* Index in first indirect block. */
		int			index2;	/* Index in second indirect block. */
		int			block0;	/* Block number of first indirect block. */
		int			block1;	/* Block number of second indirect block. */
		long		ret;

		index = index - LYNXFS_BLOCKS_DIRECT;
		index = index - LYNXFS_BLOCKS_INDIRECT;

		index0 = LYNXFS_BLOCKS_DIRECT + 
				(LYNXFS_BLOCKS_INDIRECT / LYNXFS_ADDRS_PER_BLOCK) +
				(index / (LYNXFS_ADDRS_PER_BLOCK * LYNXFS_ADDRS_PER_BLOCK));
		index1 = (index / LYNXFS_ADDRS_PER_BLOCK);
		index2 = index % LYNXFS_ADDRS_PER_BLOCK;

		block0 = _lynxfs_inode_directblock(in, index0);
		ret = lynxfs_read_raw(fs, buf,
				block0 * LYNXFS_BLOCK_SIZE,
				LYNXFS_BLOCK_SIZE);
		if (ret < 0)
			return ret;

		block1 = htonl(buf[index1]);

		ret = lynxfs_read_raw(fs, buf,
				block1 * LYNXFS_BLOCK_SIZE,
				LYNXFS_BLOCK_SIZE);
		if (ret < 0)
			return ret;

		block = htonl(buf[index2]);
	}

	return block;
}

#define _lynxfs_inode_addr(inodenum) \
			(LYNXFS_INODE_BASE + (inodenum - 1) * LYNXFS_INODE_SIZE)

/*
static inline uint32_t
lynxfs_inode_blockcount(struct lynxfs *fs,
				struct lynxfs_inode *in)
{
	uint32_t	i_blocks = in->i_blocks;
	uint16_t	i_indirect;
	(void)fs;
	if (i_blocks <= LYNXFS_BLOCKS_DIRECT)
		return i_blocks;
	i_indirect =
		((i_blocks - LYNXFS_BLOCKS_DIRECT) / LYNXFS_ADDRS_PER_BLOCK) + 1;
	return i_blocks - i_indirect;
}
*/

static void
lynxfs_print_inode(struct lynxfs *fs, struct lynxfs_inode *in)
{
	unsigned int	 i, j;

	printf("Inode #%d, length %d, links %d, rawblocks %d\n",
					in->i_num, in->i_length, in->i_links, in->i_blocks);
	printf("  Inode at 0x%x\n", _lynxfs_inode_addr(in->i_num));

	i = 0;
	/* j is the count of all blocks consumed.
	 * j = 1 because the first block also consumes a block. */
	for (j = 1; j < in->i_blocks; j++) {
		printf("  Block %d, n=0x%x, blockaddr=0x%x (consumed=%d)\n",
				i,
				lynxfs_inode_blocknum(fs, in, i),
				lynxfs_inode_blocknum(fs, in, i) * LYNXFS_BLOCK_SIZE,
				j);
		i++;
		if (i < LYNXFS_BLOCKS_DIRECT) {
		}
		else if (i < LYNXFS_BLOCKS_DIRECT + LYNXFS_BLOCKS_INDIRECT) {
			int	tmp = i - LYNXFS_BLOCKS_DIRECT;
			if ((tmp % LYNXFS_ADDRS_PER_BLOCK) == 0)
				j++;
		}
		else {
			int	tmp = i - LYNXFS_BLOCKS_DIRECT;
			if ((tmp % LYNXFS_ADDRS_PER_BLOCK) == 0)
				j++;
			if ((tmp % (LYNXFS_ADDRS_PER_BLOCK * LYNXFS_ADDRS_PER_BLOCK)) == 0)
				j++;
		}

	}
}

static int
lynxfs_read_inode(struct lynxfs *fs,
				struct lynxfs_inode *in, int inodenum)
{
	long	 ret;

//	printf("Read inode %d\n", inodenum);
	ret = lynxfs_read_raw(fs, in,
			_lynxfs_inode_addr(inodenum),
			LYNXFS_INODE_SIZE);
	if (ret < 0)
		return ret;
	if (ret < LYNXFS_INODE_SIZE)
		return -EIO;

	in->i_num = inodenum;
	in->i_mode = htons(in->i_mode);
	in->i_links = htons(in->i_links);
	in->i_length = htonl(in->i_length);
	in->i_blocks = htonl(in->i_blocks);
	in->i_mtime = htonl(in->i_mtime);
	in->i_atime = htonl(in->i_atime);
	in->i_ctime = htonl(in->i_ctime);

	return 0;
}


static size_t
lynxfs_read_data(struct lynxfs *fs, void *buf,
				struct lynxfs_inode *in, off_t off, size_t len)
{
	size_t	out = 0;

//	printf("Reading data from inode %d, off %ld, len %zd\n",
//					in->i_num, (long)off, len);

	if (off > in->i_length)
		return 0;
	if (off + len > in->i_length)
		len = in->i_length - off;

	while (len > 0) {
		int		fileblocknum = off / LYNXFS_BLOCK_SIZE;
//		printf("  Reading block %d, remaining=%d\n", fileblocknum, len);
		int	blocknum = lynxfs_inode_blocknum(fs, in, fileblocknum);
		off_t	blockoff = off % LYNXFS_BLOCK_SIZE;
		off_t	rawoff = blocknum * LYNXFS_BLOCK_SIZE + blockoff;
		size_t	rawlen = ((blockoff + len) > LYNXFS_BLOCK_SIZE)
				? (LYNXFS_BLOCK_SIZE - blockoff)
				: len;
		long	ret;

//		printf("Reading data from block %d, rawoff 0x%lx, rawlen %zd\n",
//						blocknum, (long)rawoff, rawlen);

		ret = lynxfs_read_raw(fs, buf + out, rawoff, rawlen);
		if (ret < 0)
			return ret;
		/* Did we read enough data? */
		if ((size_t)ret < rawlen)
			return -EIO;

		off += rawlen;
		len -= rawlen;
		out += rawlen;
	}

//	printf("Finished reading %zd bytes\n", out);

	return out;
}


struct lynxfs_dentry {
	uint16_t	d_unknown;
	uint16_t	d_inode;
	uint16_t	d_entrylen;
	uint16_t	d_namelen;
	char		d_name[LYNXFS_NAME_MAX + 1];	/* Variable length. */
};

#define LYNXFS_DENTRY_PREFIXLEN \
	(offsetof(struct lynxfs_dentry, d_name))

static int
lynxfs_match_dentry(struct lynxfs *fs, struct lynxfs_dentry *de,
				const char *name, int namelen)
{
	(void)fs;
//	printf("Matching %s against %.*s\n", de->d_name, namelen, name);
	if (de->d_namelen != namelen)
		return -1;
	if (memcmp(de->d_name, name, namelen) != 0)
		return -1;
	return 0;
}

static void
lynxfs_read_dentry(struct lynxfs *fs, struct lynxfs_dentry *de,
				struct lynxfs_inode *in,
				int off)
{
	memset(de, 0, sizeof(struct lynxfs_dentry));
	lynxfs_read_data(fs, de, in, off, LYNXFS_DENTRY_PREFIXLEN);
	de->d_inode = htons(de->d_inode);
	de->d_entrylen = htons(de->d_entrylen);
	de->d_namelen = htons(de->d_namelen);
//	printf("Read dentry inode# %d, dlength %d\n",
//					de->d_inode, de->d_entrylen);
	lynxfs_read_data(fs, ((void *)de) + LYNXFS_DENTRY_PREFIXLEN, in,
					off + LYNXFS_DENTRY_PREFIXLEN,
					de->d_namelen + 1);
}

static int
lynxfs_find_dentry(struct lynxfs *fs,
				struct lynxfs_dentry *de,	/* out */
				struct lynxfs_inode *in,	/* in */
				const char *name, unsigned int namelen)
{
	unsigned int	 off;

//	printf("Searching for %s (%d)\n", name, namelen);

	off = 0;
	while (off < in->i_length) {
//		printf("Reading dentry at %d of %d in inode\n",
//						off, in->i_length);
		lynxfs_read_dentry(fs, de, in, off);
//		printf("Found dentry for %s inode %d\n",
//						de->d_name, de->d_inode);
		if (lynxfs_match_dentry(fs, de, name, namelen) == 0)
			return 0;
		off += de->d_entrylen;
	}

//	printf("Entry not found\n");

	return -1;
}



static int
lynxfs_find_inode(struct lynxfs *fs,
				struct lynxfs_inode *in,
				const char *path)
{
	struct lynxfs_dentry	 de;
	const char				*name;
	unsigned int			 namelen;

//	printf("Looking for file %s\n", path);

	lynxfs_read_inode(fs, in, 1);

	name = path;
	for (;;) {
		while (*name == '/')
			name++;
//		printf("Name is %s\n", name);
		if (*name == '\0')
			return 0;
		namelen = strcspn(name, "/");
		if (lynxfs_find_dentry(fs, &de, in, name, namelen) != 0) {
			// printf("Inode not found\n");
			return -1;
		}
		lynxfs_read_inode(fs, in, de.d_inode);
		name += namelen;
	}
}


static int
lynxfs_getattr(const char *path, struct stat *st)
{
	struct lynxfs		*fs;
	struct lynxfs_inode	 in;

	fs = fuse_get_context()->private_data;

	if (lynxfs_find_inode(fs, &in, path) != 0)
		return -ENOENT;

	printf("Found file %s\n", path);
	lynxfs_print_inode(fs, &in);

	memset(st, 0, sizeof(struct stat));
	st->st_ino = in.i_num;
	st->st_mode = in.i_mode;
	st->st_nlink = in.i_links;
	st->st_size = in.i_length;
	st->st_atime = in.i_atime;
	st->st_mtime = in.i_mtime;
	st->st_ctime = in.i_ctime;
	st->st_blocks = in.i_blocks;	/* incl. meta blocks */
	st->st_blksize = LYNXFS_BLOCK_SIZE;

	if (S_ISCHR(st->st_mode) || S_ISBLK(st->st_mode)) {
		st->st_rdev = (_lynxfs_inode_majnum(&in) << 8) |
				_lynxfs_inode_minnum(&in);
	}

	return 0;
}

static int
lynxfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
				off_t offset, struct fuse_file_info *fi)
{
	struct lynxfs		*fs;
	struct lynxfs_inode	 in;
	struct lynxfs_dentry de;
	unsigned int		 off;

	(void)offset;
	(void)fi;

	fs = fuse_get_context()->private_data;

	if (lynxfs_find_inode(fs, &in, path) != 0)
		return -ENOENT;

	off = 0;
	while (off < in.i_length) {
		lynxfs_read_dentry(fs, &de, &in, off);
		filler(buf, de.d_name, NULL, 0);
		off += de.d_entrylen;
	}

	return 0;
}

static int
lynxfs_readlink(const char *path, char *buf, size_t len)
{
	struct lynxfs		*fs;
	struct lynxfs_inode	 in;

	fs = fuse_get_context()->private_data;

	if (lynxfs_find_inode(fs, &in, path) != 0)
		return -ENOENT;

	lynxfs_read_data(fs, buf, &in, 0, len);

	return 0;
}


static int
lynxfs_open(const char *path, struct fuse_file_info *fi)
{
	struct lynxfs		*fs;
	struct lynxfs_inode	 in;

    (void)fi;

	printf("Opening file %s\n", path);

	fs = fuse_get_context()->private_data;

	if (lynxfs_find_inode(fs, &in, path) != 0)
		return -ENOENT;

	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	return 0;
}

static int
lynxfs_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
	struct lynxfs		*fs;
	struct lynxfs_inode	 in;
	size_t				 ret;

    (void)fi;

	printf("Reading from file %s, off %ld, len %zd\n",
					path, (long)offset, size);

	fs = fuse_get_context()->private_data;

	if (lynxfs_find_inode(fs, &in, path) != 0)
		return -ENOENT;

	ret = lynxfs_read_data(fs, buf, &in, offset, size);

	/* XXX Here is a horrible hack. */
	if (offset == 0 && size >= 4) {
		if (buf[1] == 'E' && buf[2] == 'L' && buf[3] == 'F') {
			if (buf[0] == '\x8F' || buf[0] == '\x9F') {
				buf[0] = '\x7f';
			}
		}
	}

	return ret;
}






enum lynxfs_opt_key {
	KEY_FILE
};

static struct fuse_opt lynxfs_opts[] = {
	{ "file=%s", offsetof(struct lynxfs, path), 1 },
	FUSE_OPT_END
};

static struct fuse_operations lynxfs_oper = {
    .getattr	= lynxfs_getattr,
    .readdir	= lynxfs_readdir,
    .readlink	= lynxfs_readlink,
    .open		= lynxfs_open,
    .read		= lynxfs_read,
};

static int
lynxfs_opt_proc(void *data, const char *arg, int key,
				struct fuse_args *outargs)
{
	char	**filep;

	(void)outargs;

	switch (key) {
		case KEY_FILE:
			filep = data;
			*filep = strdup(arg);
			break;
	}
	return 1;
}

int
main(int argc, char *argv[])
{
	struct fuse_args	 args = FUSE_ARGS_INIT(argc, argv);
	struct stat			 statbuf;
	struct lynxfs		 fs;
	int					 fd;
	int					 off;
	int					 len;

	memset(&fs, 0, sizeof(struct lynxfs));

	fuse_opt_parse(&args, &fs, lynxfs_opts, lynxfs_opt_proc);
	if (fs.path == NULL) {
		fprintf(stderr, "Missing required parameter file=\n");
		exit(1);
	}

	if (access(fs.path, R_OK) != 0) {
		fprintf(stderr, "Cannot open '%s' for read: %s\n",
						fs.path, strerror(errno));
		exit(1);
	}

	stat(fs.path, &statbuf);
	printf("File length is %ld\n", (long)statbuf.st_size);

	fs.data = malloc(statbuf.st_size);

	fd = open(fs.path, O_RDONLY);
	off = 0;
	while (off < statbuf.st_size) {
		len = read(fd, &(fs.data[off]), (statbuf.st_size - off));
		if (len == -1) {
			perror("read");
			exit(1);
		}
		off += len;
	}

/*
	{
		struct lynxfs_inode	 in;
		// lynxfs_find_inode(&fs, &in, "/");
		// lynxfs_find_inode(&fs, &in, "/etc");
		// lynxfs_find_inode(&fs, &in, "/etc/fstab");
		lynxfs_find_inode(&fs, &in, "/etc/arsehole");
	}
*/

    return fuse_main(args.argc, args.argv, &lynxfs_oper, &fs);
	// return 0;
}
