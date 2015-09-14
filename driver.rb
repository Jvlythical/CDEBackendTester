
require 'require_all'
require_all 'main'

def main
	
	include CDETester
	
	config_file = 'config.json'
	
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
	languages = config['languages']
	options = config['options'] 

	CDETester.test_run(cname, languages, options)

end

main()
