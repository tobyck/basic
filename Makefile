CC=clang
BUILD_DIR=./build
OUT_FILE=$(BUILD_DIR)/tinybc
TEST_FILE=./examples/test.bas

build:
	$(CC) src/*.c -o $(OUT_FILE) -lm $(CC_ARGS)

run:
	$(OUT_FILE) $(TEST_FILE)

leak-check:
	valgrind --leak-check=full \
      --show-leak-kinds=all \
      --track-origins=yes \
	  $(OUT_FILE) $(TEST_FILE)

clean:
	rm $(BUILD_DIR) -rf
