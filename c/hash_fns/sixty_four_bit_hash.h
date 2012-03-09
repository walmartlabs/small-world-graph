class SixtyFourBitHash {
  public:
    size_t operator()(u_int64_t key) const
    {
      key = (~key) + (key << 18); // key = (key << 18) - key - 1;
      key = key ^ (key >> 31);
      key = key * 21; // key = (key + (key << 2)) + (key << 4);
      key = key ^ (key >> 11);
      key = key + (key << 6);
      key = key ^ (key >> 22);
      return (size_t) key;
    }
};

inline static u_int64_t composite_key(u_int32_t first,u_int32_t second)
{
  u_int64_t key = second;
  key <<= 32;
  key += first;
  return key;
}

struct decomposed_key {
  int site_id;
  int other_id;
};

inline static struct decomposed_key decompose_key(u_int64_t composite)
{
  struct decomposed_key dk;
  dk.site_id = composite;// & 0xffffffff;
  dk.other_id = (composite >> 32);// & 0xffffffff;
  return dk;
}
