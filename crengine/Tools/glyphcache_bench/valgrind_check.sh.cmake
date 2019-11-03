#!/bin/sh

valgrind --tool=memcheck -v --leak-check=full --show-reachable=yes --track-origins=yes \
	${CMAKE_CURRENT_BINARY_DIR}/glyphcache_bench \
	$* 2>&1 | tee ${CMAKE_CURRENT_BINARY_DIR}/valgrind_check.log
