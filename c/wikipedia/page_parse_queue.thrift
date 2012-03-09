struct RawPage {
  1: string title;
  2: string content;
}

exception QueueFullException {
}

service PageParseQueue {
  list<RawPage> dequeue(),
  void enqueue(1: list<RawPage> pages) throws (1: QueueFullException qfe)
}
