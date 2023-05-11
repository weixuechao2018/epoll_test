/*
 * Copyright (C) 2017 Jianhui Zhao <jianhuizhao329@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */
 
#ifndef _UTILS_H
#define _UTILS_H

#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>

#ifndef container_of
#define container_of(ptr, type, member)                 \
    ({                              \
        const __typeof__(((type *) NULL)->member) *__mptr = (ptr);  \
        (type *) ((char *) __mptr - offsetof(type, member));    \
    })
#endif

int get_nonce(uint8_t *dest, int len);
int parse_url(const char *url, char *host, int host_len,
    int *port, const char **path, bool *ssl);

int tcp_connect(const char *host, int port, int flags, bool *inprogress, int *eai);

#endif
