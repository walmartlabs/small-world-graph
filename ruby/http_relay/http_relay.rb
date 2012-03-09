require 'rubygems'
require 'eventmachine'
require 'evma_httpserver'
require 'json'
require 'cgi'
 
class Handler  < EventMachine::Connection
  include EventMachine::HttpServer
 
  def process_http_request
    resp = EventMachine::DelegatedHttpResponse.new(self)
 
    # connection pool on DBSlayer, tell the connection which DB we're accessing.
    query = {"SQL" => "USE dbslayer; SELECT * from widgets"}
 
    http = EM::Protocols::HttpClient2.connect("www.yahoo.com", 80)
    d = http.get "/"
 
    # defer the response until we get response from DBSlayer
    d.callback {
      resp.status = 200
      resp.content = d.content
      resp.send_response
    }
  end
end
 
EventMachine::run {
  EventMachine.epoll
  EventMachine::start_server("0.0.0.0", 8082, Handler)
  puts "Listening..."
}

