
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
     register unsigned char *scan = s->window + s->strstart; /* current string */
     register unsigned char *match;                       /* matched string */
     register unsigned int len;                  /* length of current match */
     unsigned int best_len = s->prev_length;     /* best match length so far */
     unsigned int nice_match = s->nice_match;    /* stop if match long enough */
     IPos limit = s->strstart > (IPos)MAX_DIST(s) ?
         s->strstart - (IPos)MAX_DIST(s) : NIL;
     /* Stop when cur_match becomes <= limit. To simplify the code,
      * we prevent matches with the string of window index 0.
      */
     int offset = 0;  /* offset of the head[most_distant_hash] from IN cur_match */
     Pos *prev = s->prev;
     unsigned int wmask = s->w_mask;
     unsigned char *scan_buf_base = s->window;
 
     /* The code is optimized for HASH_BITS >= 8 and MAX_MATCH-2 multiple of 16.
      * It is easy to get rid of this optimization if necessary.
      */
     Assert(s->hash_bits >= 8 && MAX_MATCH == 258, "Code too clever");
 
     /* Do not look for matches beyond the end of the input. This is necessary
      * to make deflate deterministic.
      */
     if ((unsigned int)nice_match > s->lookahead) nice_match = s->lookahead;
 
     Assert((unsigned long)s->strstart <= s->window_size-MIN_LOOKAHEAD, "need lookahead");
 
     /* find most distant hash code for lazy_match */
     if (best_len > MIN_MATCH) {
         /* search for most distant hash code */
         register int i;
         register uint16_t hash = 0;
         register IPos pos;
 
         UPDATE_HASH(s, hash, scan[1]);
         UPDATE_HASH(s, hash, scan[2]);
         for (i = 3; i <= best_len; i++) {
             UPDATE_HASH(s, hash, scan[i]);
             /* get head IPos of hash calced by scan[i-2..i] */
             pos = s->head[hash];
             /* compare it to current "farthest hash" IPos */
             if (pos <= cur_match) {
                 /* we have a new "farthest hash" now */
                 offset = i - 2;
                 cur_match = pos;
             }
         }
 
         /* update variables to correspond offset */
         limit += offset;
         /*
          * check if the most distant code's offset is out of search buffer
          * if it is true, then this means scan[offset..offset+2] are not presented
          * in the search buffer. So we just return best_len we've found.
          */
         if (cur_match < limit) return best_len;
 
         scan_buf_base -= offset;
         /* reduce hash search depth based on best_len */
         chain_length /= best_len - MIN_MATCH;
     }
 
     do {
         Assert(cur_match < s->strstart, "no future");
 
         /* Determine matched length at current pos */
         match = scan_buf_base + cur_match;
         len = get_match_len(match, scan, MAX_MATCH);
 
         if (len > best_len) {
             /* found longer string */
             s->match_start = cur_match - offset;
             best_len = len;
             /* good enough? */
             if (len >= nice_match) break;
         }
         /* move to prev pos in this hash chain */
     } while ((cur_match = prev[cur_match & wmask]) > limit && --chain_length != 0);
 
     return (best_len <= s->lookahead)? best_len : s->lookahead;
}
