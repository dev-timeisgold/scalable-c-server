# 1. 사용할 변수 세팅 (나중에 수정하기 편하게!)
CC = gcc
CFLAGS = -lpthread
TARGET = server

# 2. 기본 실행 (make만 쳤을 때 컴파일만 수행)
all: $(TARGET)

$(TARGET): server.c
	$(CC) server.c -o $(TARGET) $(CFLAGS)

# 3. 서버 실행 (make run을 쳤을 때 실행)
run: $(TARGET)
	./$(TARGET)

# 4. 청소 (make clean을 쳤을 때 빌드된 파일 삭제)
clean:
	rm -f $(TARGET)