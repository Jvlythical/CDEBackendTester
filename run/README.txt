
How does the tester work?
	1. The tester will iterate through each directory in this level
		e.g. c, python, c++
		
	2. It will then run all the test cases located in each directory
		e.g. test_1, test_2 ...

		a. The control executable file must be called 'test'
		b. The source file must be 'test' with the proper file extension
				e.g. test.py, test.c
		c. If a makefile is provided in the test, the make file will be run

	3. The tester will compare the control output and the server's output

How to provide a test case?
	1. 

