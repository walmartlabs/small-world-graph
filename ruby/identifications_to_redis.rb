require 'rubygems'
require 'json'
require 'redis'
require 'time'
require 'uri'

if ARGV.size != 3
  puts "Usage: identifications_to_redis.rb ids.tch redis_host redis_port"
  exit
end

ids = ARGV.shift
redis_host = ARGV.shift
redis_port = ARGV.shift.to_i

redis = Redis.new(:host => redis_host, :port => redis_port)

IO.popen("tchmgr list -pv #{ids}").each_line do |line|
  elems = line.strip.split("\t")
  key = elems[0]
  value = elems[1]
  redis.set key,value
end
