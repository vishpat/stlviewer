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


#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#ifdef _Linux_
#include <GL/glut.h>
#endif

#ifdef _Darwin_
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#endif

#include "stl.h"

#define MAX( x, y) (x) > (y) ? (x) : (y)
#define MIN( x, y) (x) < (y) ? (x) : (y)

#define MAX_Z_ORTHO_FACTOR 20

static float spin = 0.0;
static GLfloat scale = 1.0;
static stl_t *stl;
static float ortho_factor = 1.5;
static int change_delay = 10;

float gobal_ambient_light[4] = {0.0, 0.0, 0.0, 0};

float light_ambient[4] = {0.3, 0.3, 0.3, 0.0};
float light_diffuse[4] = {0.5, 0.5, 0.5, 1.0};
float light_specular[4] = {0.5, 0.5, 0.5, 1.0};

float mat_shininess[] = {10.0};	
float mat_specular[] = { 0.5, 0.5, 0.5, 1.0 };

float zoom = 1.0;

#define ROTATION_FACTOR 15
float rot_x = 0;
float rot_y = 0;
float rot_z = 0;

typedef struct {
	GLfloat x;
	GLfloat y;
	GLfloat z;
} vector_t;

static void
keyboardSpecialFunc(int key, int x, int y)
{
	switch (key) {
		case GLUT_KEY_UP:
			rot_x -= ROTATION_FACTOR;
			break;		
		
		case GLUT_KEY_DOWN:
			rot_x += ROTATION_FACTOR;
			break;	
		
		case GLUT_KEY_LEFT:
			rot_z -= ROTATION_FACTOR;
			break;		
		
		case GLUT_KEY_RIGHT:
			rot_z += ROTATION_FACTOR;
			break;
		
		case GLUT_KEY_PAGE_UP:
			rot_y -= ROTATION_FACTOR;
			break;	
	
		case GLUT_KEY_PAGE_DOWN:
			rot_y += ROTATION_FACTOR;
			break;	

		default:
			break;
	}
}

static void
keyboardFunc(unsigned char key, int x, int y)
{
	switch (key) {
		case 'z':
			zoom += 0.2;
			break;
		case 'x':
			zoom -= 0.2;
			break;
		case 'w':
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			break;
		case 'f':
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			break;
		case 'q':
			exit(0);	
			break;
		default:
			break;
		
	}
}

static void 
ortho_dimensions(GLfloat *min_x, GLfloat *max_x,
                GLfloat *min_y, GLfloat *max_y,
                GLfloat *min_z, GLfloat *max_z)
{
	GLfloat diff_x = stl_max_x(stl) - stl_min_x(stl);
	GLfloat diff_y = stl_max_y(stl) - stl_min_y(stl);
	GLfloat diff_z = stl_max_z(stl) - stl_min_z(stl);

        GLfloat max_diff = MAX(MAX(diff_x, diff_y), diff_z);

        *min_x = stl_min_x(stl) - ortho_factor*max_diff; 
	*max_x = stl_max_x(stl) + ortho_factor*max_diff;
	*min_y = stl_min_y(stl) - ortho_factor*max_diff; 
	*max_y = stl_max_y(stl) + ortho_factor*max_diff;
	*min_z = stl_min_z(stl) - MAX_Z_ORTHO_FACTOR * ortho_factor*max_diff; 
	*max_z = stl_max_z(stl) + MAX_Z_ORTHO_FACTOR * ortho_factor*max_diff;
}

static void
reshape(int width, int height) 
{
	int size = MIN(width, height);
        GLfloat min_x, min_y, min_z, max_x, max_y, max_z;

	int width_half = width / 2;
	int height_half = height / 2;
	
	glViewport(width_half - (size/2), height_half - (size/2) , 
			size, size);	

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

        ortho_dimensions(&min_x, &max_x, &min_y, &max_y, &min_z, &max_z);
	glOrtho(min_x, max_x, min_y, max_y, min_z, max_z);
	
	glMatrixMode(GL_MODELVIEW);
        
        glLoadIdentity();
	// Set global ambient light
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, gobal_ambient_light);	

	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
        float Lt0pos[] = {0, 0, 0, 1};
	glLightfv(GL_LIGHT0, GL_POSITION, Lt0pos);

	glLightfv(GL_LIGHT1, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT1, GL_SPECULAR, light_specular);
        float Lt1pos[] = {max_x, max_y, max_z, 0};
	glLightfv(GL_LIGHT1, GL_POSITION, Lt1pos);
        
        glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
        glEnable(GL_LIGHT1);
}


