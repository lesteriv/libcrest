#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_CONF=release_native
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/src/cr_xml.o \
	${OBJECTDIR}/src/cr_connection.o \
	${OBJECTDIR}/examples/http_mirror.o \
	${OBJECTDIR}/src/cr_string_map.o \
	${OBJECTDIR}/third/zlib/trees.o \
	${OBJECTDIR}/third/zlib/deflate.o \
	${OBJECTDIR}/src/cr_user_manager.o \
	${OBJECTDIR}/src/crest.o \
	${OBJECTDIR}/src/cr_utils.o \
	${OBJECTDIR}/src/auth_digest.o \
	${OBJECTDIR}/src/cr_mutex.o \
	${OBJECTDIR}/third/mongoose/mongoose.o \
	${OBJECTDIR}/examples/file_server.o \
	${OBJECTDIR}/src/cr_json.o \
	${OBJECTDIR}/examples/file_server_stl.o \
	${OBJECTDIR}/src/auth_basic.o


# C Compiler Flags
CFLAGS=-O3 -flto -fvisibility=hidden

# CC Compiler Flags
CCFLAGS=-fno-exceptions -fno-rtti -flto -Ofast -fvisibility=hidden
CXXFLAGS=-fno-exceptions -fno-rtti -flto -Ofast -fvisibility=hidden

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/librestpp.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/librestpp.a: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/librestpp.a
	${AR} -rv ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/librestpp.a ${OBJECTFILES} 
	$(RANLIB) ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/librestpp.a

${OBJECTDIR}/src/cr_xml.o: src/cr_xml.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -s -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cr_xml.o src/cr_xml.cpp

${OBJECTDIR}/src/cr_connection.o: src/cr_connection.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -s -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cr_connection.o src/cr_connection.cpp

${OBJECTDIR}/examples/http_mirror.o: examples/http_mirror.cpp 
	${MKDIR} -p ${OBJECTDIR}/examples
	${RM} $@.d
	$(COMPILE.cc) -s -MMD -MP -MF $@.d -o ${OBJECTDIR}/examples/http_mirror.o examples/http_mirror.cpp

${OBJECTDIR}/src/cr_string_map.o: src/cr_string_map.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -s -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cr_string_map.o src/cr_string_map.cpp

${OBJECTDIR}/third/zlib/trees.o: third/zlib/trees.cpp 
	${MKDIR} -p ${OBJECTDIR}/third/zlib
	${RM} $@.d
	$(COMPILE.cc) -s -MMD -MP -MF $@.d -o ${OBJECTDIR}/third/zlib/trees.o third/zlib/trees.cpp

${OBJECTDIR}/third/zlib/deflate.o: third/zlib/deflate.cpp 
	${MKDIR} -p ${OBJECTDIR}/third/zlib
	${RM} $@.d
	$(COMPILE.cc) -s -MMD -MP -MF $@.d -o ${OBJECTDIR}/third/zlib/deflate.o third/zlib/deflate.cpp

${OBJECTDIR}/src/cr_user_manager.o: src/cr_user_manager.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -s -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cr_user_manager.o src/cr_user_manager.cpp

${OBJECTDIR}/src/crest.o: src/crest.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -s -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/crest.o src/crest.cpp

${OBJECTDIR}/src/cr_utils.o: src/cr_utils.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -s -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cr_utils.o src/cr_utils.cpp

${OBJECTDIR}/src/auth_digest.o: src/auth_digest.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -s -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/auth_digest.o src/auth_digest.cpp

${OBJECTDIR}/src/cr_mutex.o: src/cr_mutex.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -s -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cr_mutex.o src/cr_mutex.cpp

${OBJECTDIR}/third/mongoose/mongoose.o: third/mongoose/mongoose.cpp 
	${MKDIR} -p ${OBJECTDIR}/third/mongoose
	${RM} $@.d
	$(COMPILE.cc) -s -MMD -MP -MF $@.d -o ${OBJECTDIR}/third/mongoose/mongoose.o third/mongoose/mongoose.cpp

${OBJECTDIR}/examples/file_server.o: examples/file_server.cpp 
	${MKDIR} -p ${OBJECTDIR}/examples
	${RM} $@.d
	$(COMPILE.cc) -s -MMD -MP -MF $@.d -o ${OBJECTDIR}/examples/file_server.o examples/file_server.cpp

${OBJECTDIR}/src/cr_json.o: src/cr_json.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -s -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cr_json.o src/cr_json.cpp

${OBJECTDIR}/examples/file_server_stl.o: examples/file_server_stl.cpp 
	${MKDIR} -p ${OBJECTDIR}/examples
	${RM} $@.d
	$(COMPILE.cc) -s -MMD -MP -MF $@.d -o ${OBJECTDIR}/examples/file_server_stl.o examples/file_server_stl.cpp

${OBJECTDIR}/src/auth_basic.o: src/auth_basic.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -s -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/auth_basic.o src/auth_basic.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/librestpp.a

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
