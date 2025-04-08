package api

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"log"
	"net/http"
	"time"

	"bytetrade.io/web3os/prerank_stages/common"
	"bytetrade.io/web3os/prerank_stages/model"

	"go.uber.org/zap"
)

func LoadEntriesInMongo() (map[string]int, error) {
	existEntryList := make(map[string]int, 0)
	url := common.EntryAlgorithmMonogoApiUrl() + common.GetAlgorithmSource()
	request, _ := http.NewRequest("GET", url, nil)
	request.Header.Set("X-Bfl-User", common.GetBflUser())
	client := &http.Client{Timeout: time.Second * 30}
	//res, err := client.Get(url)
	res, err := client.Do(request)
	if err != nil {
		common.Logger.Error("get entry data  fail", zap.Error(err))
		return existEntryList, err
	}
	if res.StatusCode != 200 {
		common.Logger.Error("get entry data fail code")
	}
	defer res.Body.Close()
	body, _ := io.ReadAll(res.Body)

	var resObj model.KnowledgeApiResponseModel
	if err := json.Unmarshal(body, &resObj); err != nil {
		common.Logger.Error("json decode failed, err", zap.Error(err))
		return existEntryList, err
	}
	for _, jsonObj := range resObj.Data {
		existEntryList[jsonObj] = 1
	}
	return existEntryList, nil

}

func AddEntriesInMongo(list []*model.EntryModel) {
	if len(list) > 0 {
		addList := make([]*model.EntryAddModel, 0)
		for _, entryModel := range list {
			reqModel := model.GetEntryAddModel(entryModel)
			addList = append(addList, reqModel)
		}
		jsonByte, _ := json.Marshal(addList)
		url := common.EntryMonogoEntryApiUrl()
		request, _ := http.NewRequest("POST", url, bytes.NewBuffer(jsonByte))
		request.Header.Set("Content-Type", "application/json")
		request.Header.Set("X-Bfl-User", common.GetBflUser())
		client := &http.Client{Timeout: time.Second * 10}
		response, err := client.Do(request)
		if err != nil {
			common.Logger.Error("add entry in mongo  fail", zap.Error(err))
		}
		defer response.Body.Close()
		responseBody, _ := io.ReadAll(response.Body)
		common.Logger.Info("add entries in mongo finish...")
		var resObj model.MongoEntryApiResponseModel
		if err := json.Unmarshal(responseBody, &resObj); err != nil {
			log.Print("json decode failed, err", err)
			return
		}
		if resObj.Code == 0 {
			resEntryMap := make(map[string]string, 0)
			for _, resDataDetail := range resObj.Data {
				common.Logger.Info("response entry direct", zap.String("url", resDataDetail.Url), zap.String("id", resDataDetail.ID), zap.String("source", resDataDetail.Source))
				resEntryMap[resDataDetail.Url] = resDataDetail.ID
			}
			addAlgorithmList := make([]*model.AlgorithmAddModel, 0)
			for _, entryModel := range list {
				common.Logger.Info("construct add algorithm", zap.String("url", entryModel.Url), zap.String("id", resEntryMap[entryModel.Url]))
				algoModel := model.GetAddAlgorithmModel(resEntryMap[entryModel.Url], entryModel.RecallPoint, entryModel.PrerankPoint, entryModel.Embedding)
				addAlgorithmList = append(addAlgorithmList, algoModel)
			}
			UpdateEntryAlgorith(addAlgorithmList)

		} else {
			common.Logger.Info("add feed in mongo code err", zap.Int("result code", resObj.Code))
		}
		common.Logger.Info("add entries in mongo finish all...")
	}
}

