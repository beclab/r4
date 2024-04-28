package config

import (
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
	"strconv"
	"time"

	"bytetrade.io/web3os/prerank_stages/api"
	"bytetrade.io/web3os/prerank_stages/common"
	"bytetrade.io/web3os/prerank_stages/model"

	"go.uber.org/zap"
)

func GetAlgorithmConfig() *model.AlgorithmConfig {
	directory := os.Getenv("ALGORITHM_FILE_CONFIG_PATH")
	if directory == "" {
		directory = "../config"
	}
	fileName := filepath.Join(directory, "algorithm.config")
	file, err := os.Open(fileName)
	if err != nil {
		common.Logger.Error("error file faile directory", zap.Error(err))
	}
	defer file.Close()

	decoder := json.NewDecoder(file)
	var curOption model.AlgorithmConfig
	err = decoder.Decode(&curOption)
	if err != nil {
		common.Logger.Error("file decode error", zap.String("fileName", fileName), zap.Error(err))
	}

	source := common.GetAlgorithmSource()
	embeddingStr := common.FloatArrayToString(curOption.Embedding) + ";" + fmt.Sprintf("%d", time.Now().UTC().Unix())

	recallItemConfig := api.GetRedisConfig(source, "recall_item_num")
	if recallItemConfig == nil || recallItemConfig == "" {
		api.SetRedisConfig(source, "recall_item_num", curOption.RecallItemNum)
	} else {
		recallItemsNumStr, _ := recallItemConfig.(string)
		recallItemsNumber, err := strconv.Atoi(recallItemsNumStr)
		if err != nil {
			common.Logger.Error("get recall items number err ", zap.String("recallItemsNumStr:", recallItemsNumStr))
		}
		curOption.RecallItemNum = recallItemsNumber
	}
	prerankItemConfig := api.GetRedisConfig(source, "prerank_item_num")
	if prerankItemConfig == nil || prerankItemConfig == "" {
		api.SetRedisConfig(source, "prerank_item_num", curOption.PrerankItemNum)
	} else {
		prerankItemsNumStr, _ := prerankItemConfig.(string)
		prerankItemsNumber, err := strconv.Atoi(prerankItemsNumStr)
		if err != nil {
			common.Logger.Error("get prerank items number err ", zap.String("prerankItemsNumStr:", prerankItemsNumStr))
		}
		curOption.PrerankItemNum = prerankItemsNumber
	}

	userEmbeddingConfig := api.GetRedisConfig(source, "user_embedding")
	if userEmbeddingConfig == nil || userEmbeddingConfig == "" {
		api.SetRedisConfig(source, "user_embedding", embeddingStr)
	} else {
		embeddingStr, _ := userEmbeddingConfig.(string)
		curOption.Embedding = common.StringToFloatArray(embeddingStr)
	}
	return &curOption

}
