#include <cgreen/cgreen.h>

#include "dnscache.h"
#include "apphook.h"
#include "timeutils.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>

static const char *positive_hostname = "hostname";
static const char *negative_hostname = "negative";

static void
fill_dns_cache(gint cache_size, gint expire, gint expire_failed)
{
  gint i;

  dns_cache_set_params(5 * cache_size, expire, expire_failed, NULL);

  for (i = 0; i < cache_size; i++)
    {
      guint32 ni = htonl(i);
      gboolean positive = i < cache_size / 2;
      dns_cache_store_dynamic(AF_INET, (void *) &ni, positive ? positive_hostname : negative_hostname, positive);
    }
}

static void
fill_benchmark_dns_cache(gint cache_size, gint expire, gint expire_failed)
{
  gint i;
  dns_cache_set_params(5 * cache_size, expire, expire_failed, NULL);

  for (i = 0; i < cache_size; i++)
    {
      guint32 ni = htonl(i);
      dns_cache_store_dynamic(AF_INET, (void *) &ni, positive_hostname, TRUE);
    }
}

static void
invalidate_with_sleep(guint seconds)
{
  sleep(seconds);
  invalidate_cached_time();
}

static void
assert_no_forget(gint cache_size)
{
  gint i;
  const gchar *hn = NULL;
  gsize hn_len;
  gboolean positive;
  gint positive_limit = cache_size / 2;

  for (i = 0; i < cache_size; i++)
    {
      guint32 ni = htonl(i);

      hn = NULL;
      positive = FALSE;

      assert_true_with_message(dns_cache_lookup(AF_INET, (void *) &ni, &hn, &hn_len, &positive),
        "hmmm cache forgot the cache entry too early, i=%d, hn=%s\n",
        i, hn);

      if (i < positive_limit)
        {
          assert_true_with_message(positive && strcmp(hn, "hostname") == 0,
            "hmm, cached returned an positive match, but cached name invalid, i=%d, hn=%s\n",
            i, hn);
        }
      else
        {
          assert_true_with_message(!positive && strcmp(hn, "negative") == 0,
            "hmm, cache returned a positive match, where a negative match was expected, i=%d, hn=%s\n",
            i, hn);
        }

    }
}

static void
assert_forget_negative(gint cache_size)
{
  gint i;
  const gchar *hn = NULL;
  gsize hn_len;
  gboolean positive;
  gint positive_limit = cache_size / 2;

  for (i = 0; i < cache_size; i++)
    {
      guint32 ni = htonl(i);

      hn = NULL;
      positive = FALSE;
      if (i < positive_limit)
        {
          assert_true_with_message(dns_cache_lookup(AF_INET, (void *) &ni, &hn, &hn_len, &positive) || !positive,
            "hmmm cache forgot positive entries too early, i=%d\n",
            i);
        }
      else
        {
          assert_false_with_message(dns_cache_lookup(AF_INET, (void *) &ni, &hn, &hn_len, &positive) || positive,
            "hmmm cache didn't forget negative entries in time, i=%d\n",
            i);
        }
    }
}

static void
assert_forget_all(gint cache_size)
{
  gint i;
  const gchar *hn = NULL;
  gsize hn_len;
  gboolean positive;

  for (i = 0; i < cache_size; i++)
    {
      guint32 ni = htonl(i);

      hn = NULL;
      positive = FALSE;
      assert_false_with_message(dns_cache_lookup(AF_INET, (void *) &ni, &hn, &hn_len, &positive),
            "hmmm cache did not forget an expired entry, i=%d\n",
            i);
    }
}


Describe(DNSCache);
BeforeEach(DNSCache) { app_startup(); }
AfterEach(DNSCache) { app_shutdown(); }

Ensure(DNSCache, test_expiration)
{
  gint cache_size = 1000;
  gint expire = 3;
  gint expire_failed = 1;

  fill_dns_cache(cache_size, expire, expire_failed);

  assert_no_forget(cache_size);

  /* negative entries should expire by now, positive ones still present */
  invalidate_with_sleep(2);
  assert_forget_negative(cache_size);


  /* everything should be expired by now */
  invalidate_with_sleep(2);
  assert_forget_all(cache_size);
}

Ensure(DNSCache, test_dns_cache_benchmark)
{
  GTimeVal start, end;
  const gchar *hn;
  gsize hn_len;
  gboolean positive;
  gint i;

  gint cache_size = 10000;
  gint expire = 600;
  gint expire_failed = 300;

  fill_benchmark_dns_cache(cache_size, expire, expire_failed);

  g_get_current_time(&start);
  /* run benchmarks */
  for (i = 0; i < cache_size; i++)
    {
      guint32 ni = htonl(i % cache_size);

      hn = NULL;
      dns_cache_lookup(AF_INET, (void *) &ni, &hn, &hn_len, &positive);
      //assert_that(dns_cache_lookup(AF_INET, (void *) &ni, &hn, &hn_len, &positive), is_true);
    }
  g_get_current_time(&end);
  printf("DNS cache speed: %12.3f iters/sec\n", i * 1e6 / g_time_val_diff(&end, &start));
}

// CGREEN EXCEPTION: Too many assertions within a single test.
int main(int argc, char **argv) {
    TestSuite *suite = create_test_suite();
    add_test_with_context(suite, DNSCache, test_expiration);
    add_test_with_context(suite, DNSCache, test_dns_cache_benchmark);
    return run_test_suite(suite, create_text_reporter());
}
