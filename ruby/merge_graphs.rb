#!/usr/local/bin/ruby


require 'rubygems'
require 'tokyocabinet'
require 'json'

include TokyoCabinet


# this makes a hash of names to keys that might represent those names
def make_name_to_keys(db)
  map = {}
  db.iterinit
  while key=db.iternext
    variations = potential_keys(db, key)
    variations.each { |n| (map[n] ||= []) << key }
  end
  map
end


# given a key and a DB, return an array of possible variant names of that key
def potential_keys(db, key)
  name = key.downcase() #all comparisons will be done case insensitively, relying on the edge comparison to say matches
  paren_ind = key.index('(')
  name = key[0, paren_ind-1] unless paren_ind.nil?
  arr = [name]

  raw_json = db.get(key)
  data = JSON.parse(raw_json)
  unless data.is_a?(Hash)
    STDERR.printf("bad json for %s: %s (%s)\n", key, raw_json, data.class)
    exit 1
  end
  arr << data['common_name'] if data['common_name'] && !arr.include?(data['common_name']) 
  data['aliases'].each { |nh| n=nh['name']; arr << n unless arr.include?(n) || n.match(/Wikipedia:/) } if data['aliases']

  arr
end


# return how good a match the 2 entries look like, based on commonality of tight edges
def match_score(db1, key1, db2, key2)
  data1 = JSON.parse(db1.get(key1))
  data2 = JSON.parse(db2.get(key2))
  score = 0
  if data1['edges']
    data1['edges'].each_pair do |k1, h1|
      if data2['edges']
        data2['edges'].each_pair do |k2, h2|
          if (!h1['distance'] || !h2['distance'] || h1['distance'] < 45 || h2['distance'] < 45) && k1.sub(/\s\(.+\)/,'').downcase == k2.sub(/\s\(.+\)/,'').downcase
            score += 1
          end
        end
      end
    end
  end
  score
end


# return a merged JSON of the 2 entries. In general, when have to make an either/or choice, takes the add_data's value for things that seem like they may get updated, and take the base_db's value for things that seem more fixed and benefiting from big data
def merge_entries(base_db, base_key, add_db, add_key, add_keys_translation, out_db)
  out_data = JSON.parse(base_db.get(base_key))
  add_data = JSON.parse(add_db.get(add_key))

  out_data['common_name'] = add_data['common_name'] if !out_data['common_name'] && add_data['common_name']
  
  if add_data['descriptions']
    exist = {}
    if out_data['descriptions']
      out_data['descriptions'].each_with_index do |d,i|
        exist[d['source_name']] = i if d['source_name']
      end
      add_data['descriptions'].each do |d|
        if exist[d['source_name']]
          out_data['descriptions'][exist[d['source_name']]] = d
        else
          out_data['descriptions'].push(d)
        end
      end
    else
      out_data['descriptions'] = add_data['descriptions']
    end
  end

  out_data['prominence'] = add_data['prominence'] unless out_data['prominence'] || !add_data['prominence']

  aliases_in_out_now = {}
  if add_data['aliases']
    if out_data['aliases']
      out_data['aliases'].each { |a| aliases_in_out_now[a['name']] = 1 }
      add_data['aliases'].each { |a| out_data['aliases'].push(a) unless aliases_in_out_now[a['name']] }
    else
      out_data['aliases'] = add_data['aliases']
    end
  end
  if add_data['common_name'] && base_key != add_data['common_name']
    if out_data['aliases'] 
      out_data['aliases'].push({'name' => add_data['common_name'], 'distance' => 1}) unless aliases_in_out_now[add_data['common_name']]
    else 
      out_data['aliases'] = add_data['aliases']
    end
  end

  if add_data['web_sites']
    if out_data['web_sites']
      add_data['web_sites'].each_pair { |k,v| out_data['web_sites'][k] = v }
    else
      out_data['web_sites'] = add_data['web_sites']
    end
  end

  if add_data['nomenclature']
    if out_data['nomenclature']
      add_data['nomenclature'].each_pair { |k,v| out_data['nomenclature'][k] = v }
    else
      out_data['nomenclature'] = add_data['nomenclature']
    end
  end

  if add_data['locations']
    exist = {}
    if out_data['locations']
      out_data['locations'].each { |l| exist["#{l['latitude']}/#{l['longitude']}"] = true }
      add_data['locations'].each do |l|
        out_data['locations'].push(l) unless exist["#{l['latitude']}/#{l['longitude']}"]
      end
    else
      out_data['locations'] = add_data['locations']
    end
  end

  if add_data['last_updates']
    if out_data['last_updates']
      add_data['last_updates'].each_pair { |k,v| out_data['last_updates'][k] = v }
    else
      out_data['last_updates'] = add_data['last_updates']
    end
  end

  if add_data['edges']
    if out_data['edges']
      add_data['edges'].each_pair do |k_add_orig,v_add|
        k_add = (add_keys_translation[k_add_orig] ? add_keys_translation[k_add_orig] : k_add_orig)
        v_out = out_data['edges'][k_add]
        if v_out #have this edge in base
          v_out['distance'] = v_add['distance'] if v_add['distance'] && (!v_out['distance'] || v_add['distance'] < v_out['distance'])
          exist = {}
          v_out['descriptors'].each { |d| exist[d['source_URL']] = 1 }
          v_add['descriptors'].each do |d|
            v_out['descriptors'].push(d) unless exist[d['source_URL']]
          end
        else
          out_data['edges'][k_add] = v_add
        end
        out_data['edges'][k_add]['updated_at'] = Time.now.to_s
      end
    else
      translate_edge_names(add_data, add_keys_translation)
      add_data['edges'].each_pair { |k,v| out_data['edges'][k] = v }
    end
  end

  output_entry(out_db, base_key, out_data.to_json)
