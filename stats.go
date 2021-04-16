package main

import (
	"context"
	"fmt"
	"net"
	"os"
	"os/signal"
	"syscall"
	"time"

	psLoad "github.com/shirou/gopsutil/v3/load"
	psNet "github.com/shirou/gopsutil/v3/net"
)

var IP = []byte{10, 0, 13, 50}
var PORT = 1234

type Stat struct {
	load float64
	down uint64
	up   uint64
}

func getStats(ctx context.Context) *Stat {
	s := new(Stat)
	avg, err := psLoad.AvgWithContext(ctx)
	if err != nil {
		fmt.Println(err)
		return s
	}
	s.load = avg.Load5
	io1, err := psNet.IOCountersWithContext(ctx, false)
	if err != nil {
		fmt.Println(err)
		return s
	}
	time.Sleep(time.Second)
	io2, err := psNet.IOCountersWithContext(ctx, false)
	if err != nil {
		fmt.Println(err)
		return s
	}

	s.down = io2[0].BytesRecv - io1[0].BytesRecv
	s.up = io2[0].BytesSent - io1[0].BytesSent

	return s
}

func sendStats(s *Stat) {
	Conn, _ := net.DialUDP("udp", nil, &net.UDPAddr{IP: IP, Port: PORT, Zone: ""})
	defer Conn.Close()

	o := fmt.Sprintf("%f,%d,%d", s.load, s.down, s.up)
	fmt.Println(o)
	_, _ = Conn.Write([]byte(o))
}

func main() {
	c := make(chan os.Signal, 1)
	signal.Notify(c, os.Interrupt, syscall.SIGTERM)
	ctx, cancel := context.WithCancel(context.Background())
mainLoop:
	for {
		select {
		case <-c:
			fmt.Println("bye bye")
			cancel()
			break mainLoop
		default:
			stat := getStats(ctx)
			sendStats(stat)
		}
	}

}
