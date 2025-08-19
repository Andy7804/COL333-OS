#define NSWAP 6400 // 800 slots * 8 blocks each = 6400 blocks
#define NBPS 8 // 8 blocks per slot
#define NSLOT (NSWAP/NBPS) // 800 slots

// swapslot structure - metadata for each swap slot
struct swapslot {
    int page_perm;
    int is_free;
};

extern struct swapslot swap_slots[NSLOT]; // array of swap slots