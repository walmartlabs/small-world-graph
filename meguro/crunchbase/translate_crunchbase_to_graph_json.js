var singular_namespaces = ['company', 'person', 'financial-organization', 'service-provider'] //skipping 'product'
var plural_namespaces = ['companies', 'people', 'financial-organizations', 'service-providers'] //skipping 'products'
var source_name = 'CrunchBase';
var month_names = ['January', 'February', 'March', 'April', 'May', 'June', 'July', 'August', 'September', 'October', 'November', 'December'];

function clean_string(v) {
  if (typeof(v) != 'string') { v = String(v); }
  return v.replace(/^\s+/, '').replace(/\s+$/, '').replace(/<[^>]*>/g,'').replace(/\n/g,' ').replace(/"/g,'&quot;').replace(/\u2019/g,"'").replace(/[\u0080-\uFFFF]+/g,' ').replace(/\s+/g,' ');
}

function date_string(what, obj) {
  var str = '';
  what += '_';
  str += obj[what+'day']+' ';
  var v = String(obj[what+'month']);
  if (v.match(/^\d+$/)) {
    str += month_names[v-1]+' ';
  } else {
    str += v+' ';
  }
  str += obj[what+'year'];
  return str;
}

function do_top_level_field(in_value, out_hash, out_field) {
  if (in_value) {
    if (typeof(in_value) != 'string') {
      in_value = String(in_value);
    }
    if (in_value.match(/\S/)) {
      out_hash[out_field] = clean_string(in_value);
    }
  }
}

function do_relation(target, category, subcategory, snippet, source_url, distance, reverse_distance, source_updated_at) {
  if (!snippet.match(/[\.\?!]$/)) {
    snippet += '.';
  }
  if (!target) { Meguro.log('t: '+category+' '+subcategory+' '+distance); exit; }
  target = clean_string(target);
  if (!snippet) { Meguro.log('s'); exit; }
  snippet = clean_string(snippet);
  if (!source_url) { Meguro.log('u'); exit; }
  source_url = clean_string(source_url);
  if (!source_updated_at) { Meguro.log('a: '+category+' '+subcategory+' '+distance); exit; }
  source_updated_at = clean_string(source_updated_at);
  Meguro.emit(key_g, JSON.stringify({"relation":{"name":target, "relationship_category":category, "relationship_subcategory":subcategory, "snippet":snippet, "source_name":source_name, "source_URL":source_url, "distance":distance, "source_updated_at":source_updated_at}}));
  Meguro.emit(target, JSON.stringify({"relation":{"name":key_g, "relationship_category":category, "relationship_subcategory":subcategory, "snippet":snippet, "source_name":source_name, "source_URL":source_url, "distance":reverse_distance, "source_updated_at":source_updated_at}}));
}

function permalink_to_name(permalink) {
  var p = permalink.split('-');
  if (p.length > 1 && p[p.length-1].match(/^\d+$/)) {
    p[p.length-1] = '('+p[p.length-1]+')';
  }
  var n = '';
  for (var i=0, il=p.length; i < il; ++i) {
    var w = p[i];
    n += w.charAt(0).toUpperCase();
    for (var j=1, jl=w.length; j < jl; ++j) {
      if (w[j-1] == '.') {
        n += w.charAt(j).toUpperCase();
      } else {
        n += w.charAt(j);
      }
    }
    if (i != il-1) {
      n += ' ';
    }
  }
  return n;
}


function reduce(key, values) {
  var output = {};
  var output_edges = {};
  for (var i = 0; i < values.length; ++i) {
    try {
      values[i] = JSON.parse(values[i]); // doing this because isn't working right on argument
    } catch(err) {
      Meguro.log('k: '+key+' i: '+i+' v: '+values[i]);
      exit;
    }
  }
ITER:
  for (var i = 0, il=values.length; i < il; ++i) {
    var v = values[i];
    if (v.about) {
      v = v.about;
      for (var k in v) {
        output[k] = v[k];
      }
    } else if (v.relation) {
      v = v.relation;
      var edge = null;
      var desc_ind = 0;
      if (output_edges[v.name]) {
        edge = output_edges[v.name];
        for (var j=0, jl=edge.descriptors.length; j < jl; ++j) {
          var d = edge.descriptors[j];
          if (d.relationship_category == v.relationship_category && d.relationship_subcategory == v.relationship_subcategory) {
            if (d.distance <= v.distance) {
              continue ITER; //already have descriptor worth keeping about this
            }
            desc_ind = j;
          }
          break;
        }
        if (edge.distance > v.distance) {
          edge.distance = v.distance;
        }
      }
      if (!edge) {
        edge = {"descriptors":[{}], "distance":v.distance};
        output_edges[v.name] = edge;
      }
      for (var k in v) {
        if (k != 'name') {
          edge.descriptors[desc_ind][k] = v[k];
        }
      }
    }
    else {
      Meguro.log("Why unknown in reduce: "+v);
      exit;
    }
  }
  output.edges = output_edges;
  var now = new Date();
  output.last_updates = {};
  output.last_updates[source_name] = now.toGMTString();
  Meguro.save(key, JSON.stringify(output));
}


function map(key,value) {
  var type = null;
  key_g = null;
  for (var i = 0, il=singular_namespaces.length; i < il; ++i) {
    var sn = singular_namespaces[i];
    if (sn+'/' == key.substr(0, sn.length+1)) {
      type = sn;
      key_g = permalink_to_name(key.substr(sn.length+1));
      break;
    }
  }
  if (!type) { return; }

  //var in_data = eval('('+value+')');
  var in_data = JSON.parse(value);

  var output = {};

  if (type == 'person') {
    if (!in_data.first_name || !in_data.last_name) {
      return;
    }
    do_top_level_field(in_data.first_name+' '+in_data.last_name, output, 'common_name');
  } else {
    do_top_level_field(in_data.name, output, 'common_name');
  }
  var common_name_lower = output.common_name.toLowerCase();
  if (common_name_lower != output.common_name) {
    output.aliases = [{'name':common_name_lower,'distance':5}];
  }

  output.nomenclature = {};
  do_top_level_field(key, output.nomenclature, 'crunchbase');

  output.web_sites = {};
  do_top_level_field(in_data.homepage_url, output.web_sites, 'home_page');
  do_top_level_field(in_data.blog_url, output.web_sites, 'blog');
  if (in_data.web_presences) {
    for (var i=0, il=in_data.web_presences.length; i < il; ++i) {
      var wp = in_data.web_presences[i];
      if (wp.title.indexOf('LinkedIn') >= 0) {
        do_top_level_field(wp.external_url, output.web_sites, 'linkedin');
        break;
      }
    }
  }

  if (in_data.twitter_username && in_data.twitter_username.match(/\S/)) {
    do_top_level_field('http://twitter.com/'+in_data.twitter_username, output.web_sites, 'twitter');
    do_top_level_field(in_data.twitter_username, output.nomenclature, 'twitter');
  }

  if (in_data.overview) {
    output.descriptions = [];
    output.descriptions[0] = {};
    do_top_level_field(in_data.overview, output.descriptions[0], 'text');
    do_top_level_field(source_name, output.descriptions[0], 'source_name');
    do_top_level_field(in_data.crunchbase_url, output.descriptions[0], 'source_URL');
    do_top_level_field(in_data.updated_at, output.descriptions[0], 'source_updated_at');
  }

  Meguro.emit(key_g, JSON.stringify({"about":output}));

  switch (type) {
    case 'company':
    case 'financial-organization':
    case 'service-provider':
      if (in_data.founded_year) {
        do_relation(in_data.founded_year, 'BUSINESS', 'FOUNDING_DATE', '[['+in_data.name+']] was founded in [['+in_data.founded_year+']]', in_data.crunchbase_url, 40, 60, in_data.updated_at);
        if (in_data.founded_day && in_data.founded_month) {
          var date_str = date_string('founded', in_data);
          do_relation(date_str, 'BUSINESS', 'FOUNDING_DATE', '[['+in_data.name+']] was founded on [['+date_str+']]', in_data.crunchbase_url, 10, 25, in_data.updated_at);
        }
      }
      if (in_data.offices) {
        output.locations = [];
        for (var i=0, il=in_data.offices.length; i < il; ++i) {
          var office = in_data.offices[i];
          output.locations[i] = {};
          do_top_level_field(office.latitude, output.locations[i], 'latitude');
          do_top_level_field(office.longitude, output.locations[i], 'longitude');
          if (office.city) {
            do_relation(office.city, 'BUSINESS', 'LOCATION', '[['+in_data.name+']] has a location in [['+office.city+']]', in_data.crunchbase_url, 50, 60, in_data.updated_at);
          }
        }
      }
      if (in_data.acquisition && in_data.acquisition.acquiring_company && in_data.acquisition.acquiring_company.name && in_data.acquisition.acquiring_company.permalink) {
        var acq = in_data.acquisition;
        var acq_str = '[['+in_data.name+']] was acquired by '+acq.acquiring_company.name+((acq.price_amount && acq.price_currency_code) ? ' for '+acq.price_amount+' '+acq.price_currency_code : '');
        if (acq.acquired_year) {
          if (acq.acquired_day && acq.acquired_month) {
            var date_str = date_string('acquired', acq);
            do_relation(date_str, 'BUSINESS', 'ACQUISITION', acq_str+' on [['+date_str+']]', in_data.crunchbase_url, 30, 50, in_data.updated_at);
          }
          do_relation(acq.acquired_year, 'BUSINESS', 'ACQUISITION', acq_str+' in [['+acq.acquired_year+']]', in_data.crunchbase_url, 50, 70, in_data.updated_at);
          acq_str += ' in '+acq.acquired_year
        }
        acq_str = acq_str.replace(acq.acquiring_company.name, '[['+acq.acquiring_company.name+']]');
        do_relation(permalink_to_name(acq.acquiring_company.permalink), 'BUSINESS', 'ACQUISITION', acq_str, in_data.crunchbase_url, 10, 10, in_data.updated_at);
      }
      if (in_data.acquisitions && in_data.acquisitions.length) {
        for (var i=0, il=in_data.acquisitions.length; i < il; ++i) {
          var acq = in_data.acquisitions[i];
          if (acq.company && acq.company.name && acq.company.permalink) {
            var acq_str = '[['+in_data.name+']] acquired '+acq.company.name+((acq.price_amount && acq.price_currency_code) ? ' for '+acq.price_amount+' '+acq.price_currency_code :    '');
            if (acq.acquired_year) { 
              if (acq.acquired_day && acq.acquired_month) { 
                var date_str = date_string('acquired', acq);
                do_relation(date_str, 'BUSINESS', 'ACQUISITION', acq_str+' on [['+date_str+']]', in_data.crunchbase_url, 30, 50, in_data.updated_at);
              }
              do_relation(acq.acquired_year, 'BUSINESS', 'ACQUISITION', acq_str+' in [['+acq.acquired_year+']]', in_data.crunchbase_url, 50, 60, in_data.updated_at);
              acq_str += ' in '+acq.acquired_year;
            }
            acq_str = acq_str.replace(acq.company.name, '[['+acq.company.name+']]');
            do_relation(permalink_to_name(acq.company.permalink), 'BUSINESS', 'ACQUISITION', acq_str, in_data.crunchbase_url, 10, 10, in_data.updated_at);
          }
        }
      }
      if (in_data.competitions) {
        for (var i=0, il=in_data.competitions.length; i < il; ++i) {
          var comp = in_data.competitions[i];
          if (comp && comp.competitor && comp.competitor.name && comp.competitor.permalink) {
            do_relation(permalink_to_name(comp.competitor.permalink), 'BUSINESS', 'COMPETITION', '[['+in_data.name+']] and [['+comp.competitor.name+']] are competitors', in_data.crunchbase_url, 40, 40, in_data.updated_at);
          }
        }
      }
      if (in_data.providerships) {
        for (var i=0, il=in_data.providerships.length; i < il; ++i) {
          var prov = in_data.providerships[i];
          if (prov && prov.title && prov.title.length && prov.provider && prov.provider.name && prov.provider.permalink) {
            do_relation(permalink_to_name(prov.provider.permalink), 'BUSINESS', 'PROVIDER', '[['+prov.provider.name+']] '+(prov.is_past ? 'used to provide' : 'provides')+' '+prov.title.toLowerCase()+' services for [['+in_data.name+']]', in_data.crunchbase_url, 40, 30, in_data.updated_at);
          }
        }
      }
      if (in_data.relationships) {
        for (var i=0, il=in_data.relationships.length; i < il; ++i) {
          var rel = in_data.relationships[i];
          if (rel && rel.person && rel.person.permalink && rel.person.permalink.length > 0 && rel.person.first_name && rel.person.last_name) {
            var snippet = '[['+rel.person.first_name+' '+rel.person.last_name+']]';
            if (rel.title && rel.title.length > 0) {
              if (rel.title.match(/^Board/)) {
                rel.title = 'on the '+rel.title;
              }
              snippet += (rel.is_past ? ' was ' : ' is ') + (rel.title.indexOf(',')>=0 || rel.title.match(/^C[A-Z][A-Z]/) || rel.title.match('Chair') || rel.title.match(/^on /) ? '' : (rel.title.match(/^[aeiou]/i) ? 'an ' : 'a ')) + (rel.title.indexOf(',')>0 ? rel.title+',' : rel.title);
            } else {
              snippet += (rel.is_past ? ' used to work' : ' works');
            }
            snippet += ' at [['+in_data.name+']]';
            do_relation(permalink_to_name(rel.person.permalink), 'BUSINESS', 'EMPLOYEE', snippet, in_data.crunchbase_url, ((rel.title.match('ounder') || rel.title.match(/^C[A-Z][A-Z]/)) ? 10 : 50), 10, in_data.updated_at);
          }
        }
      }
      if (in_data.investments) {
        for (var i=0, il=in_data.investments.length; i < il; ++i) {
          var inv = in_data.investments[i];
          if (!inv) { continue; }
          inv = inv.funding_round;
          if (inv && inv.company && inv.company.name && inv.company.name.length && inv.company.permalink) {
            var snippet = '[['+in_data.name + ']] invested in ' + inv.company.name;
            if (inv.funded_year) {
              if (inv.funded_day && inv.funded_month) {
                var date_str = date_string('funded', inv);
                do_relation(date_str, 'BUSINESS', 'INVESTMENT', snippet+' on [['+date_str+']]', in_data.crunchbase_url, 25, 40, in_data.updated_at);
              }
            }
            snippet = '[['+in_data.name+']] invested in [['+inv.company.name+']] in '+inv.funded_year;
            do_relation(permalink_to_name(inv.company.permalink), 'BUSINESS', 'INVESTMENT', snippet, in_data.crunchbase_url, 10, 10, in_data.updated_at);
          }
        }
      }
      if (in_data.ipo && in_data.ipo.pub_year) {
        do_relation(in_data.ipo.pub_year, 'BUSINESS', 'INVESTMENT', '[['+in_data.name+"]]'s I.P.O. was in [["+in_data.ipo.pub_year+']]', in_data.crunchbase_url, 50, 60, in_data.updated_at);
        if (in_data.ipo.pub_day && in_data.ipo.pub_month) {
          var date_str = date_string('pub', in_data.ipo);
          do_relation(date_str, 'BUSINESS', 'INVESTMENT', '[['+in_data.name+"]]'s I.P.O. was on [["+date_str+']]', in_data.crunchbase_url, 40, 50, in_data.updated_at);
        }
      }
      break;
    case 'person':
      if (in_data.born_year && in_data.born_year.length > 0) {
        do_relation(in_data.born_year, 'PERSON', 'BIRTH', '[['+output.common_name+']] was born in [['+in_data.born_year+']]', in_data.crunchbase_url, 45, 70, in_data.updated_at);
        if (in_data.born_day && in_data.born_month) {
          var date_str = date_string('birth', in_data);
          do_relation(date_str, 'PERSON', 'BIRTH', '[['+output.common_name+']] was born on [['+date_str+']]', in_data.crunchbase_url, 10, 50, in_data.updated_at);
        }
      }
      break;
  }
}


