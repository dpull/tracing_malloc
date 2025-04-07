package main

import (
	"flag"
	"log"
)

func main() {
	log.SetFlags(log.Ldate | log.Ltime | log.Lshortfile)

	mapsPath := flag.String("maps", "", "cache file of /proc/$PID/maps")
	flag.IntVar(&Addr2lineNum, "num", 8, "number of addr2line process")
	outputFormat := flag.String("format", "loli", "")
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
		snapshot, err := Analyze(file, maps, addr2line)
		if err != nil {
			log.Fatalf("analyze failed:%v", err)
		}
		err = output(file, *outputFormat, snapshot, addr2line)
		if err != nil {
			log.Fatalf("output failed:%v", err)
		}
	}

}

func output(file, format string, snapshot []BinData, addr2line *Addr2Line) error {
	switch format {
	case "text":
		return TextFormatOutput(file+".addr2line", snapshot, addr2line)
	case "loli":
		return LoliFormatOutput(file+".loli", snapshot, addr2line)
	}
	return nil
}
