#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h> // include before cmocka.h - but why?!
#include <cmocka.h>
#include <string.h>
#include <glib.h>

#include "ringbuffer.h"

static const size_t capacity = 47;

typedef struct _TestData
{
  int idx;
  gboolean ack;
} TestData;

static void
_ringbuffer_fill(RingBuffer *rb, size_t n, int start_idx, gboolean ack)
{
  TestData *td;
  int i;

  for (i = 0; i < n; i++)
    {
      td = ring_buffer_push(rb);
      td->idx = start_idx + i;
      td->ack = ack;
    }
}

static void
_ringbuffer_fill2(RingBuffer *rb, size_t n, int start_idx, gboolean ack)
{
  TestData *td;
  int i;

  for (i = 0; i < n; i++)
    {
      td = ring_buffer_tail(rb);
      td->idx = start_idx + i;
      td->ack = ack;
      assert_true(("Push should return last tail.", ring_buffer_push(rb) == td));
    }
}

static gboolean
_is_continual(void *data)
{
  TestData *td = (TestData *)data;

  return td->ack;
}

static void
assert_continual_range_length_equals(RingBuffer *rb, size_t expected)
{
  size_t range_len = ring_buffer_get_continual_range_length(rb, _is_continual);
  assert_true(range_len == expected);
}

static void
assert_test_data_idx_range_in(RingBuffer *rb, int start, int end)
{
  TestData *td;
  int i;

  assert_true(ring_buffer_count(rb) == end-start+1);

  for (i = start; i <= end; i++)
    {
      td = ring_buffer_element_at(rb, i - start);
      assert_true(td->idx == i);
    }
}



static int
setup(void **state)
{
  RingBuffer *rb = g_new(RingBuffer, 1);
  ring_buffer_alloc(rb, sizeof(TestData), capacity);

  *state = rb;

  return 0;
}

static int
teardown(void **state)
{
  RingBuffer *rb = *state;

  ring_buffer_free(rb);
  g_free(*state);

  return 0;
}

static void
test_init_buffer_state(void **state)
{
  RingBuffer *rb = *state;

  assert_false(("buffer should not be full", ring_buffer_is_full(rb)));
  assert_true(("buffer should be empty", ring_buffer_is_empty(rb)));
  assert_true(("buffer should be empty", ring_buffer_count(rb) == 0));
  assert_true(("invalid buffer capacity", ring_buffer_capacity(rb) == capacity));
}

static void
test_pop_from_empty_buffer(void **state)
{
  RingBuffer *rb = *state;

  assert_true(("cannot pop from empty buffer", ring_buffer_pop(rb) == NULL));
}

static void
test_push_to_full_buffer(void **state)
{
  RingBuffer *rb = *state;

  _ringbuffer_fill(rb, capacity, 1, TRUE);
  assert_true(("cannot push to a full buffer", ring_buffer_push(rb) == NULL));
}

static void
test_ring_buffer_is_full(void **state)
{
  RingBuffer *rb = *state;
  int i;
  TestData *last = NULL;

  for (i = 1; !ring_buffer_is_full(rb); i++)
    {
      TestData *td = ring_buffer_push(rb);
      assert_true(("ring_buffer_push failed", td != NULL));
      td->idx = i;
      last = td;
    }

  assert_true(ring_buffer_count(rb) == capacity);
  assert_true(last->idx == capacity);
}

static void
test_pop_all_pushed_element_in_correct_order(void **state)
{
  RingBuffer *rb = *state;
  int cnt = 0;
  const int start_from = 1;
  TestData *td = NULL;

  _ringbuffer_fill(rb, capacity, 1, TRUE);

  while ((td = ring_buffer_pop(rb)))
    {
      assert_true((cnt+start_from) == td->idx);
      ++cnt;
    }

  assert_true(cnt == capacity);
}

