package main

import (
	"bufio"
	"bytes"
	"encoding/binary"
	"fmt"
	"log"
	"os"
	"os/exec"
	"regexp"
	"sort"
	"strconv"
	"strings"
	"sync"

	"github.com/pkg/errors"
)

var Addr2lineNum int

type Addr2Line struct {
	fbaseToFname    map[uint64]string
	fbaseToAddrset  map[uint64]map[uint64]struct{}
	isSOCache       map[string]bool
	addressToLine   map[uint64]string
	addressToLineMu sync.Mutex
}

func NewAddr2Line() *Addr2Line {
	return &Addr2Line{
		fbaseToFname:   make(map[uint64]string),
		fbaseToAddrset: make(map[uint64]map[uint64]struct{}),
		isSOCache:      make(map[string]bool),
		addressToLine:  make(map[uint64]string),
	}
}

func (a *Addr2Line) Add(address, fbase uint64, fname string) {
	a.fbaseToFname[fbase] = fname
	if _, ok := a.fbaseToAddrset[fbase]; !ok {
		a.fbaseToAddrset[fbase] = make(map[uint64]struct{})
	}
	a.fbaseToAddrset[fbase][address] = struct{}{}
}

func (a *Addr2Line) Execute() error {
	for fbase, addrset := range a.fbaseToAddrset {
		fname := a.fbaseToFname[fbase]

		var addresses []uint64
		for addr := range addrset {
			addresses = append(addresses, addr)
		}
		err := a.addr2line(addresses, fbase, fname)
		if err != nil {
			return err
		}
	}
	return nil
}

func (a *Addr2Line) GetFromCache(address uint64) string {
	return a.addressToLine[address]
}

func (a *Addr2Line) addr2line(addrset []uint64, fbase uint64, fname string) error {
	isSO, err := a.isSharedObject(fname)
	if err != nil {
		return errors.WithStack(err)
	}

	if isSO {
		for i := range addrset {
			addrset[i] = addrset[i] - fbase
		}
	}

	maxAddrLine := len(addrset)/Addr2lineNum + 1

	var wg sync.WaitGroup
	startPos := 0
	for startPos < len(addrset) {
		var addrlist []uint64
		if startPos+maxAddrLine < len(addrset) {
			addrlist = addrset[startPos : startPos+maxAddrLine]
		} else {
			addrlist = addrset[startPos:]
		}
		startPos += maxAddrLine

		wg.Add(1)
		go func() {
			defer wg.Done()

			for i := 0; i < 8; i++ {
				addrlist = a.popen(addrlist, fname, isSO, fbase)
				if addrlist == nil {
					break
				}
			}
		}()
	}

	wg.Wait()
	return nil
}

func (a *Addr2Line) popen(addrlist []uint64, fname string, isSO bool, fbase uint64) []uint64 {
	var stdin bytes.Buffer
	var stdout bytes.Buffer
	var stderr bytes.Buffer
	cmd := exec.Command("addr2line", "-Cfse", fname)
	cmd.Stdin = &stdin
	cmd.Stdout = &stdout
	cmd.Stderr = &stderr

	for _, v := range addrlist {
		stdin.WriteString(fmt.Sprintf("0x%x\n", v))
	}

	err := cmd.Run()
	if err != nil && stdout.Len() == 0 {
		log.Printf("cmd run failed(0x%x):%v", addrlist[0], err)
		return addrlist
	}

	if stderr.Len() > 0 {
		log.Printf("cmd run error:%s", stderr.String())
	}

	scanner := bufio.NewScanner(&stdout)
	for i, addr := range addrlist {
		addrRaw := addr
		if isSO {
			addrRaw = addr + fbase
		}

		if !scanner.Scan() {
			log.Printf("read function failed:%d, 0x%x", i, addrlist[i])
			return addrlist[i:]
		}
		function := strings.TrimSpace(scanner.Text())

		if !scanner.Scan() {
			log.Printf("read file:line failed:%d, 0x%x", i, addrlist[i])
			return addrlist[i:]
		}
		file := strings.TrimSpace(scanner.Text())

		line := fmt.Sprintf("%s\t%s\t%s", function, file, fname)

		a.addressToLineMu.Lock()
		a.addressToLine[addrRaw] = line
		a.addressToLineMu.Unlock()
	}

	return nil
}

func (a *Addr2Line) isSharedObject(filePath string) (bool, error) {
	if result, ok := a.isSOCache[filePath]; ok {
		return result, nil
	}

	file, err := os.Open(filePath)
	if err != nil {
		return false, errors.WithStack(err)
	}
	defer file.Close()

	// 移动到EI_IDENT位置
	if _, err := file.Seek(16, 0); err != nil {
		return false, errors.WithStack(err)
	}

	var eiType uint16
	if err := binary.Read(file, binary.LittleEndian, &eiType); err != nil {
		return false, errors.WithStack(err)
	}

	isSO := eiType == 3
	a.isSOCache[filePath] = isSO
	return isSO, nil
}

