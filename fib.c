#include "fib.h"
#include <linux/kernel.h>
#include <linux/string.h>

/* Reverse given string */
static void reverse_string(char *str)
{
    if (!str)
        return;

    for (int i = 0; i < (strlen(str) / 2); i++) {
        char c = str[i];
        str[i] = str[strlen(str) - 1 - i];
        str[strlen(str) - 1 - i] = c;
    }
}

/* Add str_num and str_addend, and store result in str_sum.
 * digit which exceed value of "digits" will be truncated.
 * return -1 if any failed, or string length of result. */
static int string_of_number_add(char *str_sum,
                                char *str_addend,
                                unsigned int digits)
{
    int length;
    int idx = 0, carry = 0;
    char str_tmp[LEN_BN_STR] = "";

    if (!str_sum || !str_addend || (digits == 0))
        return -1;

    /* Reverse string because we process it from lowest digit. */
    reverse_string(str_sum);
    reverse_string(str_addend);

    /* Take the longer length, because we need to go through all digits. */
    length = (strlen(str_sum) > strlen(str_addend)) ? strlen(str_sum)
                                                    : strlen(str_addend);

    /* Keep calculate if both str_sum and str_addend does not meet '\0'. */
    for (int i = 0, j = 0; (i < length) && (j < length);) {
        int sum = 0;

        if (str_sum[i] != '\0') {
            sum += (str_sum[i++] - '0');
        }
        if (str_addend[j] != '\0') {
            sum += (str_addend[j++] - '0');
        }

        if (idx < digits)
            str_tmp[idx++] = ((sum + carry) % 10) + '0';

        carry = (sum + carry) / 10;
    }

    if ((idx < digits) && carry)
        str_tmp[idx++] = carry + '0';

    str_tmp[idx] = '\0';

    /* Reverse string back */
    reverse_string(str_tmp);
    reverse_string(str_addend);

    snprintf(str_sum, digits, "%s", str_tmp);

    return (int) strlen(str_sum);
}

/* Multiply multiplicand with miltiplier, and store result in multiplicand.
 * digit which exceed value of "digits" will be truncated.
 * return -1 if any failed, or string length of result. */
static int string_of_number_multiply(char *multiplicand,
                                     char *multiplier,
                                     unsigned int digits)
{
    char str_tmp[LEN_BN_STR] = "";
    char str_product[LEN_BN_STR] = "";
    int carry = 0;

    if (!multiplicand || !multiplier || (digits == 0) ||
        (strlen(multiplicand) == 0) || (strlen(multiplier) == 0))
        return -1;

    /* Sepcial case of "0" */
    if ((strlen(multiplier) == 1 && multiplier[0] == '0') ||
        (strlen(multiplicand) == 1 && multiplicand[0] == '0')) {
        snprintf(multiplicand, digits, "%s", "0");
        return (int) strlen(multiplicand);
    }

    /* Reverse string because we process it from lowest digit. */
    reverse_string(multiplicand);
    reverse_string(multiplier);

    for (int i = 0; i < strlen(multiplier); i++) {
        int idx = 0;

        for (int decuple = i; (idx < digits) && (decuple > 0); decuple--)
            str_tmp[idx++] = '0';

        for (int j = 0; j < strlen(multiplicand); j++) {
            int product = (multiplicand[j] - '0') * (multiplier[i] - '0');
            if (idx < digits)
                str_tmp[idx++] = ((product + carry) % 10) + '0';
            carry = (product + carry) / 10;
        }

        if ((idx < digits) && carry) {
            str_tmp[idx++] = carry + '0';
            carry = 0;
        }
        str_tmp[idx] = '\0';

        reverse_string(str_tmp);
        string_of_number_add(str_product, str_tmp, sizeof(str_product) - 1);
    }

    /* Reverse back */
    reverse_string(multiplier);
    snprintf(multiplicand, digits, "%s", str_product);

    return (int) strlen(multiplicand);
}

/* Convert BigN to string which represent in decimal */
void print_BigN_string(struct BigN fib_seq, char *bn_out, unsigned int digits)
{
    char bn_scale[LEN_BN_STR] = "18446744073709551616";
    char bn_lower[LEN_BN_STR] = "";
    char bn_upper[LEN_BN_STR] = "";

    if (!bn_out)
        return;

    snprintf(bn_out, digits, "%d", 0);
    snprintf(bn_lower, sizeof(bn_lower), "%llu", fib_seq.lower);
    snprintf(bn_upper, sizeof(bn_upper), "%llu", fib_seq.upper);

    string_of_number_multiply(bn_scale, bn_upper, sizeof(bn_scale));
    string_of_number_add(bn_out, bn_scale, digits);
    string_of_number_add(bn_out, bn_lower, digits);
}

static inline void addBigN(struct BigN *output, struct BigN x, struct BigN y)
{
    output->upper = x.upper + y.upper;
    if (y.lower > ~x.lower)
        output->upper++;
    output->lower = x.lower + y.lower;
}

static inline void multiBigN(struct BigN *output, struct BigN x, struct BigN y)
{
    size_t w = 8 * sizeof(unsigned long long);
    struct BigN product = {.upper = 0, .lower = 0};

    for (size_t i = 0; i < w; i++) {
        if ((y.lower >> i) & 0x1) {
            struct BigN tmp;

            product.upper += x.upper << i;

            tmp.lower = (x.lower << i);
            tmp.upper = i == 0 ? 0 : (x.lower >> (w - i));
            addBigN(&product, product, tmp);
        }
    }

    for (size_t i = 0; i < w; i++) {
        if ((y.upper >> i) & 0x1) {
            product.upper += (x.lower << i);
        }
    }
    output->upper = product.upper;
    output->lower = product.lower;
}

static inline void subBigN(struct BigN *output, struct BigN x, struct BigN y)
{
    if ((x.upper >= y.upper) && (x.lower >= y.lower)) {
        output->upper = x.upper - y.upper;
        output->lower = x.lower - y.lower;
    } else {
        output->upper = x.upper - y.upper - 1;
        output->lower = ULLONG_MAX + x.lower - y.lower + 1;
    }
}

struct BigN fib_sequence_fd(long long k)
{
    struct BigN t1 = {.upper = 0, .lower = 0}, t2 = {.upper = 0, .lower = 0};
    struct BigN a = {.upper = 0, .lower = 0}, b = {.upper = 0, .lower = 1};
    struct BigN multi_two = {.upper = 0, .lower = 2},
                tmp = {.upper = 0, .lower = 0};
    /* The position of the highest bit of k. */
    /* So we need to loop `rounds` times to get the answer. */
    int rounds = 0;
    for (int i = k; i; ++rounds, i >>= 1)
        ;

    for (int i = rounds; i > 0; i--) {
        // t1 = a * (2 * b - a); /* F(2k) = F(k)[2F(k+1) − F(k)] */
        multiBigN(&t1, b, multi_two);
        subBigN(&t1, t1, a);
        multiBigN(&t1, a, t1);
        // t2 = b * b + a * a;   /* F(2k+1) = F(k+1)^2 + F(k)^2 */
        multiBigN(&t2, b, b);
        multiBigN(&tmp, a, a);
        addBigN(&t2, t2, tmp);

        if ((k >> (i - 1)) & 1) {
            a = t2; /* Threat F(2k+1) as F(k) next round. */
            addBigN(&b, t1,
                    t2); /* Threat F(2k) + F(2k+1) as F(k+1) next round. */
        } else {
            a = t1; /* Threat F(2k) as F(k) next round. */
            b = t2; /* Threat F(2k+1) as F(k+1) next round. */
        }
    }

    return a;
}
