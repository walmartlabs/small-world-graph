#!/usr/bin/ruby


require 'rubygems'
require 'json'



raise "Usage: <input dump>" unless ARGV.size==1


out_hash = {}

File.open(ARGV[0], "r") do |infile|
  while (line = infile.gets)
    p = line.split(/\|/)
    rules_here = JSON.parse(p[1])
    rules_here.each do |k,v|
      out_hash[k] = {}
      out_hash[k]['sel'] = v
    end
  end
end

STDOUT.print out_hash.to_json

