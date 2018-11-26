#!/bin/sh

valgrind --tool=memcheck -v --leak-check=full --show-reachable=yes --track-origins=yes \
	${CMAKE_CURRENT_BINARY_DIR}/cr3 \
	$* 2>&1 | tee ${CMAKE_CURRENT_BINARY_DIR}/valgrind_check.log
