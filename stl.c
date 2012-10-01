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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <float.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <unistd.h>

#include "stl.h"

#define STL_MAGIC 0xdeadbeef
#define STL_STR_SOLID_START "solid"
#define STL_STR_SOLID_END   "endsolid"
#define STL_STR_FACET_START "facet"
#define STL_STR_FACET_END "endfacet"
#define STL_STR_LOOP_START "outer"
#define STL_STR_LOOP_END "endloop"
#define STL_STR_VERTEX "vertex"

typedef enum {
        STL_FILE_TYPE_INVALID,
        STL_FILE_TYPE_TXT,
        STL_FILE_TYPE_BIN
} stl_file_type_t;

typedef enum stl_token {
        STL_TOKEN_INVALID,
        STL_TOKEN_SOLID_START,
        STL_TOKEN_SOLID_END,
        STL_TOKEN_FACET_START,
        STL_TOKEN_FACET_END,
        STL_TOKEN_LOOP_START,
        STL_TOKEN_LOOP_END,
        STL_TOKEN_VERTEX
} stl_token_t;

struct {
        const char *str;
        stl_token_t token;
} stl_token_map[] = {
        {STL_STR_SOLID_START, STL_TOKEN_SOLID_START},
        {STL_STR_SOLID_END, STL_TOKEN_SOLID_END},
        {STL_STR_FACET_START, STL_TOKEN_FACET_START},
        {STL_STR_FACET_END, STL_TOKEN_FACET_END},
        {STL_STR_LOOP_START, STL_TOKEN_LOOP_START},
        {STL_STR_LOOP_END, STL_TOKEN_LOOP_END},
        {STL_STR_VERTEX, STL_TOKEN_VERTEX}, 
};

#define STL_TOTAL_TOKENS (sizeof(stl_token_map)/sizeof(stl_token_map[0]))

typedef enum stl_state {
        STL_STATE_START,
        STL_STATE_SOLID_START,
        STL_STATE_SOLID_END,
        STL_STATE_FACET_START,
        STL_STATE_FACET_END,
        STL_STATE_LOOP_START,
        STL_STATE_LOOP_END,
        STL_STATE_VERTEX
} stl_state_t; 

struct stl_s {
        int magic;
        char *file;
        stl_file_type_t type;
        stl_state_t state;

        STLuint32 facet_cnt;
        STLuint vertex_cnt;

        STLfloat *vertices;
        STLfloat min_x;
        STLfloat max_x;
        STLfloat min_y;
        STLfloat max_y;
        STLfloat min_z;
        STLfloat max_z;

        int lineno;
        int loaded;
};

stl_t * 
stl_alloc(void) 
{
        stl_t *stl = NULL;
        stl = (stl_t *)malloc(sizeof(*stl));
        
        if (stl == NULL) {
                return NULL;
        }

        memset(stl, 0, sizeof(*stl));
        stl->magic = STL_MAGIC;
        return stl;
}

void 
stl_free(stl_t *stl) 
{
        if (stl) {
                
                assert(stl->magic == STL_MAGIC);

                if (stl->magic != STL_MAGIC) {
                        return;
                }

                if (stl->vertices) {
                        free(stl->vertices);
                }

                free(stl);
        }
}


static stl_token_t
stl_str_token(const char* str)
{
        stl_token_t token = STL_TOKEN_INVALID;
        int i = 0;

        for (i = 0; i < STL_TOTAL_TOKENS; i++) {
                
                if (strcasecmp(stl_token_map[i].str, str) == 0) {
                        token = stl_token_map[i].token; 
                        break;
                }
        }

        return token;
}
        
