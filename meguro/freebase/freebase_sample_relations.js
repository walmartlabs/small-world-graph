var bad_relation_first_parts = {"freebase":1, "pipeline":1, "type":1, "dataworld":1, "common":1, "user":1, "community":1};
var months = ['January', 'February', 'March', 'April', 'May', 'June', 'July', 'August', 'September', 'October', 'November', 'December'];

function map(key, value) {
  var parts = value.replace(/[\t\s]+$/,'').split(/\t+/);
  if (parts.length != 3) {
    return;
  }

  var k1 = parts[0];
  var nkh1 = Meguro.dictionary(k1);
  if (!nkh1) {
    return;
  }

  var r = parts[1];
  if (r.match(/^reverse_of:/) || r.match(/^master:/) || r.match(/[_\/u]id$/)) {
    return;
  }
  var rp = r.split(/\//);
  if (rp.length > 1 && bad_relation_first_parts.hasOwnProperty(rp[1])) {
    return;
  }

  nkh1 = JSON.parse(nkh1);
  var n1 = (nkh1.hasOwnProperty('name') ? nkh1['name'] : nkh1['key']);

  var k2 = parts[2];
  var n2;
  if (k2.match(/^\/guid\//)) {
    var nkh2 = Meguro.dictionary(k2);
    if (!nkh2) {
      return;
    }
    nkh2 = JSON.parse(nkh2);
    n2 = (nkh2.hasOwnProperty('name') ? nkh2['name'] : nkh2['key']);
  } else if (k2 == 'true' || k2 == 'false' || k2.match(/^https?:/)) {
    return;
  } else if (k2.match(/^-?[\d\.]+([eE]-?\d+)?$/)) { //number
    var lrp = rp[rp.length-1];
    if (!k2.match(/^\d{4}$/) || !k2.match(/^[\d\-]+$/) || (lrp.indexOf('year')<0 && lrp.indexOf('date')<0 && lrp.indexOf('introduced')<0 && lrp.indexOf('founded')<0 && lrp.indexOf('closed')<0)) { //suggests not a year
      return; //don't want plain numbers
    }
    n2 = k2;
  } else if (k2.match(/^\d{4}-\d\d(-\d\d)?$/)) {
    var dp = k2.split(/-/);
    n2 = (dp[2] ? dp[2].replace(/^0/,'')+' ' : '') + months[parseInt(dp[1]-1,10)] + ' ' + dp[0];
  } else {
    n2 = k2;
  }
  Meguro.emit(r, JSON.stringify([n1, n2]));
}


function reduce(key, values) {
  var npairs = values.length;
  var str = npairs+"\t"+key;
  var samples = [];
  for (var i=0, il=Math.min(3,npairs); i < il; ++i) {
    var pair = JSON.parse(values[i]);
    samples.push('"'+pair[0]+' '+key+' '+pair[1]+'"');
  }
  Meguro.save(key,str+"\t("+samples.join(', ')+')');
}

