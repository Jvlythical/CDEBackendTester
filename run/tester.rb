
require 'open3'
require 'json'

require_relative '../main/http.rb'

# USAGE
# Place test_cases in the same directory

# Module tests the CDE's run functionality
# Assumes that upload file works correctly

# Tester must provide compile url and a 
# temp container's name in config.json

module RunTester
	
	include Http
	
	@run = 'http://localhost:3000/compile/compile_file'
	@poll = 'http://localhost:3000/compile/poll'

	@debug = true

	@params = nil
	@expected_stdout = nil
	@expected_stderr = nil
	@const_params = {
		:user_id => '',
		:access_token => '',
		:type => 'temp'
	}

	def self.init(cname, debug = false)
		raise "Invalid container." if cname.nil?
			
		@const_params['name'] = cname
		@debug = debug
	end
	
	# Driver for testing the program
	def self.run_test_stdin(stdin, n)
		
		out = ViewRender.new

		success = []
		failure = []

		stdin = stdin || []

		for i in 0...n
			
			@params[:stdin] = nil

			# Initial run
			output = Http.send_post_request(@run, @params)
			output = JSON.parse(output)
			
			# Generate output
			stdout = output['stdout']
			stderr = output['stderr']
			
			out.print_params(@params) if @debug

			for input in stdin
				@params[:stdin] = input

				out.print_input(input) if @debug

				output = Http.send_post_request(@run, @params)
				output = JSON.parse(output)

				stdout = output['stdout']
				stderr = output['stderr']
			end
			
			# If the program hasn't finished
			# And all the inputs have been given, then poll
			if output['status'] != 1
				sleep 1

				output = Http.send_get_request(@poll, @const_params)
				output = JSON.parse(output)

				stdout = output['stdout']
				stderr = output['stderr']
			end

			out.print_cde_out(stdout) if @debug
			out.print_cde_err(stderr) if @debug
				
			# If output equals expected
			# then increment the success count
			# else increment the failure count
			same_stdout = stdout == @expected_stdout
			same_stderr = stderr == @expected_stderr

			if same_stdout and same_stderr
				success.push(stdout)
			else
				failure.push(stdout)
			end

		end
	
		return success.length.to_s + '/' + n.to_s + ' PASSED'  
	end

#
#   Validates user provided config file
#   
#   Config file requires:
#   	1. main
#   	2. test
#
#   Optional:
#   	1. make 
#   	2. args
#   	3. inputs
#

	def self.trampoline(dir_path) 
		
		# Requirements
		config = self.get_config(dir_path)

		raise 'FAILURE: Missing config for ' + dir_path if config.nil?
		raise 'FAILURE: Invalid main for ' + dir_path if config['main'].nil?
		raise 'FAILURE: Invalid test for ' + dir_path if config['test'].nil?
		raise 'FAILURE: Invalid language for ' + dir_path if config['language'].nil?
		
		inputs = []
		args = []
		
		# Get args
		if !config['args'].nil?
			args = config['args']
		end

		# Get input
		if !config['inputs'].nil?
			inputs = config['inputs']
		end
					
		return config, inputs, args
	end

# Setters
	
	def self.params(params)
		@params = params.merge(@const_params)
	end

	def self.expect(dir_path, test, args, input)
		
		out = ViewRender.new

		test += ' ' + args if !args.nil?
		test += ' < ' + input if !input.nil?

		# Get control output
		cmd = "sh -c 'cd " + dir_path + "; " + test + "'"
		stdout, stderr, status = Open3.capture3(cmd)
		stdout.encode('UTF-16', :invalid => :replace, :undefined => :replace, replace: "")

		@expected_stdout = stdout
		@expected_stderr = stderr

		out.print_control_out(cmd, stdout, stderr) if @debug
	end

	def self.get_config(dir_path)
		file_path = File.join(dir_path, 'config.json')
		config = nil

		begin
			if File.exist? file_path
				fp = File.open(file_path, 'r')

					config = JSON.parse(fp.read)
					fp.close
			end
		rescue => err
			puts 'RunTester: ' + err
			return nil
		end

		return config
	end

	def self.compile(dir_path, compiler, file_name)
		
		out = ViewRender.new
		file_name = File.basename(file_name)

		if File.exist? File.join(dir_path, 'Makefile')
			cmd = 'make'
		else
			cmd = "sh -c 'cd " + dir_path + "; " + compiler + " " + file_name + "'"
		end

		stdout, stderr, status = Open3.capture3(cmd)
		out.print_compile(cmd) if @debug
	end	

	class ViewRender
		
		def print_control_out(cmd, stdout, stderr)
			puts "CONTROL"
			puts '~ command'
			puts cmd 
			puts ''
			
			if stdout.length != 0
				puts '~ local stdout '
				puts stdout
				puts ''
			end

			if stderr.length != 0
				puts '~ stderr '
				puts stderr 
				puts ''
			end
		end

		def print_params(params)
			puts 'TEST'
			puts params
		end

		def print_input(input)
			puts 'Inputting: ' + input
		end

		def print_cde_out(output)
			puts ''
			puts '~ cde stdout'
			puts output
			puts ''
		end

		def print_cde_err(error)
			puts ''
			puts '~ cde stderr'
			puts error
			puts ''
		end

		def print_compile(cmd)
			puts '~ compiling'
			puts cmd
			puts ''
		end

	end

end

