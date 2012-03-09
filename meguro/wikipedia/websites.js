function handle_template(key,template) {
  if (template.title && template.title.match(/infobox/i)) {
    var params = template.parameters;
    for (var param_name in params) {
      if (param_name === 'website') {
        var website = params[param_name];
        if (website.length > 0) {
          Meguro.emit(key,'1');
          return;
        }
      }
    }
  }
}

function map(key,value) {
  json = JSON.parse(value);
  if (json) {
    for(var i=0; i < json.length; i++) {
      switch (json[i].type) {
        case 'template':
          handle_template(key,json[i]);
          break;
        default:
          break;
      }
    }
  }
}
