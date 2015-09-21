
require_relative 'main/cde_tester.rb'

def main
	
	include CDETester
	
	config_file = 'config.json'
	start = Time.now
	
	exit if !File.exist? config_file
	
	fp = File.open(config_file)

	begin
		config = JSON.parse(fp.read)
	rescue => err
		puts err
		exit
	ensure
		fp.close
	end
	
	cname = config['container']
	run_config = config['run']
	options = config['options'] 

	CDETester.test_run(cname, run_config, options)

	puts 'Finish Time ' + (Time.now - start).to_s

end

main()
