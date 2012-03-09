#!/usr/bin/env ruby -w

require 'rubygems'
require 'csv'
require 'json'

rules = {}

class NilClass
  def blank?
    return true
  end
end

class String
  def blank?
    self == ""
  end
end

CSV::open(ARGV.shift,'r') do |row|
  if row[6] and not row[6].blank?
    box_type = row[0].downcase
    rules[box_type] ||= {}
    param_name = row[1]
    param_rule = {}
    param_rule["distance"] = row[2].to_i
    param_rule["reverse_distance"] = row[3].to_i
    param_rule["category"] = row[4]
    param_rule["subcategory"] = row[5] unless row[5].blank?
    param_rule["template"] = row[6]
    rules[box_type][param_name] = param_rule
  end
end

puts rules.to_json
