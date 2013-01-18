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
CND_DLIB_EXT=so
CND_CONF=file_server
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
	${OBJECTDIR}/src/cr_result.o \
	${OBJECTDIR}/src/cr_string_map.o \
	${OBJECTDIR}/third/zlib/trees.o \
	${OBJECTDIR}/third/zlib/deflate.o \
	${OBJECTDIR}/src/cr_user_manager.o \
	${OBJECTDIR}/src/crest.o \
	${OBJECTDIR}/src/cr_cache.o \
	${OBJECTDIR}/src/cr_utils.o \
	${OBJECTDIR}/src/auth_digest.o \
	${OBJECTDIR}/tests/tests.o \
	${OBJECTDIR}/_ext/1642614647/cr_event_loop.o \
	${OBJECTDIR}/examples/file_server.o \
	${OBJECTDIR}/src/cr_json.o \
	${OBJECTDIR}/src/auth_basic.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-fno-exceptions -fno-rtti -flto -Ofast -fdata-sections -ffunction-sections -std=c++11 -g3
CXXFLAGS=-fno-exceptions -fno-rtti -flto -Ofast -fdata-sections -ffunction-sections -std=c++11 -g3

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-lpthread -ldl

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/server

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/server: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	g++ -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/server ${OBJECTFILES} ${LDLIBSOPTIONS} -fno-exceptions -fno-rtti -static-libstdc++ -Ofast -s

${OBJECTDIR}/src/cr_xml.o: src/cr_xml.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -fno-exceptions -fno-rtti -flto -Ofast -fdata-sections -ffunction-sections -std=c++11 -g3 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cr_xml.o src/cr_xml.cpp

${OBJECTDIR}/src/cr_connection.o: src/cr_connection.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -fno-exceptions -fno-rtti -flto -Ofast -fdata-sections -ffunction-sections -std=c++11 -g3 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cr_connection.o src/cr_connection.cpp

${OBJECTDIR}/src/cr_result.o: src/cr_result.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -fno-exceptions -fno-rtti -flto -Ofast -fdata-sections -ffunction-sections -std=c++11 -g3 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cr_result.o src/cr_result.cpp

${OBJECTDIR}/src/cr_string_map.o: src/cr_string_map.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -fno-exceptions -fno-rtti -flto -Ofast -fdata-sections -ffunction-sections -std=c++11 -g3 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cr_string_map.o src/cr_string_map.cpp

${OBJECTDIR}/third/zlib/trees.o: third/zlib/trees.cpp 
	${MKDIR} -p ${OBJECTDIR}/third/zlib
	${RM} $@.d
	$(COMPILE.cc) -g -fno-exceptions -fno-rtti -flto -Ofast -fdata-sections -ffunction-sections -std=c++11 -g3 -MMD -MP -MF $@.d -o ${OBJECTDIR}/third/zlib/trees.o third/zlib/trees.cpp

${OBJECTDIR}/third/zlib/deflate.o: third/zlib/deflate.cpp 
	${MKDIR} -p ${OBJECTDIR}/third/zlib
	${RM} $@.d
	$(COMPILE.cc) -g -fno-exceptions -fno-rtti -flto -Ofast -fdata-sections -ffunction-sections -std=c++11 -g3 -MMD -MP -MF $@.d -o ${OBJECTDIR}/third/zlib/deflate.o third/zlib/deflate.cpp

${OBJECTDIR}/src/cr_user_manager.o: src/cr_user_manager.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -fno-exceptions -fno-rtti -flto -Ofast -fdata-sections -ffunction-sections -std=c++11 -g3 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cr_user_manager.o src/cr_user_manager.cpp

${OBJECTDIR}/src/crest.o: src/crest.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -fno-exceptions -fno-rtti -flto -Ofast -fdata-sections -ffunction-sections -std=c++11 -g3 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/crest.o src/crest.cpp

${OBJECTDIR}/src/cr_cache.o: src/cr_cache.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -fno-exceptions -fno-rtti -flto -Ofast -fdata-sections -ffunction-sections -std=c++11 -g3 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cr_cache.o src/cr_cache.cpp

${OBJECTDIR}/src/cr_utils.o: src/cr_utils.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -fno-exceptions -fno-rtti -flto -Ofast -fdata-sections -ffunction-sections -std=c++11 -g3 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cr_utils.o src/cr_utils.cpp

${OBJECTDIR}/src/auth_digest.o: src/auth_digest.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -fno-exceptions -fno-rtti -flto -Ofast -fdata-sections -ffunction-sections -std=c++11 -g3 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/auth_digest.o src/auth_digest.cpp

${OBJECTDIR}/tests/tests.o: tests/tests.cpp 
	${MKDIR} -p ${OBJECTDIR}/tests
	${RM} $@.d
	$(COMPILE.cc) -g -fno-exceptions -fno-rtti -flto -Ofast -fdata-sections -ffunction-sections -std=c++11 -g3 -MMD -MP -MF $@.d -o ${OBJECTDIR}/tests/tests.o tests/tests.cpp

${OBJECTDIR}/_ext/1642614647/cr_event_loop.o: ../slite/src/third/libcrest/src/cr_event_loop.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/1642614647
	${RM} $@.d
	$(COMPILE.cc) -g -fno-exceptions -fno-rtti -flto -Ofast -fdata-sections -ffunction-sections -std=c++11 -g3 -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/1642614647/cr_event_loop.o ../slite/src/third/libcrest/src/cr_event_loop.cpp

${OBJECTDIR}/examples/file_server.o: examples/file_server.cpp 
	${MKDIR} -p ${OBJECTDIR}/examples
	${RM} $@.d
	$(COMPILE.cc) -g -fno-exceptions -fno-rtti -flto -Ofast -fdata-sections -ffunction-sections -std=c++11 -g3 -MMD -MP -MF $@.d -o ${OBJECTDIR}/examples/file_server.o examples/file_server.cpp

${OBJECTDIR}/src/cr_json.o: src/cr_json.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -fno-exceptions -fno-rtti -flto -Ofast -fdata-sections -ffunction-sections -std=c++11 -g3 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cr_json.o src/cr_json.cpp

${OBJECTDIR}/src/auth_basic.o: src/auth_basic.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -fno-exceptions -fno-rtti -flto -Ofast -fdata-sections -ffunction-sections -std=c++11 -g3 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/auth_basic.o src/auth_basic.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/server

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
