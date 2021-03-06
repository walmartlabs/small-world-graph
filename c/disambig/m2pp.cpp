#include <string>
#include <sstream>
#include <zmq.hpp>
#include "m2pp.hpp"
#include "m2pp_internal.hpp"
#include <iostream>

using namespace std;

namespace m2pp {

connection::connection(const std::string& sender_id_, const std::string& sub_addr_, const std::string& pub_addr_) 
	: ctx(1), sender_id(sender_id_), sub_addr(sub_addr_), pub_addr(pub_addr_), reqs(ctx, ZMQ_UPSTREAM), resp(ctx, ZMQ_PUB) {
	reqs.connect(sub_addr.c_str());
	resp.connect(pub_addr.c_str());
	resp.setsockopt(ZMQ_IDENTITY, sender_id.data(), sender_id.length());
}

connection::~connection() {
}

request connection::recv() {
	zmq::message_t inmsg;
	reqs.recv(&inmsg);
	return request::parse(inmsg);
}

void connection::reply_http(const request& req, const std::string& response, uint16_t code, const std::string& status, std::vector<header> hdrs) {
	std::ostringstream httpresp;

	httpresp << "HTTP/1.1" << " " << code << " " << status << "\r\n";
	httpresp << "Content-Length: " << response.length() << "\r\n";
	for (std::vector<header>::iterator it=hdrs.begin();it!=hdrs.end();it++) {
		httpresp << it->first << ": " << it->second << "\r\n";
	}
	httpresp << "\r\n" << response;

	// Using the new mongrel2 format as of v1.3 
	std::ostringstream msg;
	msg << req.sender << " " << req.conn_id.size() << ":" << req.conn_id << ", " << httpresp.str();
	std::string msg_str = msg.str();
	zmq::message_t outmsg(msg_str.length());
	::memcpy(outmsg.data(), msg_str.data(), msg_str.length());
	resp.send(outmsg);
}

request request::parse(zmq::message_t& msg) {
	request req;
	std::string result(static_cast<const char *>(msg.data()), msg.size());

	std::vector<std::string> results = utils::split(result, " ", 3);

	req.sender = results[0];
	req.conn_id = results[1];
	req.path = results[2];

	std::string body;
	std::string ign;

	utils::parse_json(utils::parse_netstring(results[3], body), req.headers);

	req.body = utils::parse_netstring(body, ign);

  param_map_t::const_iterator query = req.headers.find("QUERY");
  if (query != req.headers.end()) {
    utils::parse_query(query->second,req.params);
  }
  utils::parse_query(req.body,req.params);

	return req;
}

}
