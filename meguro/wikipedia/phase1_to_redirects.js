function map(key, value) {
  var json = JSON.parse(value);

  for (var i=0, il=json.length; i < il; ++i) {
    var v = JSON.parse(json[i]);
    if (v.alias && v.fr == 'r') {
      Meguro.emit(v.alias, key);
    }
  }
}
