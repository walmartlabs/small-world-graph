function map(key, value) {
  var json = JSON.parse(value);
  for (var k2 in json.edges) {
    var ab = key + '|' + k2;
    var descs = json.edges[k2].descriptors;
    for (var i=0, il=descs.length; i < il; ++i) {
      var d = descs[i];
      Meguro.emit(ab+'|'+d.relationship_category+'|'+d.relationship_subcategory, d.snippet);
    }
  }
}
