function map(key,value) {
  var outlinks = JSON.parse(value);
  for(var i=0; i < outlinks.length; i++) {
    Meguro.emit(outlinks[i],1);
  }
}

function reduce(key,values) {
  Meguro.save(key,values.length);
}
