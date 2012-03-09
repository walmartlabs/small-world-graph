struct SiteRelationship {
  1: i32 first;
  2: i32 second;
  3: i16 distance;
}

service SiteRelationships {
  list<i32> sites_close_to(1:i32 site_id),
  list<SiteRelationship> distances(1:list<i32> site_ids)
}
