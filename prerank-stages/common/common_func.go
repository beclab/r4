package common

import (
	"fmt"
	"os"
	"strconv"
	"strings"
	"time"
	"unicode/utf8"

	"go.uber.org/zap"
	"golang.org/x/text/cases"
	"golang.org/x/text/language"
)

func CreateNotExistDirectory(currentDirectory string, varName string) {
	if _, currentDirectoryExistErr := os.Stat(currentDirectory); os.IsNotExist(currentDirectoryExistErr) {
		if currentDirectoryCreateErr := os.MkdirAll(currentDirectory, os.ModePerm); currentDirectoryCreateErr != nil {
			Logger.Error("create fail directory "+varName, zap.String(varName, currentDirectory), zap.Error(currentDirectoryCreateErr))

		}
	}
}

func GetSpecificDayOneDayStart(currentTime time.Time) time.Time {
	currentTimeUtc := currentTime.UTC()
	targetStartDay := time.Date(currentTimeUtc.Year(), currentTimeUtc.Month(), currentTimeUtc.Day(), 0, 0, 0, 0, time.UTC)
	return targetStartDay
}

func FloatArrayToString(arr []float32) string {
	res := ""
	for i := 0; i < len(arr); i++ {
		res += fmt.Sprint(arr[i])
		if i != len(arr)-1 {
			res += ";"
		}
	}
	return res
}

func StringToCamelCase(s string) string {
	s = strings.ReplaceAll(s, "_", " ")
	s = cases.Title(language.English).String(s)
	return strings.ReplaceAll(s, " ", "")
}

func StringToFloatArray(str string) []float32 {
	subStrings := strings.Split(str, ";")
	result := make([]float32, 0)
	for i := 0; i < len(subStrings)-1; i++ {
		value, err := strconv.ParseFloat(subStrings[i], 32)
		if err != nil {
			fmt.Println("StringToFloatArray err :", str)
			return nil
		}
		result = append(result, float32(value))
	}
	return result
}

func GetUTF8ValidString(str string) string {
	if utf8.ValidString(str) {
		return str
	}

	v := make([]rune, 0, len(str))
	for i, r := range str {
		if r == utf8.RuneError {
			_, size := utf8.DecodeRuneInString(str[i:])
			if size == 1 {
				continue
			}
		}
		v = append(v, r)
	}
	return string(v)
}

func IsInStringArray(arr []string, target string) bool {
	for _, num := range arr {
		if num == target {
			return true
		}
	}
	return false
}
