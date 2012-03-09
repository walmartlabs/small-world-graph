#ifndef M2_HPP
#define M2_HPP

#include <string>
#include <vector>
#include <map>
#include <utility>
#include <zmq.hpp>
#include <stdint.h>

namespace m2pp {

typedef std::pair<std::string, std::string> header;
typedef std::map<std::string,std::string> param_map_t;

struct request {
	std::string sender;
	std::string conn_id;
	std::string path;
  param_map_t headers;
  param_map_t params;
	std::string body;
	static request parse(zmq::message_t& msg);
};

class connection {
	public:
		connection(const std::string& sender_id_, const std::string& sub_addr_, const std::string& pub_addr_);
		~connection();
		request recv();
		void reply_http(const request& req, const std::string& response, uint16_t code = 200, const std::string& status = "OK", std::vector<header> hdrs = std::vector<header>());
	private:
		zmq::context_t ctx;
		std::string sender_id;
		std::string sub_addr;
		std::string pub_addr;
		zmq::socket_t reqs;
		zmq::socket_t resp;
};

}


#endif