static stl_error_t 
stl_parse_txt(stl_t *stl)
{
        FILE *fp = fopen(stl->file, "r");
        char *str_token = NULL;
        stl_token_t token;
        int vertex_cnt = 0;
        int error = 1;
        stl_error_t ret = STL_ERR_NONE;

        if (fp == NULL) {
                return STL_ERR_FOPEN;
        }

        char buffer[256];

        while (fgets(buffer, sizeof(buffer), fp) != NULL) {
                
                stl->lineno++;
                str_token = strtok(buffer, " \r\n");

                /* Empty line */
                if (str_token == NULL) {
                        continue;
                }

                /* Check for invalid line format */
                token = stl_str_token(str_token);
                if (token == STL_TOKEN_INVALID) {
                        STL_DBG("Error while processing token %s\n", str_token);
                        goto err;
                }


                switch (token) {

                        case STL_TOKEN_SOLID_START:
                                stl->state = STL_STATE_SOLID_START;
                                break;
                        
                        case STL_TOKEN_SOLID_END:
                                error = 0;
                                stl->state = STL_STATE_SOLID_END;
                                break;
                        
                        case STL_TOKEN_FACET_START:

                                if (stl->state != STL_STATE_SOLID_START && 
                                    stl->state != STL_STATE_FACET_END) { 
                                        goto err;
                                }       
                                
                                stl->state = STL_STATE_FACET_START;
                                break;
                        
                        case STL_TOKEN_FACET_END:
                                if (stl->state != STL_STATE_LOOP_END) { 
                                        goto err;
                                }       
 
                                stl->state = STL_STATE_FACET_END;
                                stl->facet_cnt = stl->facet_cnt + 1;
                                break;

                        case STL_TOKEN_LOOP_START:

                                if (stl->state != STL_STATE_FACET_START) { 
                                        goto err;
                                }       
 
                                stl->state = STL_STATE_LOOP_START;
                                vertex_cnt = 0;
                                break;

                        case STL_TOKEN_LOOP_END:
                                if (stl->state != STL_STATE_VERTEX || 
                                    vertex_cnt != 3) {
                                        goto err;
                                }

                                stl->state = STL_STATE_LOOP_END;
                                break;

                        case STL_TOKEN_VERTEX:
                                if (stl->state != STL_STATE_VERTEX && 
                                    stl->state != STL_STATE_LOOP_START) {
                                        goto err;
                                }

                                stl->state = STL_STATE_VERTEX;
                                stl->vertex_cnt = stl->vertex_cnt + 1;
                                vertex_cnt += 1;
                                break;

                        default:
                                goto err;
                }


        }

err:
        ret = error ? STL_ERR_FILE_FORMAT : STL_ERR_NONE;
        
        fclose(fp);

        return ret;
}


static stl_error_t
stl_get_vertices(stl_t *stl) 
{
        FILE *fp = fopen(stl->file, "r");
        char *str_token = NULL;
        char *vx = NULL;
        char *vy = NULL;
        char *vz = NULL;
        STLfloat fvx, fvy, fvz;
        stl_token_t token;
        int vertex_idx = 0;
        int error = 1;
        stl_error_t ret = STL_ERR_NONE;

        if (fp == NULL) {
                return STL_ERR_FOPEN;
        }

        char buffer[256];

	stl->min_x = stl->min_y = stl->min_z = FLT_MAX;
	stl->max_x = stl->max_y = stl->max_z = FLT_MIN;

        while (fgets(buffer, sizeof(buffer), fp) != NULL) {
               
                str_token = strtok(buffer, " \r\n");

                /* Empty line */
                if (str_token == NULL) {
                        continue;
                }

                /* Check for invalid line format */
                token = stl_str_token(str_token);
                if (token == STL_TOKEN_INVALID) {
                        STL_DBG("Error while processing token %s\n", str_token);
                        goto err;
                }

                if (token == STL_TOKEN_VERTEX) {
                        vx = strtok(NULL, " ");

                        if (vx == NULL) {
                                goto err; 
                        }

                        fvx = strtof(vx, NULL);

                        if (fvx < stl->min_x) {
                                stl->min_x = fvx;
                        }
                        
                        if (fvx > stl->max_x) {
                                stl->max_x = fvx;
                        }
                        
                        stl->vertices[vertex_idx++] = fvx;

                        vy = strtok(NULL, " ");

                        if (vy == NULL) {
                                goto err;
                        }

                        fvy = strtof(vy, NULL);

                        if (fvy < stl->min_y) {
                                stl->min_y = fvy;
                        }
                        
                        if (fvy > stl->max_y) {
                                stl->max_y = fvy;
                        }
 
                        stl->vertices[vertex_idx++] = fvy;

                        vz = strtok(NULL, " ");

                        if (vz == NULL) {
                                goto err;
                        }

                        fvz = strtof(vz, NULL);

                        if (fvz < stl->min_z) {
                                stl->min_z = fvz;
                        }
                        
                        if (fvz > stl->max_z) {
                                stl->max_z = fvz;
                        }
 
                        stl->vertices[vertex_idx++] = fvz;
                }
        }

        error = 0;

err:
        ret = error ? STL_ERR_FILE_FORMAT : STL_ERR_NONE;
 
        fclose(fp);
        
        return ret;
}

static stl_file_type_t
stl_get_filetype(char *filename)
{
	stl_file_type_t type = STL_FILE_TYPE_INVALID;

	int fd = open(filename, O_RDONLY);
      	if (fd == -1) {
		return STL_FILE_TYPE_INVALID;
	} 

	STLuint8 c;
	int bytes_read = -1;
	while ((bytes_read = read(fd, &c, sizeof(c))) != 0 && c <= 127) {
	}

	type = (bytes_read == 0) ? STL_FILE_TYPE_TXT : STL_FILE_TYPE_BIN;

	close(fd);

	return type;
	
}

typedef struct {
	STLFloat32 x;
	STLFloat32 y;
	STLFloat32 z;
} stl_vector_t;

#define STL_BIN_HEADER_SIZE 80 
#define STL_TRIANGLE_VERTEX_CNT 3

