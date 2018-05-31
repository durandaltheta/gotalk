/*
This implementation is a copy of:
https://github.com/jamesroutley/write-a-hash-table
*/

typedef struct {
    char* key;
    void* value;
} ht_item;

typedef struct {
    int size;
    int count;
    ht_item** items;
} ht_hash_table;

static ht_item HT_DELETED_ITEM = {NULL, NULL};

// forward declarations
static ht_item* ht_new_item(const char* k, const void* v);
static void ht_del_item(ht_item* i);
static int ht_hash(const char* s, const int a, const int m);
static int ht_get_hash(const char* s, const int num_buckets, const int attempt);
static void ht_resize(ht_hash_table* ht, const int base_size);
static void ht_resize_up(ht_hash_table* ht);
static void ht_resize_down(ht_hash_table* ht);
static ht_hash_table* ht_new_sized(const int base_size);

// forward declarations of user facing functions
ht_hash_table* ht_new();
void ht_insert(ht_hash_table* ht, const char* key, const void* value);
void* ht_search(ht_hash_table* ht, const char* key);
void* ht_get_by_index(ht_hash_table* ht, const int index);
void ht_delete(ht_hash_table* h, const char* key);
