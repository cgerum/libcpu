#ifndef __nix_fd_h
#define __nix_fd_h

#include "nix-env.h"

int
nix_fd_init(size_t count);

int
nix_fd_alloc(int fd, int *err);

int
nix_fd_alloc_at(int gfd, int fd, int *err);

int
nix_fd_release(int fd, int *err);

int
nix_fd_get(int fd);

int
nix_fd_get_nearest(nix_env_t *env, int fd, int dir);


#endif  /* !__nix_fd_h */
