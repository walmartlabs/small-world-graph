function map(key, value) {
  var json = JSON.parse(value);
  for (var i=0, il=json.length; i < il; ++i) {
    var v = JSON.parse(json[i]);
    if (v.ol) {
      v = v.ol;
      var canon_out = '';
      var seen = {};
      for (var j=0, jl=v.length; j < jl; ++j) {
        var lnk = v[j];
        var targ = Meguro.dictionary(lnk);
        if (targ) {
          if (targ == '|') { //goes nowhere
            continue;
          }
        } else {
          targ = lnk;
        }
        if (!seen.hasOwnProperty(targ)) {
          canon_out += '|' + targ;
          seen[targ] = 1;
        }
      }
      if (canon_out.length > 0) {
        Meguro.emit(key, canon_out.substr(1));
      }
      break;
    }
  }
}
