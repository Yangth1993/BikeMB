#include <assert.h>
#include <string.h>

#include "../../src/firmware/bikemb/src/ai/qwen_chat_adapter.h"

int main() {
  char out[8] = {};
  const char *answer = "你好世界";
  const size_t written = BikeMbQwenChat_CopyBoundedAnswer(answer, out, sizeof(out));

  assert(written == 6);
  assert(strcmp(out, "你好") == 0);
  return 0;
}
