function map(key, value) {
  var json = JSON.parse(value);
  for (var i=0, il=json.length; i < il; ++i) {
    var el = json[i];
    if (typeof(el) != 'string' && el.type == 'category' && el.category) {
      var cat = el.category;
      if (cat) {
        cat = cat.toLowerCase();
        if (cat.indexOf(' agencies') > 0 || cat.indexOf('living people') >= 0 || cat.indexOf('companies') >= 0 || cat.indexOf('organization') >= 0 || cat.indexOf('government') >= 0 || cat.indexOf('establishment') >= 0 || cat.indexOf('ministries') >= 0 || cat.indexOf(' established') > 0) {
          Meguro.emit(key, 1);
          break;
        }
      }
    }
  }
}

      
