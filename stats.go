package main

import (
	"context"
	"errors"
	"fmt"
	"net"
	"os"
	"os/signal"
	"strconv"
	"syscall"
	"time"

	psLoad "github.com/shirou/gopsutil/v3/load"
	psNet "github.com/shirou/gopsutil/v3/net"
)

// https://stackoverflow.com/a/40326580
func getEnv(key, fallback string) string {
	if value, ok := os.LookupEnv(key); ok {
		return value
	}
	return fallback
}

var IP = getEnv("STATS_TRAGET_IP", "127.0.0.1")
var PORT = 1234
var INTERFACE = getEnv("STATS_SOURCE_INTERFACE", "bond0")
var INTERVAL = getEnv("STATS_REPORT_INTERVAL", "1")

type Stat struct {
	load float64
	down uint64
	up   uint64
}

func getInterfaceStat(interfaceName string, ctx context.Context) (psNet.IOCountersStat, error) {
	io, err := psNet.IOCountersWithContext(ctx, true)
	var interfaceStat psNet.IOCountersStat
	if err != nil {
		return interfaceStat, err
	}
	for _, s := range io {
		if s.Name == interfaceName {
			return s, nil
		}
	}
	return psNet.IOCountersStat{}, errors.New("no interface found")
}

func getStats(ctx context.Context) *Stat {
	s := new(Stat)
	avg, err := psLoad.AvgWithContext(ctx)
	if err != nil {
		fmt.Println(err)
		return s
	}
	s.load = avg.Load5

	stat1, err := getInterfaceStat(INTERFACE, ctx)
	if err != nil {
		fmt.Println(err)
		return s
	}
	time.Sleep(time.Second)
	stat2, err := getInterfaceStat(INTERFACE, ctx)
	if err != nil {
		fmt.Println(err)
		return s
	}

	s.down = stat2.BytesRecv - stat1.BytesRecv
	s.up = stat2.BytesSent - stat1.BytesSent

	return s
}

func sendStats(s *Stat, a *net.UDPAddr) {

	Conn, err := net.DialUDP("udp", nil, a)
	if err != nil {
		fmt.Println(err)
		return
	}
	defer Conn.Close()

	o := fmt.Sprintf("%f,%d,%d", s.load, s.down, s.up)
	fmt.Println(o)
	_, _ = Conn.Write([]byte(o))

}

func main() {
	c := make(chan os.Signal, 1)
	signal.Notify(c, os.Interrupt, syscall.SIGTERM)
	ctx, cancel := context.WithCancel(context.Background())
	interval, err := strconv.Atoi(INTERVAL)
	if err != nil {
		fmt.Println(err)
		return
	}
	ip, _, err := net.ParseCIDR(IP + "/24")
	if err != nil {
		fmt.Println("Error", IP, err)
		return
	}
	target := net.UDPAddr{IP: ip, Port: PORT, Zone: ""}
	fmt.Printf("Sending stats from %s to %s every %ds\n", INTERFACE, &target, interval)
	interval_ := time.Duration(interval - 1)
mainLoop:
	for {
		select {
		case <-c:
			fmt.Println("bye bye")
			cancel()
			break mainLoop
		default:
			stat := getStats(ctx)
			sendStats(stat, &target)
			time.Sleep(interval_ * time.Second)
		}
	}

}
