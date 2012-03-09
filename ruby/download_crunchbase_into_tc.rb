#!/usr/bin/ruby


require 'rubygems'
require 'tokyocabinet'
require 'typhoeus'
require 'json'

include TokyoCabinet


#####

def make_call(hdb, path)
  res = Typhoeus::Request.get('http://api.crunchbase.com/v/1/'+path+'.js')
  if res.code == 200
    res = res.body
  else
    return nil
  end
  if !hdb.put(path, res)
    ecode = hdb.ecode
    STDERR.printf("put error: %s\n", hdb.errmsg(ecode))
    exit 1
  end
  res
end

#####


raise "Usage: <output tch>" unless ARGV.size==1

# create the object
hdb = HDB::new

# open the database
if !hdb.open(ARGV[0], HDB::OWRITER | HDB::OCREAT)
  ecode = hdb.ecode
  STDERR.printf("open error: %s\n", hdb.errmsg(ecode))
end
print "#{hdb.size()} existing entries\n"

singular_namespaces = ['company', 'person', 'financial-organization', 'service-provider'] #skipping 'product'
plural_namespaces = ['companies', 'people', 'financial-organizations', 'service-providers'] #skipping 'products'

plural_namespaces.each_index do |i|
  namespace = plural_namespaces[i]
  print "Namespace: #{namespace} ("
  res = make_call(hdb, namespace)
  next unless res
  parsed_json = JSON.parse(res)
  nelems = parsed_json.length
  print "#{nelems})\n"
  category = singular_namespaces[i]
  j = 0
  percent5 = (nelems/20+0.5).floor
  parsed_json.each do |elem|
    puts "#{j} done" if j % percent5 == 0
    unless elem.is_a?(Hash) && elem['permalink']
      STDERR.print "Bad from JSON for "+namespace
    else
      make_call(hdb, category+'/'+elem['permalink'])
    end
    j += 1
  end
end

if !hdb.close
  ecode = hdb.ecode
  STDERR.printf("close error: %s\n", hdb.errmsg(ecode))
end

STDERR.print "Done!"


