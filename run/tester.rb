
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
	@expect = nil
	@params = nil
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
			
			out.print_params(@params) if @debug

			for input in stdin
				@params[:stdin] = input

				out.print_input(input) if @debug

				output = Http.send_post_request(@run, @params)
				output = JSON.parse(output)

				stdout = output['stdout']
			end
			
			# If the program hasn't finished
			# And all the inputs have been given, then poll
			if output['status'] != 1
				sleep 1

				output = Http.send_get_request(@poll, @const_params)
				output = JSON.parse(output)

				stdout = output['stdout']
			end

			print_cde_out(stdout) if @debug
				
			# If output equals expected
			# then increment the success count
			# else increment the failure count
			if stdout.strip != @expect.strip
				failure.push(stdout)
			else
				success.push(stdout)
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

		@expect = stdout
		out.print_control(stdout, stderr) if @debug
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
		
		if File.exist? File.join(dir_path, 'Makefile')
			cmd = 'make'
		else
			cmd = "sh -c 'cd " + dir_path + "; " + compiler + " " + file_name + "'"
		end

		stdout, stderr, status = Open3.capture3(cmd)

	end	

	class ViewRender
		
		def print_control_out(stdout, stderr)
			puts "CONTROL"
			puts '~ command'
			puts cmd 
			puts ''
			
			if stdout.length != 0
				puts '~ stdout '
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
			puts stdout
			puts ''
		end

	end

end

