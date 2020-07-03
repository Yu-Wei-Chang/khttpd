/* Max. digits of BigN in decimal is 39. */
#define LEN_BN_STR 40

struct BigN {
    unsigned long long lower, upper;
};

struct BigN fib_sequence_fd(long long k);
void print_BigN_string(struct BigN fib_seq, char *bn_out, unsigned int digits);