static void
test_drop_elements(void **state)
{
  (void) state; /* unused */

  RingBuffer rb;
  const int rb_capacity = 103;
  const int drop = 31;

  ring_buffer_alloc(&rb, sizeof(TestData), rb_capacity);

  _ringbuffer_fill(&rb, rb_capacity, 1, TRUE);

  ring_buffer_drop(&rb, drop);
  assert_true(("drop failed", ring_buffer_count(&rb) == (rb_capacity - drop)));

  ring_buffer_free(&rb);
}

static void
test_elements_ordering(void **state)
{
  RingBuffer *rb = *state;
  TestData *td;
  int start_from = 10;
  int cnt = 0;

  _ringbuffer_fill(rb, capacity, start_from, TRUE);

  while ( (td = ring_buffer_pop(rb)) )
    {
      assert_true((cnt + start_from) == td->idx);
      ++cnt;
    }
}

static void
test_element_at(void **state)
{
  RingBuffer *rb = *state;
  size_t i;
  TestData *td;

  _ringbuffer_fill(rb, capacity, 0, TRUE);

  for ( i = 0; i < ring_buffer_count(rb); i++ )
    {
      td = ring_buffer_element_at(rb, i);
      assert_true(td != NULL);
      assert_true(td->idx == i);
    }
}

static void
test_continual_range(void **state)
{
  RingBuffer *rb = *state;

  _ringbuffer_fill(rb, capacity, 1, TRUE);
  assert_continual_range_length_equals(rb, capacity);
}

static void
test_zero_length_continual_range(void **state)
{
  RingBuffer *rb = *state;
  TestData *td;

  _ringbuffer_fill(rb, capacity, 1, TRUE);

  td = ring_buffer_element_at(rb, 0);
  td->ack = FALSE;

  assert_continual_range_length_equals(rb, 0);
}

static void
test_broken_continual_range(void **state)
{
  RingBuffer *rb = *state;
  TestData *td;

  _ringbuffer_fill(rb, capacity, 1, TRUE);

  td = ring_buffer_element_at(rb, 13);
  td->ack = FALSE;

  assert_continual_range_length_equals(rb, 13);
}

static void
test_push_after_pop(void **state)
{
  (void) state; /* unused */
}

static void
test_tail(void **state)
{
  (void) state; /* unused */

  RingBuffer rb;
  TestData *td_tail;

  ring_buffer_alloc(&rb, sizeof(TestData), 103);
  _ringbuffer_fill2(&rb, 103, 0, TRUE);

  ring_buffer_pop(&rb);

  td_tail = ring_buffer_tail(&rb);
  td_tail->idx = 103;

  assert_true(("Push should return last tail.", ring_buffer_push(&rb) == td_tail));

  assert_test_data_idx_range_in(&rb, 1, 103);

  ring_buffer_free(&rb);
}

int
main(void)
{
  struct CMUnitTest tests[] = {
    cmocka_unit_test_setup_teardown(test_init_buffer_state, setup, teardown),
    cmocka_unit_test_setup_teardown(test_pop_from_empty_buffer, setup, teardown),
    cmocka_unit_test_setup_teardown(test_push_to_full_buffer, setup, teardown),
    cmocka_unit_test_setup_teardown(test_ring_buffer_is_full, setup, teardown),
    cmocka_unit_test_setup_teardown(test_pop_all_pushed_element_in_correct_order, setup, teardown),
    cmocka_unit_test(test_drop_elements),
    cmocka_unit_test_setup_teardown(test_elements_ordering, setup, teardown),
    cmocka_unit_test_setup_teardown(test_element_at, setup, teardown),
    cmocka_unit_test_setup_teardown(test_continual_range, setup, teardown),
    cmocka_unit_test_setup_teardown(test_zero_length_continual_range, setup, teardown),
    cmocka_unit_test_setup_teardown(test_broken_continual_range, setup, teardown),
    cmocka_unit_test_setup_teardown(test_push_after_pop, setup, teardown),
    cmocka_unit_test(test_tail)
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
