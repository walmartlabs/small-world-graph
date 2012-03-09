function map(key,value) {
  json = JSON.parse(value);
  categories = []
  if (json) {
    for(var i=0; i < json.length; i++) {
      var sec = json[i];
      switch (sec.type) {
        case 'category':
          if (sec.category) {
            categories.push(sec.category.replace("|","").replace(/^\s+|\s+$/g,""));
          }
          break;
        default:
          break;
      }
    }
  }
  Meguro.emit(key,JSON.stringify(categories));
}
