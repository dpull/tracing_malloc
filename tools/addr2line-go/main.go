package main

import (
	"flag"
	"fmt"
	"log"
	"sort"
)

func main() {
	log.SetFlags(log.Ldate | log.Ltime | log.Lshortfile)

	mapsPath := flag.String("maps", "", "cache file of /proc/$PID/maps")
	flag.IntVar(&Addr2lineNum, "num", 8, "number of addr2line process")
	flag.Parse()

	if *mapsPath == "" {
		flag.Usage()
		return
	}

	maps, err := loadMaps(*mapsPath)
	if err != nil {
		log.Fatalf("load maps failed:%v", err)
	}

	files := flag.Args()
	addr2line := NewAddr2Line()

	for _, file := range files {
		err = analyze(file, maps, addr2line)
		if err != nil {
			log.Fatalf("analyze failed:%v", err)
		}
	}
}

func analyze(snapshotPath string, maps []MapInfo, addr2line *Addr2Line) error {
	data, err := loadBinFile(snapshotPath)
	if err != nil {
		return err
	}

	sort.Slice(data, func(i, j int) bool {
		return data[i].Time < data[j].Time
	})

	for _, item := range data {
		for _, address := range item.Stack {
			for _, module := range maps {
				if address >= module.Start && address < module.End {
					addr2line.Add(address, module.Start, module.Pathname)
					break
				}
			}
		}
	}

	err = addr2line.Execute()
	if err != nil {
		return err
	}

	for i := range data {
		var stackLine []string
		for _, address := range data[i].Stack {
			line := addr2line.GetFromCache(address)
			if line == "" {
				line = fmt.Sprintf("0x%x", address)
			}
			stackLine = append(stackLine, line)
		}
		data[i].StackLine = stackLine
	}

	return saveFile(snapshotPath+".addr2line", data)
}
