/* Guha, 4-1-10 */

source_name = 'Wikipedia';
alias_to_canon_g = {};


function map(key, value) {
  if (value == '') { return; }
  var targ = Meguro.dictionary(key);
  if (targ) {
    if (targ != '|') {
      Meguro.emit(targ, value);
    }
  } else {
    Meguro.emit(key, value);
  }
}


// ---------------------------------------------
// -------- R e d u c e r ----------------------
// ---------------------------------------------


/*
 * Make a final graph entry based on the different emitted values
 */
function reduce(key,values) {
  var output = {};

  // now go through again and actually consider "about" information, edges, etc.
  var output_edges = {};
  var aliases = {};
  for (var ii = 0, iil=values.length; ii < iil; ++ii) {
    var vv = JSON.parse(values[ii]);
ITER:
    for (var i=0, il=vv.length; i < il; ++i) {
      var v = JSON.parse(vv[i]);
      if (v.about) {
        v = v.about;
        for (var k in v) {
          output[k] = v[k];
        }
      }
      else if (v.alias) {
        if (!aliases.hasOwnProperty(v.alias)) {
          aliases[v.alias] = v.distance;
        }
        var lower_alias = v.alias.toLowerCase();
        if (!aliases.hasOwnProperty(lower_alias)) {
          aliases[lower_alias] = v.distance;
        }
      } else if (v.relation) {
        v = v.relation;
        var name;
        if (alias_to_canon_g.hasOwnProperty(v.name)) {
          name = alias_to_canon_g[v.name];
        } else {
          name = Meguro.dictionary(v.name);
          if (name) {
            alias_to_canon_g[v.name] = name;
          } else {
            name = v.name;
          }
        }
        var edge = null;
        var desc_ind = null;
        if (output_edges.hasOwnProperty(name)) {
          edge = output_edges[name];
          for (var j=0, jl=edge.descriptors.length; j < jl; ++j) {
            var d = edge.descriptors[j];
            if (d.relationship_category == v.relationship_category && d.relationship_subcategory == v.relationship_subcategory) {
              if (d.relationship_category == 'UNCLASSIFIED') { // in-text
                var chosen_snippet = choose_snippet(d.snippet, v.snippet, name, key);
                edge.descriptors[j].snippet = chosen_snippet;
                if (chosen_snippet != d.snippet) {
                  edge.descriptors[j].source_URL = "http://en.wikipedia.org/wiki/"+encodeURIComponent(name);
                }
                continue ITER;
              }
              if (d.distance <= v.distance) {
                continue ITER; //already have descriptor worth keeping about this
              }
              desc_ind = j;
              break;
            }
          }
          if (desc_ind == null) { // no matching descriptor
            desc_ind = edge.descriptors.length;
            edge.descriptors.push({});
          }
          if (edge.distance > v.distance) {
            edge.distance = v.distance;
          }
        } else { // new edge
          edge = {"descriptors":[{}], "distance":v.distance};
          output_edges[name] = edge;
          desc_ind = 0;
        }
        var desc = edge.descriptors[desc_ind];
        for (var k in v) { 
          if (k != 'name') {
            desc[k] = v[k];
          }
        }
      }
    }
  }

  if (output.common_name) {
    output.edges = output_edges;
    var lower_name = output.common_name.toLowerCase();
    if (lower_name != output.common_name && !aliases.hasOwnProperty(lower_name)) {
      aliases[lower_name] = 0;
    }
    for (var alias0 in aliases) { // 1 iteration just to see if any keys
      output.aliases = [];
      for (var alias in aliases) { //does actual work
        output.aliases.push({'name':alias, 'distance':aliases[alias]});
      }
      break;
    }
    var now = new Date();
    output.last_updates = {};
    output.last_updates[source_name] = now.toGMTString();
    Meguro.save(key, JSON.stringify(output));
  }
}




/* Given multiple snippets, choose the best one according to the shorter one that (ideally) mentions both concepts. */
function choose_snippet(snippet1, snippet2, key1, key2) {
  var key_a = convert_to_query(key1);
  var key_b = convert_to_query(key2);
  var snippets = undefined;
  if (snippet1.length < snippet2.length) {
    snippets = [snippet1, snippet2];
  } else {
    snippets = [snippet2, snippet1];
  }
  /* first see if any of snippets contains both names */
  for (var i = 0, il=snippets.length; i < il; ++i) {
    var s = snippets[i];
    if ((rel_match(s, key1) || s.toLowerCase().indexOf(key_a)) && (rel_match(s, key2) || s.toLowerCase().indexOf(key_b))) {
      return snippets[i];
    }
  }
  return snippets[0];
}


function rel_match(where, what) {
  return (where.indexOf("[["+what+"|")>=0 || where.indexOf("[["+what+"]")>=0);
}


function convert_to_query(word) {
  /* first, see if this might be a name (by seeing if 1-3 words in the "word" that are all capitalized... */
  var parts = word.split(/\s+/);
  var nparts = parts.length;
  if (nparts > 1 && nparts < 5) {
    var noncapital = 0;
    for (var i = 0; i < nparts; ++i) {
      if (!parts[i].match(/^[A-Z]/)) {
        noncapital = 1;
        break;
      }
    }
    // if so, just look at last part
    if (!noncapital && parts[nparts-1].length > 2) {
      if (word.match(/\.$/)) {
        word = parts[nparts-2].replace(/[,:;!]$/,'');
      }
      else if (parts.length > 3) {
        word = parts[0]+' '+parts[1];
      }
      else if (parts[nparts-1] == 'University' || parts[nparts-1] == 'College') {
        parts.pop();
        word = parts.join(' ');
      }
      else {
        word = parts[nparts-1];
      }
    }
  }
  return word.toLowerCase();
}

