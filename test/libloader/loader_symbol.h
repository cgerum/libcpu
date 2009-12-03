/*
 * Copyright (c) 2007-2008, Orlando Bassotto. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the distribution.
 *   * Neither the name of the author nor the names of its contributors
 *     may be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __loader_symbol_h
#define __loader_symbol_h

#include "loader_base.h"

typedef struct _loader_symbol {
	char const       *module;
	char const       *name;
	loader_size_t     file_offset;
	loader_uintptr_t  virtual_address;
	loader_uintptr_t  physical_address;
	loader_size_t     size;
	loader_uint32_t   type;
	loader_sint32_t   ordinal;
	loader_flags_t    flags;
} loader_symbol_t;

#define LOADER_SYMBOL_TYPE_INVALID  0
#define LOADER_SYMBOL_TYPE_FUNCTION 1
#define LOADER_SYMBOL_TYPE_OBJECT   2
#define LOADER_SYMBOL_TYPE_SECTION  3
#define LOADER_SYMBOL_TYPE_ANCHOR   4

#define LOADER_SYMBOL_FLAG_GLOBAL   1
#define LOADER_SYMBOL_FLAG_COMMON   2
#define LOADER_SYMBOL_FLAG_ABSOLUTE 4
#define LOADER_SYMBOL_FLAG_EXTERNAL 8
#define LOADER_SYMBOL_FLAG_WEAK     16
#define LOADER_SYMBOL_FLAG_ORDINAL  32

#endif  /* !__loader_symbol_h */
