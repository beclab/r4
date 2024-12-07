package main

import (
	"bufio"
	"math"
	"os"
	"path/filepath"
	"sort"
	"strconv"
	"strings"
	"time"

	"bytetrade.io/web3os/prerank_stages/api"
	"bytetrade.io/web3os/prerank_stages/common"
	"bytetrade.io/web3os/prerank_stages/config"
	"bytetrade.io/web3os/prerank_stages/protobuf_entity"

	"go.uber.org/zap"
	"google.golang.org/protobuf/proto"
)

var TimeDecayRatio = float64(1.0 / 24)

func loadRecallResult(language string, startTimeStamp int64, feedMap map[string]int) ([]*protobuf_entity.Entry, map[string]int, error) {
	existRankEntryMap := make(map[string]int, 0)
	rankEntries := make([]*protobuf_entity.Entry, 0)
	recallFilePath := common.RecallFilePath(common.GetAlgorithmSource(), language)

	if !common.IsFileExist(recallFilePath) {
		return rankEntries, existRankEntryMap, nil
	}
	currentZlibFileByte, err := os.ReadFile(recallFilePath)

	if err != nil || currentZlibFileByte == nil {
		common.Logger.Error("load recall result  file error ", zap.Error(err))
		return rankEntries, existRankEntryMap, err
	}
	var ptotoList protobuf_entity.ListEntry
	proto.Unmarshal(currentZlibFileByte, &ptotoList)

	for _, entry := range ptotoList.Entries {
		_, feedOK := feedMap[entry.FeedUrl]
		if !feedOK {
			common.Logger.Error("entry feed not exist", zap.String("entry url", entry.Url), zap.String("feed url:", entry.FeedUrl))
			continue
		}
		if common.GetSupportTimeliness() != 0 {
			if time.Now().UTC().Unix()-entry.CreatedAt > int64(common.GetSupportTimeliness()) {
				continue
			}
		}
		interval := float64(0)
		if startTimeStamp > entry.CreatedAt {
			interval = float64((startTimeStamp - entry.PublishedAtTimestamp) / 60 / 60)
		}

		entry.RecallPoint = entry.RecallPoint * float32(math.Exp(-TimeDecayRatio*interval))
		existRankEntryMap[entry.Url] = 1
		rankEntries = append(rankEntries, entry)
	}

	common.Logger.Info("load saved recall data ", zap.Int("file size:", len(ptotoList.Entries)), zap.Int("check size:", len(rankEntries)))

	sort.SliceStable(rankEntries, func(i, j int) bool {
		return rankEntries[i].RecallPoint < rankEntries[j].RecallPoint
	})
	return rankEntries, existRankEntryMap, nil

}

func saveRecallResult(language string, entryList []*protobuf_entity.Entry) error {
	if len(entryList) == 0 {
		return nil
	}

	var recallProtobuf protobuf_entity.ListEntry
	recallProtobuf.Entries = append(recallProtobuf.Entries, entryList...)

	currentProtoByte, marshalErr := proto.Marshal(&recallProtobuf)
	if marshalErr != nil {
		common.Logger.Error("marshalErr url", zap.Error(marshalErr))
		return marshalErr
	}
	common.CreateNotExistDirectory(common.RecallDirectory(common.GetAlgorithmSource()), "recall directory")
	currentRecallFilePath := common.RecallFilePath(common.GetAlgorithmSource(), language)

	tempFile, createTempFileErr := os.Create(currentRecallFilePath)
	if createTempFileErr != nil {
		common.Logger.Error("create temp file err", zap.String("currentFeedFilePath", currentRecallFilePath), zap.Error(createTempFileErr))
		return createTempFileErr
	}
	writer := bufio.NewWriter(tempFile)
	_, writeErr := writer.Write(currentProtoByte)
	if writeErr != nil {
		common.Logger.Error("write file error", zap.Error(writeErr))
		return writeErr
	}
	syncErr := writer.Flush()
	if syncErr != nil {
		common.Logger.Error("sync file error", zap.Error(syncErr))
		return syncErr
	}
	return nil
}

