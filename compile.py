#!/usr/bin/env python

import os
import sys
import platform

program = "stlviewer"

system_name = platform.system()

os.system("rm -f %s" % program)

src_dir = "."
modules = ["trackball.c", "stl.c", "stl_viewer.c"]
files = map(lambda module: src_dir + "/" + module, modules)
files_str = ' '.join(files)

includes = []
includes = map(lambda include: "-I" + include, includes)
include_str = ' '.join(includes)

libraries = ['glut', 'GL', 'm']
libraries = map(lambda library: "-l" + library, libraries)
libraries_str = ' '.join(libraries)

defines = ['_OPENGL_' , '_%s_' % system_name]
defines = map(lambda define: "-D" + define, defines)
defines_str = ' '.join(defines) 

if system_name == "Darwin":
	frameworks = ['GLUT', 'OpenGL']
        frameworks = map(lambda framework: "-framework " + framework, frameworks)
	framework_str =  ' '.join(frameworks)

if system_name == "Linux":
	compile_cmd = "gcc -Wall -o %s %s %s %s %s" % (program, files_str, include_str, defines_str, libraries_str)
elif system_name == "Darwin":
	compile_cmd = "cc -g -Wall -Wno-deprecated -o %s %s %s %s" % (program, files_str, defines_str, framework_str)


print compile_cmd
output = os.system(compile_cmd)
sys.exit(0)