type BinData struct {
	Ptr      int64
	Time     int64
	Size     int64
	Stack    []uint64
	UserData interface{}
}

func loadBinFile(filePath string) ([]BinData, error) {
	file, err := os.Open(filePath)
	if err != nil {
		return nil, errors.WithStack(err)
	}
	defer file.Close()

	data := make([]BinData, 0, 65536)
	buf := make([]byte, 256) // 8*4+8*28 = 256 bytes

	for {
		n, err := file.Read(buf)
		if err != nil || n == 0 {
			break
		}

		if n != len(buf) {
			return nil, errors.Errorf("incorrect file format: %d != %d", n, len(buf))
		}

		reader := bytes.NewReader(buf)

		var item BinData
		var reserve int64

		if err := binary.Read(reader, binary.LittleEndian, &item.Ptr); err != nil {
			return nil, errors.Errorf("read ptr filed: %v", err)
		}
		if err := binary.Read(reader, binary.LittleEndian, &item.Time); err != nil {
			return nil, errors.Errorf("read time filed: %v", err)
		}
		if err := binary.Read(reader, binary.LittleEndian, &item.Size); err != nil {
			return nil, errors.Errorf("read size filed: %v", err)
		}
		if err := binary.Read(reader, binary.LittleEndian, &reserve); err != nil {
			return nil, errors.Errorf("read reserve filed: %v", err)
		}

		item.Stack = make([]uint64, 28)
		if err := binary.Read(reader, binary.LittleEndian, &item.Stack); err != nil {
			return nil, fmt.Errorf("read stack filed: %v", err)
		}

		if item.Ptr <= 0 {
			continue
		}

		lastNonZero := len(item.Stack) - 1
		for ; lastNonZero >= 0 && item.Stack[lastNonZero] == 0; lastNonZero-- {
		}
		if lastNonZero >= 0 {
			item.Stack = item.Stack[:lastNonZero+1]
			data = append(data, item)
		}
	}

	return data, nil
}

type MapInfo struct {
	Start    uint64
	End      uint64
	Perms    string
	Offset   uint64
	Dev      string
	Inode    string
	Pathname string
}

func extractMaps(line string) (MapInfo, error) {
	re := regexp.MustCompile(`([0-9A-Fa-f]+)-([0-9A-Fa-f]+)\s+([-r][-w][-x][sp])\s+([0-9A-Fa-f]+)\s+([:0-9A-Fa-f]+)\s+([0-9A-Fa-f]+)\s+(.*)$`)
	matches := re.FindStringSubmatch(line)

	if len(matches) != 8 {
		return MapInfo{}, fmt.Errorf("read maps failed: %s", line)
	}

	start, err := strconv.ParseUint(matches[1], 16, 64)
	if err != nil {
		return MapInfo{}, fmt.Errorf("read start failed: %v", err)
	}

	end, err := strconv.ParseUint(matches[2], 16, 64)
	if err != nil {
		return MapInfo{}, fmt.Errorf("read end failed: %v", err)
	}

	perms := matches[3]

	offset, err := strconv.ParseUint(matches[4], 16, 64)
	if err != nil {
		return MapInfo{}, fmt.Errorf("read offset failed: %v", err)
	}

	dev := matches[5]
	inode := matches[6]
	pathname := matches[7]

	return MapInfo{
		Start:    start,
		End:      end,
		Perms:    perms,
		Offset:   offset,
		Dev:      dev,
		Inode:    inode,
		Pathname: pathname,
	}, nil
}

func loadMaps(filePath string) ([]MapInfo, error) {
	file, err := os.Open(filePath)
	if err != nil {
		return nil, errors.WithStack(err)
	}
	defer file.Close()

	var maps []MapInfo
	scanner := bufio.NewScanner(file)

	for scanner.Scan() {
		line := scanner.Text()
		info, err := extractMaps(line)
		if err != nil {
			log.Printf("warn: %v", err)
			continue
		}

		// 只保留具有r-xp权限的映射
		if info.Perms == "r-xp" {
			maps = append(maps, info)
		}
	}

	if err := scanner.Err(); err != nil {
		return nil, errors.WithStack(err)
	}

	return maps, nil
}

func Analyze(snapshotPath string, maps []MapInfo, addr2line *Addr2Line) ([]BinData, error) {
	data, err := loadBinFile(snapshotPath)
	if err != nil {
		return nil, err
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
		return nil, err
	}

	return data, nil
}