func adjustRecallResult(maxNum int, curEntry *protobuf_entity.Entry, rankEntries []*protobuf_entity.Entry) []*protobuf_entity.Entry {
	index := len(rankEntries)
	if maxNum <= len(rankEntries) {
		if rankEntries[index-1].RecallPoint >= curEntry.RecallPoint {
			return rankEntries
		}
		index = index - 1
	} else {
		rankEntries = append(rankEntries, &protobuf_entity.Entry{})
	}

	for ; index > 0; index-- {
		tmpEntry := rankEntries[index-1]
		if tmpEntry.RecallPoint >= curEntry.RecallPoint {
			break
		}
		rankEntries[index] = tmpEntry
	}
	rankEntries[index] = curEntry
	return rankEntries

}
func entryRecallCal(entryPath string, language string, maxNum int, LastRecallTime int64, userEmbedding []float32, rankEntries []*protobuf_entity.Entry, rankEntriesMap map[string]int, feedMap map[string]int) ([]*protobuf_entity.Entry, int64) {
	files, err := os.ReadDir(entryPath)
	if err != nil {
		common.Logger.Error("load entry  file error step 1", zap.Error(err))
		return rankEntries, 0
	}
	var maxCreatedAt int64
	for _, file := range files {
		fileName := file.Name()
		lastInd := strings.Index(fileName, ".")
		fileNameInt, _ := strconv.ParseInt(fileName[:lastInd], 10, 64)
		if fileNameInt < LastRecallTime-3600 {
			continue
		}

		filePath := filepath.Join(entryPath, fileName)
		currentZlibFileByte, _ := os.ReadFile(filePath)
		//uncompressByte := common.DoZlibUnCompress(currentZlibFileByte)
		var protoEntryList protobuf_entity.ListEntry
		proto.Unmarshal(currentZlibFileByte, &protoEntryList)

		totalEntryInFile := 0
		totalEntryUseToCalculateInFile := 0
		for _, protoEntry := range protoEntryList.Entries {
			if protoEntry.Language != language {
				continue
			}
			_, feedOK := feedMap[protoEntry.FeedUrl]
			if !feedOK {
				common.Logger.Error("entry feed not exist", zap.String("entry url", protoEntry.Url), zap.String("feed url:", protoEntry.FeedUrl))
				continue
			}
			_, ok := rankEntriesMap[protoEntry.Url]
			if ok {
				continue
			}
			if common.GetSupportTimeliness() != 0 {
				if time.Now().UTC().Unix()-protoEntry.CreatedAt > int64(common.GetSupportTimeliness()) {
					continue
				}
			}
			if protoEntry.CreatedAt > LastRecallTime {
				if maxCreatedAt < protoEntry.CreatedAt {
					maxCreatedAt = protoEntry.CreatedAt
				}
				point, coineErr := common.Cosine(protoEntry.Embedding, userEmbedding)
				if coineErr != nil {
					common.Logger.Error("coine cal Err", zap.String("file", filepath.Join(entryPath, file.Name())), zap.Error(coineErr))
					continue
				}
				protoEntry.RecallPoint = float32(point)
				rankEntries = adjustRecallResult(maxNum, protoEntry, rankEntries)
				totalEntryUseToCalculateInFile++
			}
			rankEntriesMap[protoEntry.Url] = 1
			totalEntryInFile++
		}
		common.Logger.Info("recall calculate package entry in file", zap.String("file name:", fileName), zap.Int("cal entry num:", totalEntryUseToCalculateInFile), zap.Int("total entry num:", totalEntryInFile))
	}
	return rankEntries, maxCreatedAt

}

