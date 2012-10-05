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
CND_CONF=examples_debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/src/utils.o \
	${OBJECTDIR}/third/zlib/trees.o \
	${OBJECTDIR}/third/zlib/deflate.o \
	${OBJECTDIR}/src/crest_user_manager.o \
	${OBJECTDIR}/src/crest.o \
	${OBJECTDIR}/src/auth_digest.o \
	${OBJECTDIR}/examples/proc.o \
	${OBJECTDIR}/third/mongoose/mongoose.o \
	${OBJECTDIR}/src/crest_connection.o \
	${OBJECTDIR}/src/auth_basic.o


# C Compiler Flags
CFLAGS=-Wall -Wextra

# CC Compiler Flags
CCFLAGS=-Wall -Wextra -fno-rtti -fno-exceptions
CXXFLAGS=-Wall -Wextra -fno-rtti -fno-exceptions

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-lpthread -ldl

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/proc

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/proc: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	gcc -fno-exceptions -fno-rtti -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/proc ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/src/utils.o: src/utils.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/utils.o src/utils.cpp

${OBJECTDIR}/third/zlib/trees.o: third/zlib/trees.cpp 
	${MKDIR} -p ${OBJECTDIR}/third/zlib
	${RM} $@.d
	$(COMPILE.cc) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/third/zlib/trees.o third/zlib/trees.cpp

${OBJECTDIR}/third/zlib/deflate.o: third/zlib/deflate.cpp 
	${MKDIR} -p ${OBJECTDIR}/third/zlib
	${RM} $@.d
	$(COMPILE.cc) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/third/zlib/deflate.o third/zlib/deflate.cpp

${OBJECTDIR}/src/crest_user_manager.o: src/crest_user_manager.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/crest_user_manager.o src/crest_user_manager.cpp

${OBJECTDIR}/src/crest.o: src/crest.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/crest.o src/crest.cpp

${OBJECTDIR}/src/auth_digest.o: src/auth_digest.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/auth_digest.o src/auth_digest.cpp

${OBJECTDIR}/examples/proc.o: examples/proc.cpp 
	${MKDIR} -p ${OBJECTDIR}/examples
	${RM} $@.d
	$(COMPILE.cc) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/examples/proc.o examples/proc.cpp

${OBJECTDIR}/third/mongoose/mongoose.o: third/mongoose/mongoose.cpp 
	${MKDIR} -p ${OBJECTDIR}/third/mongoose
	${RM} $@.d
	$(COMPILE.cc) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/third/mongoose/mongoose.o third/mongoose/mongoose.cpp

${OBJECTDIR}/src/crest_connection.o: src/crest_connection.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/crest_connection.o src/crest_connection.cpp

${OBJECTDIR}/src/auth_basic.o: src/auth_basic.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/auth_basic.o src/auth_basic.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/proc

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
