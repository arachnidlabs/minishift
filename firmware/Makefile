.PHONY: phony

all: slave test data

slave: phony
	cd slave; make all

test: phony
	cd test; make all

data: slave test
	cp slave/main_data.c jig/main_data.h
	cp test/test_data.c jig/test_data.h
