FROM golang:1.8.3 as builder
WORKDIR /go/src/github.com/lad1337/cube
COPY stats.go .
RUN go get 
RUN CGO_ENABLED=0 GOOS=linux go build -a -installsuffix cgo -o stats .

FROM scratch
WORKDIR /root/
COPY --from=builder /go/src/github.com/lad1337/cube/stats .
ENV HOST_PROC=/host_proc
ENV HOTS_SYS=/host_sys
ENV HOST_ETC=/host_etc
ENV HOST_RUN=/host_run
ENV HOST_DEV=/host_dev
CMD ["./stats"]