static void 
drawTriangle(	GLfloat x1, GLfloat y1, GLfloat z1,
		GLfloat x2, GLfloat y2, GLfloat z2,
		GLfloat x3, GLfloat y3, GLfloat z3)
{
	vector_t normal;
	vector_t U;
	vector_t V;
	GLfloat length;

	U.x = x2 - x1;
	U.y = y2 - y1;
	U.z = z2 - z1;

	V.x = x3 - x1;
	V.y = y3 - y1;
	V.z = z3 - z1;

	normal.x = U.y * V.z - U.z * V.y;
	normal.y = U.z * V.x - U.x * V.z;
	normal.x = U.x * V.y - U.y * V.x;

	length = normal.x * normal.x + normal.y * normal.y + normal.z * normal.z;
	length = sqrt(length);

	glNormal3f(normal.x / length, normal.y / length, normal.z / length);
	glVertex3f(x1, y1, z1);
	glVertex3f(x2, y2, z2);
	glVertex3f(x3, y3, z3);
}

void
drawBox(void)
{
	GLfloat *vertices = NULL;

	stl_error_t err =  stl_vertices(stl, &vertices);
	GLuint triangle_cnt = stl_facet_cnt(stl);
	int i = 0, base = 0;

	if (err) {
		fprintf(stderr, "Problem getting the vertex array");
		exit(1);
	}
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	
	glPushMatrix();

	glTranslatef(
		((stl_max_x(stl) + stl_min_x(stl))/2),
		((stl_max_y(stl) + stl_min_y(stl))/2),
		((stl_max_z(stl) + stl_min_z(stl))/2));		


	glScalef(zoom, zoom, zoom);

	glRotatef(rot_x, 1, 0, 0);
	glRotatef(rot_y, 0, 1, 0);
	glRotatef(rot_z, 0, 0, 1);

	glTranslatef(
		-((stl_max_x(stl) + stl_min_x(stl))/2),
		-((stl_max_y(stl) + stl_min_y(stl))/2),
		-((stl_max_z(stl) + stl_min_z(stl))/2));		

	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular );
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
	
        glBegin(GL_TRIANGLES);
	for (i = 0; i < triangle_cnt; i++) {
		base = i*9;
		drawTriangle(vertices[base], vertices[base + 1], vertices[base + 2],
			     vertices[base + 3], vertices[base + 4], vertices[base + 5],
			     vertices[base + 6], vertices[base + 7], vertices[base + 8]);
	} 
	glEnd();

	glPopMatrix();

	spin  += 1;

	glFlush();
	glutSwapBuffers();
}

void
timer(int extra)
{
	glutPostRedisplay();
	glutTimerFunc(change_delay, timer, 0);
}


void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  drawBox();
}

void
init(char *filename)
{
	stl_error_t err;

 	stl = stl_alloc();
	if (stl == NULL) {
		fprintf(stderr, "Unable to allocate memoryfor the stl object");
		exit(1);
	}

	err = stl_load(stl, filename);
	
	if (err != STL_ERR_NONE) {
		fprintf(stderr, "Problem loading the stl file, check lineno %d\n", 
			stl_error_lineno(stl));
		exit(1);
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);
	
        glClearColor(135.0 / 255, 206.0 / 255.0, 250.0 / 255.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glColor3f(120.0 / 255.0 , 120.0 / 255.0, 120.0 / 255.0);

        glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);	


	glFlush();
}

int
main(int argc, char **argv)
{

  if (argc != 2) {
	fprintf(stderr, "%s <stl file>\n", argv[0]);
	exit(1);
  }
	
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutCreateWindow(argv[1]);
  glutKeyboardFunc(keyboardFunc);	
  glutSpecialFunc(keyboardSpecialFunc);	
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutTimerFunc(change_delay, timer, 0);
  init(argv[1]);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
