CFLAGS=-g -std=c99 -O0

TEST_EXTS = .fun .ok
UTCS_ID ?= $(shell pwd | sed -e 's/.*_//')

MY_TESTS = ${addprefix ${UTCS_ID},${TEST_EXTS}}
SIG = ${firstword ${shell (ls ${MY_TESTS} > /dev/null 2>&1  && (cat ${MY_TESTS} | md5sum)) || echo "?" }}

POSSIBLE_TESTS = ${basename ${wildcard *${firstword ${TEST_EXTS}}}}
TESTS = ${sort ${POSSIBLE_TESTS}}
TEST_EXECS = ${addsuffix .exec, $(TESTS)}
TEST_OKS = ${addsuffix .ok,${TESTS}}
TEST_RESULTS = ${addsuffix .result,${TESTS}}
TEST_TARGETS = ${addsuffix .test,${TESTS}}
TEST_OUTS = ${addsuffix .out,${TESTS}}
TEST_RAWS = ${addsuffix .raw,${TESTS}}
TEST_DIFFS = ${addsuffix .diff,${TESTS}}

p4 : p4.o Makefile
	gcc $(CFLAGS) -o p4 p4.o -lm

%.o : %.c Makefile
	gcc $(CFLAGS) -MD -c $*.c

#%.o : %.S Makefile
#	gcc $(CFLAGS) -MD -c $*.S

$(TEST_EXECS) : %.exec : Makefile p4 %.fun
	-timeout 5 ./p4 $*

$(TEST_RAWS) : %.raw : Makefile p4 %.exec
	-((timeout 5 ./$*.exec `cat $*.args` || echo timeout) | head -n 10000) > $*.raw

$(TEST_OUTS) : %.out : Makefile %.raw
	-cp $*.raw $*.out

${TEST_DIFFS} : %.diff : Makefile %.out %.ok
	-(diff -wBb $*.out $*.ok || true) > $*.diff 2> /dev/null

${TEST_RESULTS} : %.result : Makefile %.diff
	(test -z "`cat $*.diff`" && echo "pass" > $*.result) || echo "fail" > $*.result

${TEST_TARGETS} : %.test : Makefile %.result
	@echo "$* ... `cat $*.result`"

test: Makefile ${TEST_RESULTS} ${TEST_TARGETS} ;

clean :
	rm -f *.raw *.out *.d *.o *.s p4 *.diff *.result $(TEST_EXECS)

-include *.d


#####################################################################


sig:
	@echo ${SIG}

tests :
	@echo "$(TESTS)"

copy_student_tests:
ifeq ($(SIG),?)
	@echo "no tests"
else
ifeq ($(TEST_DIR),)
	@echo "no TEST_DIR"
else
	@for i in $(TEST_EXTS); do \
	    cp $(UTCS_ID)$$i $(TEST_DIR)/$(SIG)$$i; \
	done
endif
endif

copy_given_tests:
ifeq ($(TEST_DIR),)
	@echo "no TEST_DIR"
else
	@for i in $(TEST_EXTS); do \
	    cp t?$$i $(TEST_DIR); \
	done
endif

remove_tests:
	@for i in $(TEST_EXTS); do \
	    rm -rf *$$i; \
	done

copy_results:
ifeq ($(RESULTS_DIR),)
	@echo "no RESULTS_DIR"
else
	-cp *.result $(RESULTS_DIR)
	-cp *.ok $(RESULTS_DIR)
	-cp *.out $(RESULTS_DIR)
	-cp *.raw $(RESULTS_DIR)
endif
