function map(key, value) {
  var json = JSON.parse(value);
  var out = {};
  if (json.web_sites) {
    for (var k in json.web_sites) {
      out[k] = json.web_sites[k];
    }
  }
  if (json.nomenclature) {
    for (var k in json.nomenclature) {
      out[k] = json.nomenclature[k];
    }
  }
  Meguro.emit(key, JSON.stringify(out));
}
