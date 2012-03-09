function map(key,value) {
  var json = JSON.parse(value);
  var index_val = new Object();
  if (json.web_sites) {
    index_val.web_sites = json.web_sites;
  }
    if (json.aliases) {
      var new_aliases = [];
      var old = json.aliases;
      for (var i=0, il=old.length; i < il; ++i) {
        if (old[i].match(/^[A-Z]/)) {
          new_aliases.push(old[i]);
        }
      }
      if (new_aliases.length > 0) {
        index_val.aliases = new_aliases;
      }
    }
    if (json.common_name)
      index_val.common_name = json.common_name;
    Meguro.emit(key,JSON.stringify(index_val));
//}
}
