/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPLv2.
  See the file COPYING.
*/

/** @file
 *
 * minimal example filesystem using high-level API
 *
 * Compile with:
 *
 *     gcc -Wall hello.c `pkg-config fuse3 --cflags --libs` -o hello
 *
 * ## Source code ##
 * \include hello.c
 */

#define FUSE_USE_VERSION 31

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse3/fuse.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <exception>
#include <regex>
#include <sstream>

#include "common/service.h"
/*
 * Command line options
 *
 * We can't set default values for the char* fields here because
 * fuse_opt_parse would attempt to free() them when the user specifies
 * different values on the command line.
 */
#include <memory>
#include <string>
#include <vector>

#include "client/sdk.h"
#include "common/exception.h"
#include "service.pb.h"
using namespace spkdfs;
using namespace std;
class FUSE {
  SDK *sdk;

  uint32_t _index = 0;
  vector<Node> nodes;

public:
  FUSE(const string &ips) {
    nodes = parse_nodes(ips);
    init();
  };
  ~FUSE() { deinit(); }
  void deinit() {
    if (sdk != nullptr) {
      delete sdk;
      sdk = nullptr;
    }
  }
  void init_node(const Node &node) { sdk = new SDK(to_string(node)); }
  void init() {
    vector<Node> sub;
    sub.reserve(3);
    for (size_t i = 0; i < nodes.size(); i++) {
      if (sub.size() == 3) {
        node_discovery(sub);
        for (const Node &node : sub) {
          if (node.nodeStatus == NodeStatus::ONLINE) {
            init_node(node);
            return;
          }
        }
        sub.resize(0);
      }
      sub.push_back(nodes[i]);
    }
    if (sub.size() == 0) {
      throw runtime_error("all nodes offline");
    }
    node_discovery(sub);
    for (const Node &node : sub) {
      if (node.nodeStatus == NodeStatus::ONLINE) {
        init_node(node);
        return;
      }
    }
    throw runtime_error("all nodes offline");
  }
  void reinit() {
    deinit();
    init();
  }
  void mkdir(const string &dst) {
    cout << "libfuse mkdir :" << dst << endl;
    try {
      sdk->mkdir(dst);
    } catch (const spkdfs::MessageException &e) {
      return;
    } catch (const exception &e) {
      cout << e.what() << endl;
      reinit();
    }
    mkdir(dst);
  };
  void rm(const std::string &dst) {
    cout << "libfuse rm :" << dst << endl;
    try {
      sdk->rm(dst);
    } catch (const exception &e) {
      cout << dst << endl;
      cout << e.what() << endl;
      reinit();
    }
    rm(dst);
  }
  Inode ls(const std::string &dst) {
    cout << "libfuse ls :" << dst << endl;
    try {
      return sdk->ls(dst);
    } catch (const spkdfs::MessageException &e) {
      cout << to_string(e.errorMessage()) << endl;
      throw e;
    } catch (const exception &e) {
      cout << dst << endl;
      cout << e.what() << endl;
      reinit();
    }
    return ls(dst);
  }
  // void put(const std::string &src, const std::string &dst, const std::string &storage_type,
  //          unsigned int blocksize);
  // void get(const std::string &src, const std::string &dst);
};

static struct options {
  const char *ips;
  int show_help;
} options;

#define OPTION(t, p) \
  { t, offsetof(struct options, p), 1 }

static const struct fuse_opt option_spec[]
    = {OPTION("--ips=%s", ips), OPTION("-h", show_help), OPTION("--help", show_help), FUSE_OPT_END};
FUSE *fuse_ptr;
static void *spkdfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
  (void)conn;
  cfg->kernel_cache = 1;
  return NULL;
}

static int spkdfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
  (void)fi;
  cout << "spkdfs_getattr: " << path << endl;
  try {
    Inode inode = fuse_ptr->ls(path);
    memset(stbuf, 0, sizeof(struct stat));
    if (inode.is_directory) {
      stbuf->st_mode = S_IFDIR | 0755;
      stbuf->st_nlink = 2;
    } else {
      stbuf->st_mode = S_IFREG | 0444;
      stbuf->st_nlink = 1;
      stbuf->st_size = inode.filesize;
    }
  } catch (const MessageException &e) {
    return -ENOENT;
  }
  return 0;
}

static int spkdfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
                          struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
  filler(buf, ".", NULL, 0, fuse_fill_dir_flags::FUSE_FILL_DIR_NONE);
  filler(buf, "..", NULL, 0, fuse_fill_dir_flags::FUSE_FILL_DIR_NONE);
  // filler(buf, options.filename, NULL, 0, fuse_fill_dir_flags::FUSE_FILL_DIR_NONE);
  Inode inode = fuse_ptr->ls(path);
  if (!inode.is_directory) {
    cout << "not directory" << endl;
    throw runtime_error("not directory error");
  }
  for (auto str : inode.sub) {
    if (str.back() == '/') str.pop_back();
    filler(buf, str.c_str(), NULL, 0, fuse_fill_dir_flags::FUSE_FILL_DIR_NONE);
  }
  return 0;
}

static int spkdfs_open(const char *path, struct fuse_file_info *fi) {
  // if (strcmp(path + 1, options.filename) != 0) return -ENOENT;
  Inode inode = fuse_ptr->ls(path);
  if (inode.is_directory) {
    cout << "is directory" << endl;
    throw runtime_error("is directory error" + string(path));
  }

  // if ((fi->flags & O_ACCMODE) != O_RDONLY) return -EACCES;
  // try {
  // sdk->
  // } catch (declaration) {
  // statements
  // }
  return 0;
}

static int spkdfs_read(const char *path, char *buf, size_t size, off_t offset,
                       struct fuse_file_info *fi) {
  size_t len;

  // if (strcmp(path + 1, options.filename) != 0) return -ENOENT;

  // len = strlen(options.contents);
  // if (offset < len) {
  //   if (offset + size > len) size = len - offset;
  //   memcpy(buf, options.contents + offset, size);
  // } else
  //   size = 0;

  return size;
}
static int spkdfs_write(const char *, const char *, size_t size, off_t offset,
                        struct fuse_file_info *) {
  return size;
}
static const struct fuse_operations spkdfs_oper = {
    .getattr = spkdfs_getattr,  // 316
    .open = spkdfs_open,        // 441
    .read = spkdfs_read,        // 452
    .write = spkdfs_write,
    .readdir = spkdfs_readdir,  // 561
    .init = spkdfs_init,        // 583
};

static void show_help(const char *progname) {
  printf("usage: %s [options] <mountpoint>\n\n", progname);
  printf(
      "File-system specific options:\n"
      "    --ips=<s>          ips of the cluster\n"
      "                        (default: \"127.0.0.1:11801\")\n"
      "\n");
}

int main(int argc, char *argv[]) {
  int ret;
  struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

  /* Set defaults -- we have to use strdup so that
     fuse_opt_parse can free the defaults if other
     values are specified */
  options.ips = strdup("127.0.0.1:10801,127.0.0.1:11801\n");

  /* Parse options */
  if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1) return 1;

  /* When --help is specified, first print our own file-system
     specific help text, then signal fuse_main to show
     additional help (by adding `--help` to the options again)
     without usage: line (by setting argv[0] to the empty
     string) */
  if (options.show_help) {
    show_help(argv[0]);
    assert(fuse_opt_add_arg(&args, "--help") == 0);
    args.argv[0][0] = '\0';
  }
  fuse_ptr = new FUSE(options.ips);
  ret = fuse_main(args.argc, args.argv, &spkdfs_oper, NULL);
  fuse_opt_free_args(&args);
  return ret;
}
