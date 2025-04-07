package main

import (
	"encoding/binary"
	"os"
	"strings"

	"github.com/google/uuid"
	"github.com/pkg/errors"
)

type Point struct {
	X float64
	Y float64
}

type MemInfoSeries struct {
	Count  uint32
	Points []Point
}

type StringMap struct {
	HashKey uint32
	Value   string
}

type StacktraceModel struct {
	UUID        string
	Seq         uint32
	Time        int32 // 单位毫秒
	Size        int32
	Addr        uint64
	FuncAddr    uint64 // 调用堆栈中的第一个函数地址
	LibraryHash uint32 // 对应字符串表中的hash值
}

type StackItem struct {
	HashCode uint32 // 指向HashValue中hash对应的字符串
	Address  uint64 // 函数地址
}

type CallStack struct {
	UUID   string
	Count  uint32
	Stacks []StackItem
}

// SymbolPair 表示符号对
type SymbolPair struct {
	Key   uint64
	Value string
}

// SymbolMap 表示符号映射
type SymbolMap struct {
	Str   string
	Count uint32
	Pairs []SymbolPair
}

// FreeAddr 表示释放的地址
type FreeAddr struct {
	Addr uint64
	Seq  uint32
}

// ScreenShot 表示屏幕截图
type ScreenShot struct {
	Time   int32
	Buffer []byte
}

// SMapSection 表示内存映射节
type SMapSection struct {
	Start  uint64
	End    uint64
	Offset uint64
}

// SMap 表示内存映射
type SMap struct {
	Name         string
	SectionCount uint32
	Sections     []SMapSection
	Virtual      uint32
	RSS          uint32
	PSS          uint32
	PrivateClean uint32
	PrivateDirty uint32
	SharedClean  uint32
	SharedDirty  uint32
}

// Loli 表示主要的分析文件结构
type Loli struct {
	Magic           uint32
	Version         uint32
	MaxMemInfoValue int32
	MemInfoSeries   []MemInfoSeries
	StringMaps      []StringMap
	StackRecords    []StacktraceModel
	CallStacks      []CallStack
	SymbolMaps      []SymbolMap
	FreeAddrs       []FreeAddr
	ScreenShots     []ScreenShot
	SMaps           []SMap
}

func getStackKey(stack []uint64) string {
	var sb strings.Builder
	byteSlice := make([]byte, 8)
	for _, address := range stack {
		binary.LittleEndian.PutUint64(byteSlice, address)
		sb.Write(byteSlice)
	}
	return sb.String()
}

func (l *Loli) buildStacktrace(data []BinData) {
	callStacks := make(map[string]CallStack, len(data))

	for i := range data {
		cur := &data[i]
		key := getStackKey(cur.Stack)
		cs, exist := callStacks[key]
		if !exist {
			cs.UUID = uuid.New().String()
			cs.Count = uint32(len(cur.Stack))
			cs.Stacks = make([]StackItem, 0, cs.Count)
			for _, address := range cur.Stack {
				cs.Stacks = append(cs.Stacks, StackItem{Address: address})
			}
			callStacks[key] = cs
		}
		cur.UserData = cs.UUID
	}

	l.CallStacks = make([]CallStack, 0, len(callStacks))
	for _, v := range callStacks {
		l.CallStacks = append(l.CallStacks, v)
	}

	l.StackRecords = make([]StacktraceModel, 0, len(data))
	for i := range data {
		cur := &data[i]
		l.StackRecords = append(l.StackRecords, StacktraceModel{
			UUID:     cur.UserData.(string),
			Seq:      uint32(i),
			Time:     int32(cur.Time * 1000),
			Size:     int32(cur.Size),
			Addr:     uint64(cur.Ptr),
			FuncAddr: cur.Stack[0],
		})
	}

	return
}

func (l *Loli) Build(data []BinData, addr2line *Addr2Line) {
	l.Magic = 0xA4B3C2D1
	l.Version = 106

	l.buildStacktrace(data)
}

func LoliFormatOutput(filePath string, data []BinData, addr2line *Addr2Line) error {
	var loli Loli
	loli.Build(data, addr2line)

	file, err := os.Create(filePath)
	if err != nil {
		return errors.WithStack(err)
	}
	defer file.Close()

	binary.Write(file, binary.BigEndian, loli.Magic)
	binary.Write(file, binary.BigEndian, loli.Version)

	binary.Write(file, binary.BigEndian, int(2)) // maxMemInfoValue_
	binary.Write(file, binary.BigEndian, int(0)) // memInfoSeries_

	// err = binary.Write(file, binary.LittleEndian, intValue)
	return nil
}