end


def translate_edge_names(entry, key_translation)
  if entry['edges']
    to_delete = []
    entry['edges'].each_key { |k| to_delete.push(k) if key_translation[k] }
    to_delete.each do |k|
      entry['edges'][key_translation[k]] = entry['edges'][k]
      entry['edges'].delete(k)
    end
  end
end


def output_entry(out_db, key, val)
  if !out_db.put(key, val)
    ecode = out_db.ecode
    STDERR.printf("put error: %s\n", out_db.errmsg(ecode))
    exit 1
  end
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


raise "Usage: <base graph Tokyo Cabinet> <graph to merge in Tokyo Cabinet> <output Tokyo Cabinet>" unless ARGV.size==3


base_db_name = ARGV[0]
add_db_name = ARGV[1]
out_db_name = ARGV[2]

raise "#{out_db_name} exists already" if (File::exists?(out_db_name)) 

FROM_BASE = 1
BASE_MAPPED = 2
FROM_ADD = 3

base_db = open_db(base_db_name, HDB::OREADER)
STDERR.print "#{base_db.size()} entries in #{base_db_name}\n"

add_db = open_db(add_db_name, HDB::OREADER)
STDERR.print "#{add_db.size()} entries in #{add_db_name}\n"

# first build a hash of possible alternative keys (meaning non-parenthetical parts, in lowercase) to keys for that
STDERR.print "\nbuild up alternative keys\n"
key_variations_add = make_name_to_keys(add_db)

# go through the base DB and make a hash of things that maybe should be merged
STDERR.print "\nscore possible matches\n"
possible_matches = []
base_db.iterinit
base_keys = {}
ndid = 0
ntotal = base_db.size()
nout = (ntotal/10).floor()
while key=base_db.iternext
  base_keys[key] = FROM_BASE
  variations = potential_keys(base_db, key)
  variations.each do |v|
    if key_variations_add[v] #this guy might be in the add graph
      key_variations_add[v].each do |possible_match|
        score = match_score(base_db, key, add_db, possible_match)
        possible_matches.push([key, possible_match, score]) if score>0
      end
    end
  end
  ndid += 1
  STDERR.printf("%d / %d\n", ndid, ntotal) if ndid % nout == 0
