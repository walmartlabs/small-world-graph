/* Guha, 4-6-10 */


/*
    Fill in distances
 */
function map(key,value) {
  var entry = JSON.parse(value);

  // go through the edges
  var edges = entry.edges;
  for (var name in edges) {
    var e = edges[name];
    var min_dist = 9999;
    var descs = e.descriptors;
    for (var i=0, il=descs.length; i < il; ++i) {
      var desc = descs[i];
      var dist = desc.distance;
      if (dist < 0) {
        dist = Meguro.dictionary(key+'|'+name);
        if (!dist) {
          Meguro.log("Missing distance: "+key+"|"+name);
          dist = 255;
        } else {
          dist = parseInt(dist);
        }
        desc.distance = dist;
      }
      if (dist < min_dist) {
        min_dist = dist;
      }
    }
    e.distance = min_dist;
  }

  Meguro.emit(key, JSON.stringify(entry));
}

