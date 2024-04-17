FROM alpine:latest

RUN apk update
RUN apk add build-base
RUN apk add python3
RUN apk add rust


COPY codeRunner.c /app/
WORKDIR /app
RUN gcc -o codeRunner codeRunner.c
CMD ["./codeRunner"]