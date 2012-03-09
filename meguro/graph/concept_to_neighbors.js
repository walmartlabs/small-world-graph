function map(key, value) {
  var json = JSON.parse(value);
  var r = [];
  if (json.edges) {
    var edges = json.edges;
    var neighs = [];
    for (var n in edges) {
      if (n != key) {
        neighs.push(n);
      }
    }
    neighs = neighs.sort(function(a,b){return edges[a].distance - edges[b].distance});
    for (var i=0, il=neighs.length; i < il; ++i) {
      var n = neighs[i];
      r.push(n);
      r.push(edges[n].distance);
    }
  }
  Meguro.emit(key, r.join('|'));
}
