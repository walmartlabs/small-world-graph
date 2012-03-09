function map(key, value) {
  var seen = {};
  seen[key] = 1;
  seen[value] = 1;
  var now = value;
  var next;
  while (next=Meguro.dictionary(now)) {
    if (seen.hasOwnProperty(next)) {
      Meguro.log('on '+key+', looped back to '+next);
      Meguro.emit(key, '|');
      return;
    }
    now = next;
    seen[now] = 1;
  }
  Meguro.emit(key, now);
}
