
require 'require_all'
require_all 'run'

module CDETester

	include RunTester

	@debug = true

	def self.test_run(cname, languages)
		
		out = ViewRender.new
		test_cases_path = 'run/test_cases'

		# Initialize the tester
		# NOTE: must provide a public CDE name
		RunTester.init(cname)
		
		suites = Dir[File.join(test_cases_path, '*')]
		
		# If debug flag is set,
		# notify user 
		out.detected_suites(suites) if @debug
		
		# Test each suite located in test_cases_path
		for suite in suites

			# Get a tests in the suite
			# e.g. test_cases has a python suite, c suite, etc...
			tests = Dir[File.join(suite, '*')]
			
			out.detected_tests(suite) if @debug
			
			# If no tests, continue
			# i.e. no subfolders in the suite
			if tests.length == 0
				out.continue() 
				next
			end

			for test in tests
				
				out.print_test(test) if @debug

				# If client hasn't provided a proper
				# Test case, then continue
				begin
					config, inputs, args = RunTester.trampoline(test)
				rescue => err
					out.print_err(err)
					next
				end

				len = (inputs.length > args.length ? inputs.length : args.length)
				
				# Iterate through different input/arg 
				# cases that the client has provided
				# e.g. args = [0, 1, 2] will translate
				# to three seperate tests
				for i in 0...len
					
					# Generate the expected output
					# based off the client's generated output
					begin
						lang = languages[config['language']]
						compile = lang['compile'] 
						execute = lang['execute'] + config['test']
						
						# Compile the file if it needs compiling
						# test => relative path to test dir
						# compile => compiler
						# config['main'] => src file name
						if !compile.nil?
							RunTester.compile(test, compile, config['main'])
						end
						
						# Execute the file and expect its output
						# test => see above
						# execute => prefix needed to execute e.g. java
						# args[i] => args needed to run the program
						# inputs[i] => input neededto run the program
						RunTester.expect(test, execute, args[i], inputs[i])
					rescue => err
						out.print_err(err) 
						next
					end
									
					# Tokenize the inputs to 
					# simulate how a user would 
					# enter inputs
					input_ar = []
					if !inputs[i].nil?
						input = File.join(test, inputs[i]) 
						
						# Read inputs from file
						# and package them for running
						if File.exist? input
							fp = File.open(input, 'r')
							data = fp.read
							input_ar = data.split("\n")

							fp.close
						end
					end
					
					# Package the post params
					RunTester.params({
						:file_path => File.join(test, config['main']),
						:args => args[i],
						:redirect_file_path => input || '',
						:make_target => config['make']
					})

					res = RunTester.run_test_stdin(input_ar, 1)
					
					out.print_result(test, res) if @debug
				end # for i in 0...len

			end # for test in tests
			
			out.print_divider() if @debug

		end # for suite in suites
	end

	class ViewRender
		def detected_suites(suites)
			puts 'DETECTED SUITES'
			puts suites
			puts ''
			puts '====='
			puts ''
		end

		def detected_tests(tests)
			puts 'DETECTED TESTS'
			puts tests
			puts ''
		end

		def continue
			puts 'Continuing...' 
			puts ''
		end

		def print_test(test)
			puts 'TESTING ' + test
			puts ''
		end

		def print_err(err)
			puts err
			puts 'Continuing...'
			puts ''
		end

		def print_result(test, res)
			puts 'RESULTS'
			puts test + ' => ' + res
			puts ''
		end

		def print_divider()
			puts '====='
			puts ''
		end

	end

end
