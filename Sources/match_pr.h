#ifdef Assert
#undef Assert
#include <stdio.h>
#define Assert(x, msg) { if (!(x)) { printf("Assertion: %s: %s %d: %s\n", msg, __FILE__, __LINE__, #x); exit(1); } }
#endif

typedef unsigned short int uint16_t;

static inline long get_match_len(const unsigned char *a, const unsigned char *b, long max)
 {
     register int len = 0;
     register unsigned long xor = 0;
     register int check_loops = max/sizeof(unsigned long);
     while(check_loops-- > 0) {
         xor = (*(unsigned long *)(a+len)) ^ (*(unsigned long *)(b+len));
         if (xor) break;
         len += sizeof(unsigned long);
     }
     if ((0 == xor)) {
         while (len < max) {
             if (a[len] != b[len]) break;
             len++;
         }
         return len;
     }
     xor = __builtin_ctzl(xor)>>3;
     return len + xor;
 }

local uInt longest_match(s, cur_match)
    deflate_state *s;
    IPos cur_match;                             /* current match */
{
    unsigned chain_length = s->max_chain_length;/* max hash chain length */
    register Bytef *scan = s->window + s->strstart; /* current string */
    register Bytef *match;                      /* matched string */
    register int len;                           /* length of current match */
    register int best_len = s->prev_length;              /* ignore strings, shorter or of the same length; rename ?? */
    int nice_match = s->nice_match;             /* stop if match long enough */
    int offset = 0;                             /* offset of current hash code */
    IPos limit = s->strstart > (IPos)MAX_DIST(s) ?
        s->strstart - (IPos)MAX_DIST(s) -1 : NIL;
    /* Stop when cur_match becomes <= limit. To simplify the code,
     * we prevent matches with the string of window index 0.
     */
    Bytef *match_head = s->window;

    Posf *prev = s->prev;                       /* lists of the hash chains */
    uInt wmask = s->w_mask;

    /* The code is optimized for HASH_BITS >= 8 and MAX_MATCH-2 multiple of 16.
     * It is easy to get rid of this optimization if necessary.
     */
    Assert(s->hash_bits >= 8 && MAX_MATCH == 258, "Code too clever");

    /* Do not look for matches beyond the end of the input. This is necessary
     * to make deflate deterministic.
     */
    Assert((ulg)s->strstart <= s->window_size-MIN_LOOKAHEAD, "need lookahead");

    if (best_len > MIN_MATCH) {
        /* search for most distant code */
        register int i;
        register ush hash = 0;
        register IPos pos;

        UPDATE_HASH(s, hash, scan[1]);
        UPDATE_HASH(s, hash, scan[2]);
        for (i = 3; i <= best_len; i++) {
            UPDATE_HASH(s, hash, scan[i]);
            pos = s->head[hash];
            if (pos < cur_match) {
                offset = i - 2;
                cur_match = pos;
            }
        }

        /* update variables to correspond offset */
        limit += offset;
        if (cur_match <= limit) return best_len;

        match_head -= offset;
    }

    do {
        /* Determine matched length at current pos */
        match = match_head + cur_match;
        len = get_match_len(match, scan, MAX_MATCH);

        if (len > best_len) {
            /* found longer string */
            s->match_start = cur_match - offset;
            best_len = len;
            /* good enough? */
            if (len >= nice_match) break;
            if (s->level > 3) {
                IPos pos, distant;
                register int i;
                register ush hash = 0;
                register int check_len = len - MIN_MATCH + 1;

                /* go back to offset 0 */
                cur_match -= offset; limit -= offset;
                offset = 0;
                distant = cur_match;

                for (i = 0; i <check_len; i++) {
                    pos = prev[(cur_match + i) & wmask];
                    if (pos < distant) {
                        /* this hash chain is more distant, use it */
                        distant = pos;
                        offset = i;
                    }
                }
                if (distant <= limit + offset) break;

                /* Try hash head at len-(MIN_MATCH-1) position to see if we could get
                 * a better cur_match at the end of string. Using (MIN_MATCH-1) lets
                 * us to include one more byte into hash - the byte which will be checked
                 * in main loop now, and which allows to grow match by 1.
                 */
                UPDATE_HASH(s, hash, scan[check_len]);
                UPDATE_HASH(s, hash, scan[check_len+1]);
                UPDATE_HASH(s, hash, scan[check_len+2]);
                pos = s->head[hash];
                if (pos < distant) {
                    offset = check_len;
                    distant = pos;
                }
                if (distant <= limit + offset) break;

                /* update offset-dependent vars */
                limit += offset;
                match_head = s->window - offset;
                cur_match = distant;
                continue;
            }
        }
        /* move to prev pos in this hash chain */
        cur_match = prev[cur_match & wmask];
    } while ((cur_match > limit) && --chain_length != 0);

    return ((uInt)best_len <= s->lookahead)? best_len : s->lookahead;
}
