struct CacheLine {
  uint8_t valid;
  uint32_t tag;
  uint32_t last_used;
};

struct Cache {
  uint32_t sets;
  uint32_t ways;
  CacheLine* lines;
};

struct SeenSet {
  uint64_t cap;         // power of 2
  uint64_t len;
  uint32_t *keys;
  uint8_t  *used;       // 0 = empty, 1 = occupied
};

struct Cachesim {
  uint32_t line_bytes;
  Cache    real;
  Cache    full;
  SeenSet  seen;
  
  uint64_t  hits;
  uint64_t  compulsory_misses;
  uint64_t  capacity_misses;
  uint64_t  conflict_misses;
};


static uint64_t next_pow2(uint64_t x) {
  uint64_t p = 1;
  while (p < x) p <<= 1;
  return p;
}

// 32-bit mix (splitmix32-ish)
static inline uint32_t hash32(uint32_t x) {
  x += 0x9e3779b9u;
  x ^= x >> 16;
  x *= 0x85ebca6bu;
  x ^= x >> 13;
  x *= 0xc2b2ae35u;
  x ^= x >> 16;
  return x;
}
static inline uint32_t hash32_u64(uint64_t k) {
  uint32_t x = (uint32_t)k ^ (uint32_t)(k >> 32);
  return hash32(x);
}

static SeenSet new_seenset(uint64_t expected_items) {
  SeenSet s = {};
  s.cap = next_pow2(expected_items * 2);
  s.len = 0;
  s.keys = (uint32_t*)calloc(s.cap, sizeof(uint32_t));
  s.used = (uint8_t*)calloc(s.cap, sizeof(uint8_t));
  return s;
}

static bool seenset_insert_if_new(SeenSet *s, uint32_t key);
static void seenset_grow(SeenSet* s, uint64_t new_cap) {
  SeenSet new_set = {
    .cap = new_cap,
    .len = 0,
    .keys = (uint32_t*)calloc(new_cap, sizeof(uint32_t)),
    .used = (uint8_t*)calloc(new_cap, sizeof(uint8_t)),
  };
  for (uint64_t i = 0; i < s->cap; i++) {
    if (s->keys[i]) {
      seenset_insert_if_new(&new_set, s->keys[i]);
    }
  }
  free(s->keys);
  free(s->used);
  *s = new_set;
}

static void delete_seenset(SeenSet *s) {
  free(s->keys);
  free(s->used);
  memset(s, 0, sizeof(*s));
}

static bool seenset_insert_if_new(SeenSet *s, uint32_t key) {
  if (2 * s->len >= s->cap) {
    seenset_grow(s, 2*s->cap);
  }
  uint64_t mask = s->cap - 1;
  uint64_t i = (uint64_t)(hash32_u64(key) & mask);

  while (s->used[i]) {
    if (s->keys[i] == key) return false;
    i = (i + 1) & mask;
  }
  s->used[i] = 1;
  s->keys[i] = key;
  s->len++;
  return true;
}

Cachesim new_cachesim(uint32_t sets, uint32_t ways, uint32_t line_bytes) {
  Cachesim sim = { .line_bytes = line_bytes};
  uint32_t lines = sets * ways;
  uint32_t total_bytes = lines * line_bytes;
  sim.seen = new_seenset(1024);

  sim.real.sets  = sets;
  sim.real.ways  = ways;
  sim.real.lines = (CacheLine*)calloc(lines, sizeof(CacheLine));

  sim.full.sets  = 1;
  sim.full.ways  = total_bytes / line_bytes;
  sim.full.lines = (CacheLine*)calloc(lines, sizeof(CacheLine));
  
  return sim;
}

static inline void split_line(Cache* c, uint32_t line_addr, uint32_t* set_idx, uint32_t* tag) {
  *tag = line_addr / c->sets;
  *set_idx = line_addr % c->sets;
}

static bool cache_access(Cache* c, uint32_t line_addr, uint64_t time) {
  uint32_t set_idx;
  uint32_t tag;
  split_line(c, line_addr, &set_idx, &tag);

  CacheLine* set = &c->lines[set_idx * c->ways];

  // Hit?
  for (uint32_t w = 0; w < c->ways; w++) {
    if (set[w].valid && set[w].tag == tag) {
      set[w].last_used = time;
      return true;
    }
  }

  // Miss: fill invalid if any, else evict LRU (smallest last_used)
  uint32_t victim = 0;
  bool found_invalid = false;

  for (uint32_t w = 0; w < c->ways; w++) {
    if (!set[w].valid) {
      victim = w;
      found_invalid = true;
      break;
    }
  }

  if (!found_invalid) {
    uint64_t best_time = set[0].last_used;
    victim = 0;
    for (uint32_t w = 1; w < c->ways; w++) {
      if (set[w].last_used < best_time) {
        best_time = set[w].last_used;
        victim = w;
      }
    }
  }

  set[victim].valid = 1;
  set[victim].tag = tag;
  set[victim].last_used = time;
  return false;
}

void cachesim_eval(Cachesim* sim, uint32_t pc, uint64_t time) {
  uint32_t line_addr = pc / sim->line_bytes;

  bool full_hit = cache_access(&sim->full, line_addr, time);
  bool real_hit = cache_access(&sim->real, line_addr, time);
  if (real_hit) {
    sim->hits++;
  }
  else if (seenset_insert_if_new(&sim->seen, line_addr)) {
    sim->compulsory_misses++;
  }
  else if (full_hit) {
    sim->capacity_misses++;
  }
  else {
    sim->conflict_misses++;
  }
}
