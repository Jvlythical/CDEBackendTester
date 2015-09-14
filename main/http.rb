
require 'net/http'

module Http

	def self.send_post_request(route, params)
		res = Net::HTTP.post_form(URI(route), params)
		
		raise res.code.to_s if res.code != "200"

		return res.body
	end

	def self.send_get_request(route, params)

		route += '?'
		params.each do |key, value|
			route += (key.to_s + '=' + value.to_s + '&')
		end
		route = route[0, route.length - 1]

		res = Net::HTTP.get_response(URI(route))
		
		raise res.code.to_s if res.code != "200"

		return res.body
	end

end
