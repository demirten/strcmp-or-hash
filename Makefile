LIBMAKEFILE_PATH      = ${CURDIR}/3rdparty/libmakefile

target-y = strcmp-or-hash

strcmp-or-hash_files-y = main.c

include ${LIBMAKEFILE_PATH}/Makefile.lib

