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
#include "trackball.h"

#define MAX( x, y) (x) > (y) ? (x) : (y)
#define MIN( x, y) (x) < (y) ? (x) : (y)

#define DEFAULT_SCALE 1.0
#define DEFAULT_ROTATION_X 0
#define DEFAULT_ROTATION_Y 0
#define DEFAULT_ROTATION_Z 0
#define DEFAULT_ZOOM 1.0

#define MAX_Z_ORTHO_FACTOR 20
#define ROTATION_FACTOR 15

#define DARK_BG 0, 0, 0, 0
#define BRIGHT_BG 135.0/255.0, 206.0/255.0, 250.0/255.0, 0.0

static int rotating = 0;
static int wiremesh = 0;
static GLfloat scale = DEFAULT_SCALE;
static stl_t *stl;
static float ortho_factor = 1.5;
static float zoom = DEFAULT_ZOOM;

static int screen_width = 0;
static int screen_height = 0;

static float gobal_ambient_light[4] = {0.0, 0.0, 0.0, 0};
static float light_ambient[4] = {0.3, 0.3, 0.3, 0.0};
static float light_diffuse[4] = {0.5, 0.5, 0.5, 1.0};
static float light_specular[4] = {0.5, 0.5, 0.5, 1.0};
static float mat_shininess[] = {10.0};
static float mat_specular[] = { 0.5, 0.5, 0.5, 1.0 };

static float rot_cur_quat[4];
static float rot_last_quat[4];
static int rot_begin_x = 0;
static int rot_begin_y = 0;

static GLuint model;

typedef struct {
	GLfloat x;
	GLfloat y;
	GLfloat z;
} vector_t;

static void 
mouse_motion(int x, int y) 
{
        if (rotating) {
                trackball(rot_last_quat, 
                         (2.0 * rot_begin_x - screen_width) / screen_width,
                         (screen_height - 2.0 * rot_begin_y) / screen_height,
                         (2.0 * x - screen_width) / screen_width,
                         (screen_height - 2.0 * y) / screen_height); 
                rot_begin_x = x;
                rot_begin_y = y;
                add_quats(rot_last_quat, rot_cur_quat, rot_cur_quat);
        }
}

static void
mouse_click(int button, int state, int x, int y) 
{
        if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
                rotating = 1;
                rot_begin_x = x;
                rot_begin_y = y;
        }

        if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
                rotating = 0;
        }
}

static void
keyboardFunc(unsigned char key, int x, int y)
{
	switch (key) {
		case 'z':
                case 'Z':
			zoom += 0.2;
			break;
		case 'x':
                case 'X':
			zoom -= 0.2;
			break;
		case 'w':
                case 'W':
                        if (wiremesh == 0) {
			        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                                wiremesh = 1;
                        } else {
                                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                                wiremesh = 0;
                        }
			break;
                case 'r':
                case 'R':
                        scale = DEFAULT_SCALE;
                        trackball(rot_cur_quat, 0.0, 0.0, 0.0, 0.0);
                        zoom = DEFAULT_ZOOM;
                        break;
		case 'd':
		case 'D':
			glClearColor(DARK_BG);
			break;
		case 'b':
		case 'B':
			glClearColor(BRIGHT_BG);
			break;
		case 'q':
                case 'Q':
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
        
        screen_width = width;
        screen_height = height;

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
	normal.z = U.x * V.y - U.y * V.x;

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
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	glPushMatrix();

	glTranslatef(
		((stl_max_x(stl) + stl_min_x(stl))/2),
		((stl_max_y(stl) + stl_min_y(stl))/2),
		((stl_max_z(stl) + stl_min_z(stl))/2));


	glScalef(zoom, zoom, zoom);

        GLfloat rot_matrix[4][4];
        build_rotmatrix(rot_matrix, rot_cur_quat);
        glMultMatrixf(&rot_matrix[0][0]);

	glTranslatef(
		-((stl_max_x(stl) + stl_min_x(stl))/2),
		-((stl_max_y(stl) + stl_min_y(stl))/2),
		-((stl_max_z(stl) + stl_min_z(stl))/2));

	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular );
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

        glCallList(model);

        glPopMatrix();

	glFlush();
	glutSwapBuffers();
}

void
idle_func(void)
{
	glutPostRedisplay();
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
	GLfloat *vertices = NULL;
	GLuint triangle_cnt = 0;
	int i = 0, base = 0;

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

	err =  stl_vertices(stl, &vertices);
	if (err) {
		fprintf(stderr, "Problem getting the vertex array");
		exit(1);
	}

	triangle_cnt = stl_facet_cnt(stl);

        model = glGenLists(1);
        glNewList(model, GL_COMPILE);
        glBegin(GL_TRIANGLES);
	for (i = 0; i < triangle_cnt; i++) {
		base = i*18;
		drawTriangle(vertices[base], vertices[base + 1], vertices[base + 2],
			     vertices[base + 6], vertices[base + 7], vertices[base + 8],
			     vertices[base + 12], vertices[base + 13], vertices[base + 14]);
	}
	glEnd();
        glEndList();

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);

        glClearColor(0.0 / 255, 0.0 / 255.0, 0.0 / 255.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
  glutMotionFunc(mouse_motion); 
  glutMouseFunc(mouse_click);
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutIdleFunc(idle_func);
  init(argv[1]);
  trackball(rot_cur_quat, 0.0, 0.0, 0.0, 0.0);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
