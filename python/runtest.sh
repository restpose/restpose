#!/bin/sh

nosetests \
 --with-doctest \
 --logging-filter=-restkit \
 --with-coverage \
 --cover-inclusive \
 --cover-html \
 --cover-erase \
 --cover-tests \
 --cover-package=restpose \
 restpose "$@"