func main() {
	common.Logger.Info("recall  start", zap.Int("timelines", common.GetSupportTimeliness()), zap.Int64("check start time", time.Now().UTC().Unix()-int64(common.GetSupportTimeliness())))

	source := common.GetAlgorithmSource()
	startTimestamp := int64(time.Now().UTC().Unix())

	syncKey := common.GetSyncProvider() + common.GetSyncFeedName() + "_" + common.GetSyncModelName()
	lastSyncTimeStr, _ := api.GetRedisConfig(syncKey, "last_sync_time").(string)
	if lastSyncTimeStr == "" {
		common.Logger.Info("sync data is not end no execute", zap.String("syncKey:", syncKey))
		return
	}

	lastRecallTimeStr, _ := api.GetRedisConfig(source, "last_recall_time").(string)
	lastRecallTime, _ := strconv.ParseInt(lastRecallTimeStr, 10, 64)
	lastRecallExecTimeStr, _ := api.GetRedisConfig(source, "last_recall_exec_time").(string)
	lastRecallExecTime, _ := strconv.ParseInt(lastRecallExecTimeStr, 10, 64)
	if lastRecallExecTimeStr != "" && startTimestamp < lastRecallExecTime+60*60 {
		common.Logger.Info("recall is end no execute", zap.String("source:", source), zap.String("last recall exec time:", lastRecallExecTimeStr), zap.Int64("now time:", startTimestamp))
		return
	}
	config := config.GetAlgorithmConfig()
	recallEnvVersion := os.Getenv("RECALL_VERSION")
	recallVersion, _ := api.GetRedisConfig(source, "RECALL_VERSION").(string)

	common.Logger.Info("recall  start", zap.String("sync finish time:", lastSyncTimeStr), zap.Int64("start time:", lastRecallTime), zap.Int("recall item:", config.RecallItemNum))

	feedMap := api.LoadFeedsInMongo(source)

	var maxCreatedAt int64
	allMaxCreatedAt := lastRecallTime
	supportLanguages := strings.Split(common.GetSupportLanguage(), ",")
	for _, language := range supportLanguages {
		syncPath := filepath.Join(common.SyncEntryDirectory(common.GetSyncProvider(), common.GetSyncFeedName(), common.GetSyncModelName()))
		files, err := os.ReadDir(syncPath)
		if err != nil {
			common.Logger.Error("load sync data error ", zap.String("sync path:", syncPath), zap.Error(err))
			return
		}
		checkTimestamp := lastRecallTime - 24*60*60
		if checkTimestamp < 0 {
			checkTimestamp = 0
		}
		var loadResultErr error
		existSaveMap := make(map[string]int, 0)
		recallSaveResult := make([]*protobuf_entity.Entry, 0)

		if recallEnvVersion == "" || recallVersion == "" || recallEnvVersion == recallVersion {
			recallSaveResult, existSaveMap, loadResultErr = loadRecallResult(language, lastRecallTime, feedMap)
			if loadResultErr != nil {
				common.Logger.Error("load recall data error ", zap.Error(err))
				return
			}
			common.Logger.Info("load recall saved data", zap.Int("items", len(recallSaveResult)))
		} else {
			lastRecallTime = 0
			checkTimestamp = 0
		}

		maxNum := int(config.RecallItemNum / len(supportLanguages))
		if maxNum > 0 {
			for _, file := range files {
				fileName := file.Name()
				fileNameInt, err := strconv.ParseInt(file.Name(), 10, 64)
				if err != nil {
					common.Logger.Error("file name error not timestamp", zap.String("file name", fileName))
				}
				if fileNameInt >= checkTimestamp {
					common.Logger.Info("recall calculate package entry in fold", zap.String("fold name:", fileName))
					entrysSavePath := filepath.Join(syncPath, fileName)
					recallSaveResult, maxCreatedAt = entryRecallCal(entrysSavePath, language, maxNum, lastRecallTime, config.Embedding, recallSaveResult, existSaveMap, feedMap)
					if allMaxCreatedAt < maxCreatedAt {
						allMaxCreatedAt = maxCreatedAt
					}
				}
			}
		}
		common.Logger.Info("recall result ", zap.String("language:", language), zap.Int("timeliness", common.GetSupportTimeliness()), zap.Int("len", len(recallSaveResult)))
		saveErr := saveRecallResult(language, recallSaveResult)
		if saveErr != nil {
			return
		}
	}
	if recallEnvVersion != "" && recallVersion != recallEnvVersion {
		api.SetRedisConfig(source, "RECALL_VERSION", recallEnvVersion)
	}
	api.SetRedisConfig(source, "last_recall_time", allMaxCreatedAt)
	api.SetRedisConfig(source, "last_recall_exec_time", startTimestamp)
	common.Logger.Info("recall  end", zap.Int64("end time:", allMaxCreatedAt))
}
