#!/usr/bin/ruby
# == Synopsis
#
# wiki_to_xml: gets a wikipedia database and grabs interesting images and puts them in an xml file
#
# == Usage
#
# wiki_to_xml [OPTION] ... DIR
#
# -h, --help
#  show help
#
# --out x, -o x
#  output file
#
# --skip x, -s x
#  skip to the id
#
# --topic x , -t x
#  root topic x
#
# --verbose x , -v x
#  Print debugging messages

require 'rubygems'
require 'mysql'
require 'builder'
require 'digest/md5'
require 'getoptlong'
require 'rdoc/usage'

require 'image'
require 'wiki_crawler'

opts = GetoptLong.new(
  ['--help','-h', GetoptLong::NO_ARGUMENT],
  ['--out','-o', GetoptLong::REQUIRED_ARGUMENT],
  ['--skip','-s', GetoptLong::REQUIRED_ARGUMENT],
  ['--topic','-t', GetoptLong::REQUIRED_ARGUMENT],
  ['--verbose','-v', GetoptLong::NO_ARGUMENT]
)

outfile = '/tmp/wiki_images.xml'
root_topic = 'Public_domain'
skip_id = nil
verbose = false
opts.each do |opt,arg|
  case opt
  when '--help'
    RDoc::usage
  when '--out'
    outfile = arg
  when '--topic'
    root_topic = arg
  when '--skip'
    skip_id = arg.to_i
  when '--verbose'
    verbose = true
  end
end

m = Mysql.new('192.168.0.15','wiki','wikio','wikimedia')
f = File.open(outfile,'w')
builder = Builder::XmlMarkup.new(:target => f,:indent => 2)
builder.instruct!
Image.initialize(m,builder)

wc = WikiCrawler.new(m,builder,verbose)
builder.images do 
  begin
    ids = wc.drill_category(root_topic)
    original_size = ids.size
    counter = 0
    if skip_id
      puts "Skipping to #{skip_id}"
      ids = ids[skip_id,ids.size]
      counter = skip_id
    end
    ids.each do |id|
      puts "#{counter} / #{original_size} complete"
      wc.drill_page(id)
      counter += 1
    end
  rescue Interrupt
    puts "Rescued Interrupt"
  end
end
