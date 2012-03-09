#!/usr/bin/ruby -w
# This script just builds number of random pairs from a tokyo cabinet key set
# The first argument is the 
if ARGV.size != 2
  puts "Please specify a tokyo cabinet file and the number of pairs you desire"
  exit
end
puts "Reading Tokyo Cabinet File"

keys = []
`tchmgr list #{ARGV.shift}`.each do |line|
  keys << line.strip
end

number_of_pairs = ARGV.shift.to_i
if number_of_pairs > 0
  number_of_pairs.times do 
    puts keys[rand(keys.size)] + "|" +  keys[rand(keys.size)]
  end
else
  puts "Please specify a number of pairs greater than 0"
end
