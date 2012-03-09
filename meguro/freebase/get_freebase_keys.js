function map(key, value) {
  var parts = value.split(/\t+/);
  var id = parts[0];
  var relation = parts[1];
  var ty = null;
  if (relation == '/type/object/key') {
    ty = 'key';
  } else if (relation == '/type/object/name') {
    ty = 'name';
  }
  if (ty) {
    Meguro.emit(id, JSON.stringify([ty, parts[2], parts[3]]));
  }
}

function reduce(key, values) {
  var name = null;
  var nkey = null;
  for (var i=0, il=values.length; i < il; ++i) {
    var v = JSON.parse(values[i]);
    if (v[0] == 'name') {
      if (!name || v[1] == '/lang/en') {
        name = v[2].replace(/\s+\(.*\)/,'').replace(/ \s+/,' ');
      }
    } else if (v[0] == 'key') {
      v[1] = v[1].substr(v[1].indexOf('/', 2)+1).replace(/\//g, ' ');
      if (v[2] == 'best') {
        nkey = v[1];
      } else if (!nkey) {
        nkey = v[1] + ' ' + v[2];
      }
    }
  }
  if (name) {
    if (nkey) {
      var lastspace = nkey.lastIndexOf(' ');
      if (lastspace > 0) {
        if (nkey.substr(lastspace+1) == name) {
          nkey = nkey.substr(0, lastspace);
        }
      }
    } else {
      nkey = name;
    }
    Meguro.save(key, JSON.stringify({key:name+' ('+nkey+')', name:name}));
  } else if (nkey) {
    Meguro.save(key, JSON.stringify({key:nkey}));
  } else {
    Meguro.log("No name or key for "+key);
  }
}
