package main

import (
	"bufio"
	"fmt"
	"os"

	"github.com/pkg/errors"
)

func TextFormatOutput(filePath string, data []BinData, addr2line *Addr2Line) error {
	file, err := os.Create(filePath)
	if err != nil {
		return errors.WithStack(err)
	}
	defer file.Close()

	for i := range data {
		var stackLine []string
		for _, address := range data[i].Stack {
			line := addr2line.GetFromCache(address)
			if line == "" {
				line = fmt.Sprintf("0x%x", address)
			}
			stackLine = append(stackLine, line)
		}
		data[i].UserData = stackLine
	}

	writer := bufio.NewWriter(file)
	for _, item := range data {
		_, err := fmt.Fprintf(writer, "time:%d\tsize:%d\tptr:0x%x\n", item.Time, item.Size, item.Ptr)
		if err != nil {
			return errors.WithStack(err)
		}

		for _, frame := range item.UserData.([]string) {
			_, err := fmt.Fprintf(writer, "%s\n", frame)
			if err != nil {
				return errors.WithStack(err)
			}
		}

		_, err = fmt.Fprintf(writer, "========\n\n")
		if err != nil {
			return errors.WithStack(err)
		}
	}

	return writer.Flush()
}
