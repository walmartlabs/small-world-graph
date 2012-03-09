function map(key,value) {
  var json = JSON.parse(value);
  if (!(json.last_updates && json.last_updates.CrunchBase) && (!json.notes || !json.notes.entity)) {
    return;
  }
  Meguro.emit(json.common_name,key);
  var sites = json.web_sites;
  if (sites) {
    if (sites.home_page) {
      Meguro.emit(sites.home_page, key);
    }
    if (sites.blog) {
      Meguro.emit(sites.blog, key);
    }
  }
  if (json.aliases) {
    var aliases = json.aliases;
    for(var i=0; i < aliases.length; i++) {
      var alias = aliases[i];
      Meguro.emit(alias,key);
    }
  }
}

function reduce(key,values) {
  Meguro.save(key,JSON.stringify(values));
}
