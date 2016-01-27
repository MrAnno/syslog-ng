#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>

/*
 * https://cgreen-devs.github.io/#_mocking_functions_with_cgreen
 */

char *read_paragraph(int (*read)(void *), void *stream)
{
  int buffer_size = 0, length = 0;
  char *buffer = NULL;
  int ch;
  while ((ch = (*read)(stream)) != EOF) {
    if (++length > buffer_size) {
      buffer_size += 100;
      buffer = (char *)realloc(buffer, buffer_size + 1);
    }
    if ((buffer[length - 1] = ch) == '\n') {
      buffer[--length] = '\0';
      break;
    }
    buffer[length] = '\0';
  }
  return buffer;
}




static int stream_stub(void *stream)
{
  return (int)mock(stream);
}

Describe(ParagraphReader);
BeforeEach(ParagraphReader) {}
AfterEach(ParagraphReader) {}

Ensure(ParagraphReader, gives_null_when_reading_empty_stream)
{
  always_expect(stream_stub, will_return(EOF));                                 
  assert_that(read_paragraph(&stream_stub, NULL), is_null);
}

Ensure(ParagraphReader, gives_one_character_line_for_one_character_stream)
{
  expect(stream_stub, will_return('a'));
  expect(stream_stub, will_return(EOF));
  char *line = read_paragraph(&stream_stub, NULL);
  assert_that(line, is_equal_to_string("a"));
  free(line);
}

Ensure(ParagraphReader, gives_one_word_line_for_one_word_stream)
{
  expect(stream_stub, will_return('t'));
  expect(stream_stub, will_return('h'));
  expect(stream_stub, will_return('e'));
  always_expect(stream_stub, will_return(EOF));
  assert_that(read_paragraph(&stream_stub, NULL), is_equal_to_string("the"));
}

Ensure(ParagraphReader, gives_empty_line_for_single_line_ending)
{
  expect(stream_stub, will_return('\n'));
  assert_that(read_paragraph(&stream_stub, NULL), is_equal_to_string(""));
}




void by_paragraph(int (*read)(void *), void *in, void (*write)(void *, char *), void *out)
{
  while (1) {
    char *line = read_paragraph(read, in);
    if ((line == NULL) || (strlen(line) == 0)) {
      return;
    }
    (*write)(out, line);
    free(line);
  }
}

static int reader(void *stream)
{
  return (int)mock(stream);
}

static void writer(void *stream, char *paragraph)
{
  mock(stream, paragraph);
}

Ensure(makes_one_letter_paragraph_from_one_character_input)
{
  expect(reader, will_return('a'));
  always_expect(reader, will_return(EOF));
  expect(writer, when(paragraph, is_equal_to_string("a")));
  by_paragraph(&reader, NULL, &writer, NULL);
}

Ensure(generates_separate_paragraphs_for_line_endings)
{
  expect(reader, will_return('a'));
  expect(reader, will_return('\n'));
  expect(reader, will_return('b'));
  expect(reader, will_return('\n'));
  expect(reader, will_return('c'));
  always_expect(reader, will_return(EOF));
  expect(writer, when(paragraph, is_equal_to_string("a")));
  expect(writer, when(paragraph, is_equal_to_string("b")));
  expect(writer, when(paragraph, is_equal_to_string("c")));
  by_paragraph(&reader, NULL, &writer, NULL);
}

Ensure(pairs_the_functions_with_the_resources)
{
  expect(reader, when(stream, is_equal_to(1)), will_return('a'));
  always_expect(reader, when(stream, is_equal_to(1)), will_return(EOF));
  expect(writer, when(stream, is_equal_to(2)));
  by_paragraph(&reader, (void *)1, &writer, (void *)2);
}

Ensure(ignores_empty_paragraphs)
{
  expect(reader, will_return('\n'));
  always_expect(reader, will_return(EOF));
  never_expect(writer);
  by_paragraph(&reader, NULL, &writer, NULL);
}

// auto discovery with 'cgreen-runner'
int main(int argc, char **argv)
{
  TestSuite *suite = create_test_suite();
  add_test_with_context(suite, ParagraphReader, gives_null_when_reading_empty_stream);
  add_test_with_context(suite, ParagraphReader, gives_one_character_line_for_one_character_stream);
  add_test_with_context(suite, ParagraphReader, gives_one_word_line_for_one_word_stream);
  add_test_with_context(suite, ParagraphReader, gives_empty_line_for_single_line_ending);
  add_test(suite, makes_one_letter_paragraph_from_one_character_input);
  add_test(suite, generates_separate_paragraphs_for_line_endings);
  add_test(suite, pairs_the_functions_with_the_resources);
  add_test(suite, ignores_empty_paragraphs);
  return run_test_suite(suite, create_text_reporter());
}
