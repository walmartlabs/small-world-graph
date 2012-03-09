function map(key, value) {
  var json = JSON.parse(value);
  var s = JSON.stringify({'can':key, 'd':0});
  if (json.common_name) {
    Meguro.emit(json.common_name, s);
  } else {
    Meguro.emit(key, s);
  }
  var aliases = json.aliases;
  if (aliases) {
    for (var i=0, il=aliases.length; i < il; ++i) {
      var a = aliases[i];
      Meguro.emit(a.name, JSON.stringify({'can':key, 'd':a.distance}));
    }
  }
}

function reduce(key, values) {
  var seen = {};
  var res = [];
  for (var i=0, il=values.length; i < il; ++i) {
    var v = JSON.parse(values[i]);
    if (!seen.hasOwnProperty(v.can)) {
      res.push({'c':v.can, 'd':v.d});
      seen[v.can] = 1;
    }
  }
  res = res.sort(function(a,b){return a.d - b.d});
  var res2 = [];
  for (var i=0, il=res.length; i < il; ++i) {
    res2.push(res[i].c);
    res2.push(res[i].d);
  }
  Meguro.save(key, res2.join('|'));
}

