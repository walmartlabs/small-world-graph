#!/usr/local/bin/ruby


require 'rubygems'
require 'tokyocabinet'

include TokyoCabinet


def read_db(db) 
  vals = {}
  db.iterinit
  while key=db.iternext
    vals[key] = db.get(key)
  end
  vals
end

def open_db(name, mode)
  db = HDB::new
  if !db.open(name, mode)
    ecode = db.ecode
    STDERR.printf("open error: %s\n", db.errmsg(ecode))
  end
  db
end


def close_db(db)
  if !db.close
    ecode = db.ecode
    STDERR.printf("close error: %s\n", db.errmsg(ecode))
  end
end

#####


raise "Usage: <mapping 1> <mapping 2>" unless ARGV.size==2


db1_name = ARGV[0]
db2_name = ARGV[1]

db1 = open_db(db1_name, HDB::OREADER)
STDERR.print "#{db1.size()} entries in #{db1_name}\n"

db2 = open_db(db2_name, HDB::OREADER)
STDERR.print "#{db2.size()} entries in #{db2_name}\n"

db1_vals = read_db(db1)
db2_vals = read_db(db2)

close_db(db1)
close_db(db2)

nidentical = 0

only_1 = []
puts "Changed values (key1, value1, value2):"
db1_vals.keys.sort.each do |k1|
  v2 = db2_vals[k1]
  if v2
    v1 = db1_vals[k1]
    if v2.downcase == v1.downcase
      nidentical += 1
    else
      puts "#{k1}\t#{v1}\t#{v2}"
    end
    db2_vals.delete(k1)
  else
    only_1.push("#{k1}\t#{db1_vals[k1]}")
  end
end

puts "\nKeys only in #{db1_name}:"
only_1.each { |k| puts k }

puts "\nKeys only in #{db2_name}:"
db2_vals.keys.sort.each { |k| puts "#{k}\t#{db2_vals[k]}" }


STDERR.print "Done! #{nidentical} entries identical between the two.\n"


