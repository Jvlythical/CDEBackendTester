
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
	
	cname = '51421a9ef5c7ba436d301b4a06d68fd7df843582'
	CDETester.test_run(cname, config)

end

main()