func UpdateEntryAlgorith(addAlgorithmList []*model.AlgorithmAddModel) {
	if len(addAlgorithmList) > 0 {
		for _, currentAlgorithm := range addAlgorithmList {
			common.Logger.Info("current_algorithm", zap.String("source", currentAlgorithm.Source), zap.String("entry_id", currentAlgorithm.Entry))
		}
		algoUrl := common.AlgorithMonogoApiUrl()
		algoJsonByte, err := json.Marshal(addAlgorithmList)
		if err != nil {
			common.Logger.Error("add algorith json marshal  fail", zap.Error(err))
		}

		common.Logger.Info("update algorith ", zap.String("url", algoUrl), zap.Int("content len", len(string(algoJsonByte))), zap.Int("len", len(addAlgorithmList)))

		algoReq, _ := http.NewRequest("POST", algoUrl, bytes.NewBuffer(algoJsonByte))
		algoReq.Header.Set("Content-Type", "application/json")
		algoReq.Header.Set("X-Bfl-User", common.GetBflUser())
		algoClient := &http.Client{Timeout: time.Second * 5}
		_, err = algoClient.Do(algoReq)

		defer algoReq.Body.Close()
		body, _ := io.ReadAll(algoReq.Body)
		jsonStr := string(body)
		common.Logger.Info("update algorith response: ", zap.String("body", jsonStr))

		if err != nil {
			common.Logger.Error("add algorith in mongo  fail", zap.Error(err))
		}

		common.Logger.Info("update algorith finish ", zap.Int("content len", len(string(algoJsonByte))), zap.Int("len", len(addAlgorithmList)))
	}

}
func DelEntriesInMongo(list []string) {
	if len(list) > 0 {
		reqData := model.EntryDelModel{EntryUrls: list}
		jsonByte, _ := json.Marshal(reqData)
		url := common.EntryMonogoEntryApiUrl() + common.GetAlgorithmSource()
		request, _ := http.NewRequest("DELETE", url, bytes.NewBuffer(jsonByte))
		request.Header.Set("Content-Type", "application/json")
		request.Header.Set("X-Bfl-User", common.GetBflUser())
		client := &http.Client{Timeout: time.Second * 30}
		_, err := client.Do(request)

		defer request.Body.Close()
		body, _ := io.ReadAll(request.Body)
		jsonStr := string(body)
		common.Logger.Info("del entries: ", zap.String("body", jsonStr))
		if err != nil {
			common.Logger.Error("del entry in mongo  fail", zap.Error(err))
		}
	}
	common.Logger.Info("del entry in mongo", zap.Int("list size", len(list)))
}

func LoadFeedsInMongo(source string) map[string]int {
	existFeedList := make(map[string]int, 0)
	url := common.FeedMonogoApiUrl() + source

	common.Logger.Info("load feeds in mongo", zap.String("url", url))
	request, _ := http.NewRequest("GET", url, nil)
	request.Header.Set("X-Bfl-User", common.GetBflUser())
	client := &http.Client{Timeout: time.Second * 5}
	//res, err := client.Get(url)
	res, err := client.Do(request)

	if err != nil {
		common.Logger.Error("get entry data  fail", zap.Error(err))
		return existFeedList
	}
	if res.StatusCode != 200 {
		common.Logger.Error("get entry data fail code")
		return existFeedList
	}
	defer res.Body.Close()
	body, _ := io.ReadAll(res.Body)

	var resObj model.KnowledgeApiResponseModel
	if err := json.Unmarshal(body, &resObj); err != nil {
		log.Print("json decode failed, err", err)
		return existFeedList
	}
	for _, jsonObj := range resObj.Data {
		existFeedList[jsonObj] = 1
	}
	return existFeedList
}

func getAllEntries(reqParam string) *model.EntryApiDataResponseModel {
	url := common.EntryMonogoEntryApiUrl() + "?" + reqParam
	request, _ := http.NewRequest("GET", url, nil)
	request.Header.Set("X-Bfl-User", common.GetBflUser())
	client := &http.Client{Timeout: time.Second * 5}
	//res, err := client.Get(url)
	res, err := client.Do(request)

	if err != nil {
		common.Logger.Error("get entry data  fail", zap.Error(err))
		return nil
	}
	if res.StatusCode != 200 {
		common.Logger.Error("get entry data fail code")
		return nil
	}
	defer res.Body.Close()
	body, _ := io.ReadAll(res.Body)

	var resObj model.EntryApiResponseModel
	if err := json.Unmarshal(body, &resObj); err != nil {
		log.Print("json decode failed, err", err)
		return nil
	}
	return &resObj.Data
}

