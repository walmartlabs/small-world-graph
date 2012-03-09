struct ImageWorkUnit {
  1: string bucket;
  2: string link;
  3: i32 desired_size;
}

service ImageDownloadQueue {
  list<ImageWorkUnit> dequeue(),
  void enqueue(1: list<ImageWorkUnit> units)
}
