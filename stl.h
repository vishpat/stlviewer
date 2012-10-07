/*
 * Copyright (c) 2012, Vishal Patil 
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, 
 *    this list of conditions and the following disclaimer in the documentation 
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _STL_H_
#define _STL_H_

#define STL_DBG printf

typedef float STLFloat;
typedef float STLFloat32;

typedef unsigned char STLuint8;
typedef unsigned int STLuint32;
typedef unsigned int STLuint;

typedef struct stl_s stl_t;

typedef enum {
        STL_ERR_NONE,
        STL_ERR_LOAD,
        STL_ERR_FOPEN,
        STL_ERR_FILE_FORMAT,
        STL_ERR_MEM,
        STL_ERR_NOT_LOADED,
        STL_ERR_INVALID
} stl_error_t;

stl_t* stl_alloc(void);
stl_error_t stl_load(stl_t *, char *);
void stl_free(stl_t *);

STLFloat stl_max_x(stl_t *);
STLFloat stl_min_x(stl_t *);
STLFloat stl_max_y(stl_t *);
STLFloat stl_min_y(stl_t *);
STLFloat stl_max_z(stl_t *);
STLFloat stl_min_z(stl_t *);

STLuint stl_facet_cnt(stl_t *);
STLuint stl_vertex_cnt(stl_t *);

stl_error_t stl_vertices(stl_t *, STLFloat **points);

int stl_error_lineno(stl_t *);
#endif
