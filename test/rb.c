#include <ctype.h>
#include <u/libu.h>

#define RB_SZ   4096

int test_suite_rb_register (u_test_t *t);

static int rw (u_test_case_t *tc, int fast);
static int test_rw (u_test_case_t *tc);
static int test_rw_fast (u_test_case_t *tc);

static int rw (u_test_case_t *tc, int fast)
{
    enum { BUF_SZ = 1024 };
    int opts;
    u_rb_t *rb = NULL;
    size_t i, obuf_sz;
    char ibuf[BUF_SZ], obuf[BUF_SZ], *obuf_ptr, c;

    opts = fast ? U_RB_OPT_USE_CONTIGUOUS_MEM : U_RB_OPT_NONE;

    u_test_err_if (u_rb_create(RB_SZ, opts, &rb));

    c = '*';
    memset(ibuf, c, sizeof ibuf);

    /* 4 * 1024 write's => full */
    for (i = 0; i < 4; i++)
        u_test_err_if (u_rb_write(rb, ibuf, sizeof ibuf) != sizeof ibuf);

    /* now make offsets advance in the second region to test pairing */
    for (i = 0, c = 0; c < 127; c++)
    {
        if (!isprint(c))
            continue;

        /* consume 1024 bytes and test whether the read bytes match what was
         * written */
        if (fast)
        {
            obuf_sz = BUF_SZ;
            obuf_ptr = u_rb_fast_read(rb, &obuf_sz);
            u_test_err_if (obuf_ptr == NULL || obuf_sz != BUF_SZ);
        }
        else
        {
            u_test_err_if (u_rb_read(rb, obuf, sizeof obuf) != sizeof obuf);
            obuf_ptr = obuf;
        }

        /* u_rb_read is (4 * 1024) bytes behind */
        if (++i > 4)
            u_test_err_ifm (obuf_ptr[0] != (c - 4) || 
                    obuf_ptr[BUF_SZ - 1] != (c - 4), 
                    "expecting \'%c\', got \'%c\'", c - 4, obuf_ptr[0]);
        else
            u_test_err_ifm (obuf_ptr[0] != '*' || obuf_ptr[BUF_SZ - 1] != '*', 
                    "expecting \'%c\', got \'%c\'", '*', obuf_ptr[0]);

        /* refill */
        memset(ibuf, c, sizeof ibuf);
        u_test_err_if (u_rb_write(rb, ibuf, sizeof ibuf) != sizeof ibuf);
    }

    u_rb_free(rb);

    return U_TEST_SUCCESS;
err:
    if (rb)
        u_rb_free(rb);
    return U_TEST_FAILURE;
}

static int test_rw (u_test_case_t *tc) { return rw(tc, 0); }
static int test_rw_fast (u_test_case_t *tc) { return rw(tc, 1); }

int test_suite_rb_register (u_test_t *t)
{
    u_test_suite_t *ts = NULL;

    con_err_if (u_test_suite_new("Ring Buffer", &ts));

    con_err_if (u_test_case_register("Read-write", test_rw, ts));
    con_err_if (u_test_case_register("Read-write fast", test_rw_fast, ts));

    return u_test_suite_add(ts, t);
err:
    u_test_suite_free(ts);
    return ~0;
}
