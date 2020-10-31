CC=gcc

default:
	$(CC) -g -Wall -Wextra lab0.c -o lab0

clean:
	rm -rf *.tar.gz *.o lab0 *.txt

dist:
	tar -czvf lab0.tar.gz lab0.c Makefile README backtrace.PNG breakpoint.PNG

check: default correctoutput opentest createtest invalidarg invalidopt catchsig segfaulttest successexit

correctoutput:
	echo "test output" > input.txt; \
	./lab0 --input=input.txt --output=output.txt &> /dev/null; \
	cmp input.txt output.txt; \
	if [[ $$? -eq 0 ]]; \
	then \
		echo "Test Passed: output.txt matches input.txt"; \
	else \
		echo "Test Failed: output.txt does not match input.txt"; \
	fi; \
	rm -f input.txt output.txt
	
opentest:
	./lab0 --input=dasdjaa.txt; \
	if [[ $$? -eq 2 ]]; \
	then \
		echo "TEST Passed: if input file cannot be found, program exits code 2."; \
	else \
		echo "TEST Failed: input file cannot be found, but exit code is not 2 as specified."; \
	fi; \
	
createtest:
	echo "test creation" > input.txt; \
	touch output.txt; \
	chmod -w output.txt; \
	./lab0 --input=input.txt --output=output.txt; \
	if [[ $$? -eq 3 ]]; \
	then \
		echo "Test Passed: create error; program exits with correct code"; \
	else \
		echo "Test failed: create error; program exists with incorrect code"; \
	fi; \
	rm -f input.txt output.txt
	
invalidarg:
	./lab0 --wrongarg; \
	if [[ $$? -eq 1 ]]; \
	then \
		echo "Test Passed: invalid option exits with code 0"; \
	else \
		echo "Test Failed: wrong exit code"; \
	fi; \
	
invalidopt:
	./lab0 --input ; \
	if [[ $$? -eq 1 ]]; \
	then \
		echo "Test Passed: invalid argument to option exit 0"; \
	else \
		echo "Test failed: invalid exit code"; \
	fi; \
	
catchsig:
	./lab0 --segfault --catch; \
	if [[ $$? -eq 4 ]]; \
	then \
		echo "Test Passed: signal catched seg fault exit code 4."; \
	else \
		echo "Test Failed: catch segfault leads invalid exit code"; \
	fi; \
	
segfaulttest:
	./lab0 --segfault; \
	if [[ $$? -ne 139 ]]; \
	then \
		echo " Test failed: segfault causes failed, couldnt received error code 139."; \
	else \
		echo "Test Passed: segfault causes successfully, recevied exit code 139."; \
	fi \
	
successexit:
	echo "test in and out" > input.txt \
	./lab0 --input=input.txt --output=output.txt; \
	if [[ $$? -eq 0 ]]; \
	then \
		echo "Test Passed: program exit with code 0."; \
	else \
		echo "Test failed: program didnt exit with code 0."; \
	fi; \
	rm -f input.txt output.txt
		