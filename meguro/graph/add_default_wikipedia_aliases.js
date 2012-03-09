/*
 * This map/reduce adds the default wikipedia alias and a downcase
 * Barack Obama -> ["barack Obama","barack obama"]
 */


// Return new array with duplicate values removed
Array.prototype.unique =
  function() {
    var a = [];
    var l = this.length;
    for(var i=0; i<l; i++) {
      for(var j=i+1; j<l; j++) {
        // If this[i] is found later in the array
        if (this[i] === this[j])
          j = ++i;
      }
      a.push(this[i]);
    }
    return a;
  };

function map(key,value) {
  var json = JSON.parse(value);
  var new_aliases = [];
  var lower_key = key.toLowerCase();
  if(lower_key != key)
    new_aliases.push(lower_key);
  if (json.common_name) {
    var lower_common = json.common_name.toLowerCase();
    if (lower_common != json.common_name) 
      new_aliases.push(lower_common);
  }

  var aliases = json.aliases;
  if (aliases) {
    for(var i=0; i < aliases.length; i++) {
      var alias = aliases[i];
      new_aliases.push(alias);
      var downcased_alias = alias.toLowerCase();
      new_aliases.push(downcased_alias);
    }
  }
  if (new_aliases.length > 0) 
    json.aliases = new_aliases.unique();

  Meguro.emit(key,JSON.stringify(json));
}