static stl_error_t
stl_load_bin_file(stl_t *stl)
{
	stl_error_t err = STL_ERR_NONE;

	FILE *fp = fopen(stl->file, "r");
        if (fp == NULL) {
                return STL_ERR_FOPEN;
        }

	/* skip the stl file header */
	if (fseek(fp, STL_BIN_HEADER_SIZE, SEEK_CUR) != 0) {
		err = STL_ERR_FILE_FORMAT;
		goto done;		
	}	

	/* Read the the facet count */
	if (fread(&stl->facet_cnt, sizeof(stl->facet_cnt), 1, fp) != 1) {
		err = STL_ERR_FILE_FORMAT;
		goto done;
	} 
	
	int expected_vertex_cnt = stl->facet_cnt*3;

        stl->vertices = (STLfloat *)malloc(expected_vertex_cnt * 3 * sizeof(STLfloat));
	if (stl->vertices == NULL) {
		err = STL_ERR_MEM;
		goto done;
	}

	stl_vector_t vec;
        int vertex_idx = 0;
      	int triangle_idx = 0; 
	STLuint8 abc[2];	
	stl->min_x = stl->min_y = stl->min_z = FLT_MAX;
	stl->max_x = stl->max_y = stl->max_z = FLT_MIN;

	for (triangle_idx = 0; triangle_idx < stl->facet_cnt; triangle_idx++) {
	
		/* Read the normal vector */
		if (fread(&vec, sizeof(vec), 1, fp) != 1) {
			err = STL_ERR_FILE_FORMAT;
			goto done;
		} 	
		
		int idx = 0;
		for (idx = 0; idx < STL_TRIANGLE_VERTEX_CNT; idx++) {
			
			/* Read the vertex vector */
			if (fread(&vec, sizeof(vec), 1, fp) != 1) {
				err = STL_ERR_FILE_FORMAT;
				goto done;
			}

			stl->vertex_cnt++;
			stl->vertices[vertex_idx++] = vec.x;
			stl->vertices[vertex_idx++] = vec.y;
			stl->vertices[vertex_idx++] = vec.z;
			
			if (vec.x < stl->min_x) stl->min_x = vec.x;
			if (vec.x > stl->max_x) stl->max_x = vec.x;

			if (vec.y < stl->min_y) stl->min_y = vec.y;
			if (vec.y > stl->max_y) stl->max_y = vec.y;

			if (vec.z < stl->min_z) stl->min_z = vec.z;
			if (vec.z > stl->max_z) stl->max_z = vec.z;
		}
		
		/* Read the Attribute Byte Count */
		if (fread(&abc, sizeof(abc), 1, fp) != 1) {
			err = STL_ERR_FILE_FORMAT;
			goto done;
		} 	
	}	
	
	stl->loaded = 1;	
done:
	fclose(fp);
 	return err;
}

static stl_error_t
stl_load_txt_file(stl_t *stl)
{
        stl_error_t err = STL_ERR_NONE;
	
        if ((err = stl_parse_txt(stl)) != STL_ERR_NONE) {
                return err;
        }

        stl->vertices = (STLfloat *)malloc(3 * stl->vertex_cnt * sizeof(STLfloat));

        if (stl->vertices == NULL) {
                return STL_ERR_MEM;
        }

        if ((err = stl_get_vertices(stl)) != STL_ERR_NONE) {
                return err;
        }

        stl->loaded = 1;

	return err;
}

stl_error_t 
stl_load(stl_t *stl, char *filename)
{
        stl_error_t err = STL_ERR_NONE;
	stl_file_type_t type = STL_FILE_TYPE_INVALID;
        stl->file = filename;

	type = stl_get_filetype(filename);

	switch (type) {
	case STL_FILE_TYPE_TXT:
		err = stl_load_txt_file(stl);
		break;
	case STL_FILE_TYPE_BIN:
		err = stl_load_bin_file(stl);
		break;
	default:
		err = STL_ERR_FILE_FORMAT;
	}

        return err;
}
        

STLfloat 
stl_min_x(stl_t *stl)
{
        return stl->min_x;
}

STLfloat 
stl_max_x(stl_t *stl)
{
        return stl->max_x;
}

STLfloat 
stl_min_y(stl_t *stl)
{
        return stl->min_y;
}

STLfloat 
stl_max_y(stl_t *stl)
{
        return stl->max_y;
}

STLfloat 
stl_min_z(stl_t *stl)
{
        return stl->min_z;
}

STLfloat 
stl_max_z(stl_t *stl)
{
        return stl->max_z;
}

STLuint 
stl_facet_cnt(stl_t *stl)
{
        return stl->facet_cnt;
}

STLuint 
stl_vertex_cnt(stl_t *stl)
{
        return stl->vertex_cnt;
}

stl_error_t
stl_vertices(stl_t *stl, STLfloat **points)
{

        if (stl->loaded == 0) {
                return STL_ERR_NOT_LOADED;
        }

        *points = stl->vertices; 

        return STL_ERR_NONE;
}

int 
stl_error_lineno(stl_t *stl)
{
        return stl->lineno;
}
