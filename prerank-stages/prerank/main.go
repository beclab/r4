package main

import (
	"math"
	"os"
	"strconv"
	"strings"
	"time"

	"bytetrade.io/web3os/prerank_stages/api"
	"bytetrade.io/web3os/prerank_stages/common"
	"bytetrade.io/web3os/prerank_stages/config"
	"bytetrade.io/web3os/prerank_stages/model"
	"bytetrade.io/web3os/prerank_stages/protobuf_entity"

	"go.uber.org/zap"
	"google.golang.org/protobuf/proto"
)

var TimeDecayRatio = float64(1.0 / 24)

func adjustPrerank(curResult *model.EntryModel, prerankEntries []*model.EntryModel, maxNum int) []*model.EntryModel {
	index := len(prerankEntries)
	if maxNum <= len(prerankEntries) {
		if prerankEntries[index-1].PrerankPoint >= curResult.PrerankPoint {
			return prerankEntries
		}
		index = index - 1
	} else {
		prerankEntries = append(prerankEntries, &model.EntryModel{})
	}
	for ; index > 0; index-- {
		tmpEntry := prerankEntries[index-1]
		if tmpEntry.PrerankPoint >= curResult.PrerankPoint {
			break
		}
		prerankEntries[index] = tmpEntry
	}

	prerankEntries[index] = curResult
	return prerankEntries
}

func entryPrerankCal(language string, maxNum int, startTimestamp int64, userEmbedding []float32) []*model.EntryModel {
	recallDirectory := common.RecallFilePath(common.GetAlgorithmSource(), language)
	data, err := os.ReadFile(recallDirectory)
	if err != nil {
		common.Logger.Error("load recall result  file error step 1", zap.String("language", language), zap.Error(err))
		return nil
	}

	var ptotoList protobuf_entity.ListEntry
	proto.Unmarshal(data, &ptotoList)

	prerankEntries := make([]*model.EntryModel, 0)
	for _, curEntry := range ptotoList.Entries {
		if common.GetSupportTimeliness() != 0 {
			if time.Now().UTC().Unix()-curEntry.CreatedAt > int64(common.GetSupportTimeliness()) {
				continue
			}
		}

		point, coineErr := common.Cosine(curEntry.EmbeddingContentAll_MiniLM_L6V2Base, userEmbedding)
		if coineErr != nil {
			common.Logger.Error("coine cal Err", zap.Error(coineErr))
			continue
		}
		entryAddModel := model.GetEntryModel(curEntry)

		interval := float64((startTimestamp - curEntry.PublishedAtTimestamp) / 60 / 60)
		entryAddModel.PrerankPoint = float32(point) * float32(math.Exp(-TimeDecayRatio*interval))
		prerankEntries = adjustPrerank(entryAddModel, prerankEntries, maxNum)
	}

	return prerankEntries

}

func entriesSaveToMongo(currentList []*model.EntryModel) {

	existEntriesMap := api.LoadEntriesInMongo()
	common.Logger.Info("load entry in mongo..", zap.Int("len", len(existEntriesMap)))
	common.Logger.Info("prerank save ..", zap.Int("len", len(currentList)))
	addCnt := 1
	addList := make([]*model.EntryModel, 0)
	for _, currentEntry := range currentList {
		_, ok := existEntriesMap[currentEntry.Url]
		if !ok {
			addCnt = addCnt + 1
			addList = append(addList, currentEntry)
			if len(addList) >= 100 {
				api.AddEntriesInMongo(addList)
				addList = make([]*model.EntryModel, 0)
				time.Sleep(time.Second * 5)
			}
		} else {
			delete(existEntriesMap, currentEntry.Url)
		}
	}
	api.AddEntriesInMongo(addList)
	delList := make([]string, 0)
	for k := range existEntriesMap {
		delList = append(delList, k)
		if len(delList) >= 100 {
			api.DelEntriesInMongo(delList)
			delList = make([]string, 0)
		}
	}
	api.DelEntriesInMongo(delList)
	common.Logger.Info("add entry to mongo ...", zap.Int("len", addCnt))

}

func checkDuplicateEntries(entries []*model.EntryModel) []*model.EntryModel {
	result := make([]*model.EntryModel, 0)
	existEntryMap := make(map[string]int, 0)
	for _, item := range entries {
		if _, ok := existEntryMap[item.Title]; !ok {
			existEntryMap[item.Title] = 1
			result = append(result, item)
		}
	}
	return result
}

func main() {
	common.Logger.Info("prerank  start...1")
	source := common.GetAlgorithmSource()

	lastRecallTimeStr, _ := api.GetRedisConfig(source, "last_recall_time").(string)
	lastRecallTime, _ := strconv.ParseInt(lastRecallTimeStr, 10, 64)
	lastPrerankTimeStr, _ := api.GetRedisConfig(source, "last_prerank_time").(string)
	lastPrerankTime, _ := strconv.ParseInt(lastPrerankTimeStr, 10, 64)
	if lastRecallTime < lastPrerankTime || lastRecallTimeStr == "" {
		common.Logger.Info("prerank is end no execute", zap.String("source:", source), zap.String("last recall time:", lastRecallTimeStr), zap.String("last prerank time:", lastPrerankTimeStr))
		return
	}

	startTimestamp := int64(time.Now().UTC().Unix())
	config := config.GetAlgorithmConfig()
	common.Logger.Info("prerank  start ", zap.Any("embedding:", config.Embedding), zap.Int("recall item:", config.PrerankItemNum))
	supportLanguages := strings.Split(common.GetSupportLanguage(), ",")
	var prerankEntries []*model.EntryModel
	for _, language := range supportLanguages {
		maxNum := int(config.PrerankItemNum / len(supportLanguages))
		prelanguageRankEntries := entryPrerankCal(language, maxNum, startTimestamp, config.Embedding)
		prerankEntries = append(prerankEntries, prelanguageRankEntries...)
		common.Logger.Info("prerank to save data", zap.String("language:", language), zap.Int("len", len(prelanguageRankEntries)))
	}
	prerankEntries = checkDuplicateEntries(prerankEntries)
	entriesSaveToMongo(prerankEntries)
	api.SetRedisConfig(source, "last_prerank_time", startTimestamp)
	common.Logger.Info("prerank  end")
}