func UpdateEntriesInMongo(addList []*model.EntryAddModel) {

	if len(addList) > 0 {
		jsonByte, _ := json.Marshal(addList)
		url := common.EntryMonogoEntryApiUrl()
		request, newReqErr := http.NewRequest("POST", url, bytes.NewBuffer(jsonByte))
		if newReqErr != nil {
			log.Print("new http request fail url", url, newReqErr)
			return
		}
		request.Header.Set("Content-Type", "application/json")
		request.Header.Set("X-Bfl-User", common.GetBflUser())
		client := &http.Client{Timeout: time.Second * 5}
		response, err := client.Do(request)
		if err != nil {
			log.Print("add entry in mongo  fail", err)
			return
		}
		defer response.Body.Close()
		responseBody, _ := io.ReadAll(response.Body)
		var resObj model.MongoEntryApiResponseModel
		if err := json.Unmarshal(responseBody, &resObj); err != nil {
			log.Print("json decode failed, err", err)
			return
		}
		if resObj.Code != 0 {
			common.Logger.Info("update feed in mongo code err", zap.Int("result code", resObj.Code))
		}
		common.Logger.Info("update entries in mongo finish all...", zap.Int("entry size:", len(addList)))
	}
}

func GetUnextractedData(limit int) *model.EntryApiDataResponseModel {
	param := "limit=" + fmt.Sprintf("%d", limit) + "&crawler=true&extract=false&source=" + common.GetAlgorithmSource()
	return getAllEntries(param)
}

func GetRedisConfig(bflUser, provider, key string) interface{} {
	url := common.RedisConfigApiUrl() + provider + "/" + key
	common.Logger.Info("get redis config", zap.String("url", url))

	request, _ := http.NewRequest("GET", url, nil)
	request.Header.Set("X-Bfl-User", bflUser)
	client := &http.Client{Timeout: time.Second * 5}
	//res, err := client.Get(url)
	res, err := client.Do(request)
	if err != nil {
		common.Logger.Error("get redis config  fail", zap.Error(err))
		return ""
	}
	if res.StatusCode != 200 {
		common.Logger.Error("get redis config fail code")
		return ""
	}
	defer res.Body.Close()
	body, _ := io.ReadAll(res.Body)

	var resObj model.RedisConfigResponseModel
	if err := json.Unmarshal(body, &resObj); err != nil {
		log.Print("json decode failed, err", err)
		return ""
	}

	return resObj.Data
}

func SetRedisConfig(provider, key string, val interface{}) {
	var c model.RedisConfig
	c.Value = val
	url := common.RedisConfigApiUrl() + provider + "/" + key
	common.Logger.Info("set redis config", zap.String("url", url))

	jsonByte, err := json.Marshal(c)
	if err != nil {
		common.Logger.Error("set redis configjson marshal  fail", zap.Error(err))
	}

	common.Logger.Info("set redis config  ", zap.String("url", url), zap.String("key", key), zap.Any("val", val))

	algoReq, _ := http.NewRequest("POST", url, bytes.NewBuffer(jsonByte))
	algoReq.Header.Set("Content-Type", "application/json")
	algoReq.Header.Set("X-Bfl-User", common.GetBflUser())
	algoClient := &http.Client{Timeout: time.Second * 5}
	_, err = algoClient.Do(algoReq)
	if err != nil {
		common.Logger.Error("set redis configjson req  fail", zap.Error(err))
	}

	defer algoReq.Body.Close()
	body, _ := io.ReadAll(algoReq.Body)
	jsonStr := string(body)
	common.Logger.Info("update redis config response: ", zap.String("body", jsonStr))

}
