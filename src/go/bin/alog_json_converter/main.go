package main

import (
	"bufio"
	"flag"
	"fmt"
	"github.com/IBM/alchemy-logging/src/go/alog"
	"os"
)

func main() {

	// Flag to indicate input source (default to stdin)
	inputFile := flag.String(
		"input-file",
		"",
		"Input file with JSON log data. If none set, read from stdin.",
	)

	// Flag to indcate output file (default to stdout)
	outputFile := flag.String(
		"output-file",
		"",
		"Output file to write log lines to. If none set, write to stdout.",
	)

	flag.Parse()

	// Set up input reader
	reader := os.Stdin
	if nil != inputFile && len(*inputFile) > 0 {
		if fReader, err := os.Open(*inputFile); nil != err {
			fmt.Printf("Error opening input file: %v\n", err)
			os.Exit(1)
		} else {
			reader = fReader
		}
	}
	bufReader := bufio.NewReader(reader)

	// Set up the output writer
	writer := os.Stdout
	if nil != outputFile && len(*outputFile) > 0 {
		if fout, err := os.Create(*outputFile); nil != err {
			fmt.Printf("Error opening output file: %v\n", err)
			os.Exit(1)
		} else {
			writer = fout
		}
	}
	bufWriter := bufio.NewWriter(writer)

	// Read each line from input and write to output
	for {
		if line, err := bufReader.ReadString('\n'); nil != err {
			os.Exit(0)
		} else {
			if outlines, err := alog.JSONToPlainText(line); nil != err {
				fmt.Printf("Error converting line [%s]\n", line)
				fmt.Printf("%v\n", err)
			} else {
				for _, outline := range outlines {
					bufWriter.WriteString(outline)
					bufWriter.Flush()
				}
			}
		}
	}
}
