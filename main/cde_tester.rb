
require 'require_all'
require_all 'run'

module CDETester

	include RunTester

	@debug = true

	def self.configure(options) 
		@debug = options['verbose'] || false
	end

	def self.test_run(cname, run_config, options = [])
		
		self.configure(options)
		
		languages = run_config['languages']
		ignore = run_config['ignore']
		out = ViewRender.new
		test_cases_path = 'run/test_cases'

		# Initialize the tester
		# NOTE: must provide a public CDE name

		begin 
			RunTester.init(cname, options['verbose+'])
		rescue => err
			out.print_exit(err)
		end
		
		suites = Dir[File.join(test_cases_path, '*')]
		
		# If debug flag is set,
		# notify user 
		out.detected_suites(suites) if @debug
		
		# Test each suite located in test_cases_path
		for suite in suites
			
			# Check if the suite should be ignored
			next if ignore.include? File.basename(suite)

			# Get a tests in the suite
			# e.g. test_cases has a python suite, c suite, etc...
			tests = Dir[File.join(suite, '*')]
			
			out.detected_tests(suite) if @debug
			
			# If no tests, continue
			# i.e. no subfolders in the suite
			if tests.length == 0
				out.print_err('FAILURE: No cases found for ' + suite) 
				next
			end

			for test in tests
				
				out.print_test(test) if @debug

				# If client hasn't provided a proper
				# Test case, then continue
				begin
					config, inputs, args = RunTester.trampoline(test)
					
					# Check to see if this test
					# should be run, i.e. client can specify
					# to skip this test in their config.json
					skip = config['skip'] || false
					if skip
						out.print_skip(test)
						next
					end

					description = config['description']
					out.print_description(description) if !description.nil?
				rescue => err
					out.print_err(err)
					next
				end

				len = (inputs.length > args.length ? inputs.length : args.length)

				# Ensure that the loop will iterate once
				# That way inputs/args will be optional
				len = (len == 0 ? 1 : len)
				
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
					input_file = ''

					if !inputs[i].nil?
						input_file = File.join(test, inputs[i]) 
						
						# Read inputs from file
						# and package them for running
						if File.exist? input_file
							fp = File.open(input_file, 'r')
							data = fp.read
							input_ar = data.split("\n")

							fp.close
						end
					end
					
					# Package the post params
					RunTester.params({
						:file_path => File.join(test, config['main']),
						:args => args[i],
						:redirect_file_path => input_file || '',
						:make_target => config['make']
					})
					
					trials = config['trials'] || 1
					res = RunTester.run_test_stdin([], trials)
					out.print_result(test, res)

					# If input_ar, try simulating 
					# user stdin as well
					# Interesting questions is what happens
					# when user provides a input file and stdin? <<< Test this
					if input_ar.length > 0
						RunTester.params({
							:file_path => File.join(test, config['main']),
							:args => args[i],
							:make_target => config['make']
						})
						res = RunTester.run_test_stdin(input_ar, trials)
						out.print_result(test, res)
					end
				end # for i in 0...len

			end # for test in tests
			
			out.print_divider() 

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
			puts '- DETECTED TESTS'
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
			puts '~ RESULTS'
			puts test + ' => ' + res
			puts ''
		end

		def print_divider()
			puts '====='
			puts ''
		end

		def print_exit(err)
			puts 'INVALID CONFIGURATION'
			puts err
			puts ''
		end

		def print_description(description)
			puts 'DESCRIPTION ' +  description
			puts ''
		end

		def print_skip(test)
			puts 'SKIPPING ' + test
			puts ''
		end

	end

end