end
key_variations_add = nil #done with this

# go through the possible matches, and sorted by score, and build up a mapping of add_db keys to base_db
STDERR.print "\nmap in order\n"
add_keys_translation = {}
possible_matches.sort! { |a,b| b[2] <=> a[2] }
ndid = 0
ntotal = possible_matches.length
nout = (ntotal/10).floor()
possible_matches.each do |elem|
  if base_keys[elem[0]] != BASE_MAPPED && !add_keys_translation[elem[1]]
    STDERR.printf "%s -> %s,  %d\n", elem[1], elem[0], elem[2]
    add_keys_translation[elem[1]] = elem[0]
    base_keys[elem[0]] = BASE_MAPPED
  end
  ndid += 1
  STDERR.printf("%d / %d\n", ndid, ntotal) if ndid % nout == 0
end

# go through the add_db keys and check if there are any key names which aren't already in mapping but conflict with a key name in base. If so, rename.
STDERR.print "\nrename conflicting names\n"
out_keys = base_keys
ndid = 0
ntotal = add_db.size()
nout = (ntotal/10).floor()
add_db.iterinit
while key=add_db.iternext
  if !add_keys_translation[key] && out_keys[key] #this guy needs a renaming since it's not mapped but it's name is taken
    tag = 1
    base = key
    if key.match(/\(\d+\)/)
      p = key.split(/[\(\)]/)
      base = p[0].rstrip
      tag = p[1] + 1
    end
    new_name = nil
    while true
      new_name = base+' ('+tag.to_s+')'
      break unless out_keys[new_name]
      tag += 1
    end
    out_keys[new_name] = FROM_ADD
    add_keys_translation[key] = new_name
    STDERR.printf "%s -> %s\n", key, new_name
  end
  ndid += 1
  STDERR.printf("%d / %d\n", ndid, ntotal) if ndid % nout == 0
end

# go through the add_db. If renamed and is BASE_MAPPED, then merge and output. If is renamed and FROM_ADD, just output with new key name. If not renamed, just output.
STDERR.print "\noutput for add db\n"
out_db = open_db(out_db_name, HDB::OWRITER | HDB::OCREAT)
ndid = 0
ntotal = add_db.size()
nout = (ntotal/10).floor()
add_db.iterinit
while key=add_db.iternext
  new_name = add_keys_translation[key]
  if new_name
    if out_keys[new_name] == BASE_MAPPED
      merge_entries(base_db, new_name, add_db, key, add_keys_translation, out_db)
      #STDERR.printf "merge: %s + %s\n", new_name, key
    elsif out_keys[new_name] == FROM_ADD
      entry = JSON.parse(add_db.get(key))
      translate_edge_names(entry, add_keys_translation)
      output_entry(out_db, new_name, entry.to_json)
      #STDERR.printf "rename: %s -> %s\n", key, new_name
    else
      STDERR.printf "WHY? %s %d\n", new_name, out_keys[new_name]
    end
  else
    entry = JSON.parse(add_db.get(key))
    translate_edge_names(entry, add_keys_translation)
    output_entry(out_db, key, entry.to_json)
    #STDERR.printf "just put: %s\n", key
  end
  ndid += 1
  STDERR.printf("%d / %d\n", ndid, ntotal) if ndid % nout == 0
end

# iterate through base_db and output anything that already hasn't been (through merging)
STDERR.print "\nleftovers\n"
ndid = 0
ntotal = base_db.size()
nout = (ntotal/10).floor()
base_db.iterinit
while key=base_db.iternext
  output_entry(out_db, key, base_db.get(key)) if out_keys[key] == FROM_BASE # means not merged
  ndid += 1
  STDERR.printf("%d / %d\n", ndid, ntotal) if ndid % nout == 0
end

close_db(base_db)
close_db(add_db)
close_db(out_db)

STDERR.print "Done!\n"


