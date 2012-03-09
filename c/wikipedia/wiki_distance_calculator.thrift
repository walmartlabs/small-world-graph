struct WikiDistance {
  1: string first;
  2: string second;
  3: i32 distance;
}

service WikiDistanceCalculator {

  list<WikiDistance> distances(1:list<string> tuples),

  list<string> page_chain(1:string first, 2:string second)

}
