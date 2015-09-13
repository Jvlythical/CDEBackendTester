#!/usr/bin/env ruby

require 'open3'
require 'net/http'
require 'json'

# USAGE
# Place test_cases in the same directory

# Module tests the CDE's run functionality
# Assumes that upload file works correctly

# Tester must provide compile url and a 
# temp container's name

module RunTester
	
	@run= 'http://localhost:3000/compile/compile_file'
	@poll = 'http://localhost:3000/compile/poll'

	@debug = true
	@expect = nil
	@params = nil
	@const_params = {
		:user_id => '',
		:access_token => '',
		:type => 'temp'
	}

	def self.init(cname)
		@const_params['name'] = cname
	end
	
	# Driver for testing the program
	def self.run_test_stdin(stdin, n)
		
		success = []
		failure = []

		stdin = stdin || []

		for i in 0...n
			
			@params[:stdin] = nil

			# Initial run
			out = self.send_post_request(@run, @params)
			
			# Generate output
			output = out['stdout']
			
			if @debug
				puts 'TEST'
				puts @params
			end

			for input in stdin
				@params[:stdin] = input

				if @debug
					puts 'Inputing: ' + input
				end

				out = self.send_post_request(@run, @params)

				output = out['stdout']
			end
			
			# If the program hasn't finished
			# And all the inputs have been given, then poll
			if out['status'] != 1
				sleep 1
				out = self.send_get_request(@poll, @const_params)
				output = out['stdout']
			end

			if @debug
				puts output
				puts ''
			end

			# If output equals expected
			# then inrement the success count
			# else increment the failure count
			if output.strip != @expect.strip
				failure.push(output)
			else
				success.push(output)
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

		raise 'FAILURE: Missing config.' if config.nil?
		raise 'FAILURE: Invalid main.' if config['main'].nil?
		raise 'FAILURE: Invalid test.' if config['test'].nil?
		raise 'FAILURE: Invalid language.' if config['language'].nil?
		
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
		
		test += ' ' + args if !args.nil?
		test += ' < ' + input if !input.nil?

		# Get control output
		cmd = "sh -c 'cd " + dir_path + "; " + test + "'"
		stdout, stderr, status = Open3.capture3(cmd)
		stdout.encode('UTF-16', :invalid => :replace, :undefined => :replace, replace: "")

		if @debug
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

		@expect = stdout
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

private

	def self.send_post_request(route, params)
		res = Net::HTTP.post_form(URI(route), params)
		
		raise res.code.to_s if res.code != "200"

		return JSON.parse(res.body)
	end

	def self.send_get_request(route, params)

		route += '?'
		params.each do |key, value|
			route += (key.to_s + '=' + value.to_s + '&')
		end
		route = route[0, route.length - 1]

		res = Net::HTTP.get_response(URI(route))
		
		raise res.code.to_s if res.code != "200"

		return JSON.parse(res.body)
	end

end

