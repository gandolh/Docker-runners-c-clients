FROM alpine:latest

RUN apk update
RUN apk add build-base
RUN apk add python3
RUN apk add rust


COPY codeRunner.c cJSON.c cJSON.h uuid4.c uuid4.h /app/
WORKDIR /app
RUN gcc -o codeRunner codeRunner.c cJSON.c uuid4.c -lm
CMD ["./codeRunner"]