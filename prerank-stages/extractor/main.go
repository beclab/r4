package main

import (
	"strconv"
	"strings"
	"time"

	"bytetrade.io/web3os/prerank_stages/common"
	"bytetrade.io/web3os/prerank_stages/model"
	"github.com/beclab/article-extractor/processor"

	"bytetrade.io/web3os/prerank_stages/api"

	"go.uber.org/zap"
)

func countENNonEmpty(arr []string) int {
	var count int

	for _, value := range arr {
		if len(value) > 0 {
			count++
		}
	}

	return count
}

func main() {
	common.Logger.Info("extractor  start...")
	source := common.GetAlgorithmSource()
	startTimestamp := int64(time.Now().UTC().Unix())
	lastCrawlerTimeStr, _ := api.GetRedisConfig(common.GetBflUser(), source, "last_crawler_time").(string)
	lastCrawlerTime, _ := strconv.ParseInt(lastCrawlerTimeStr, 10, 64)
	lastExtractorTimeStr, _ := api.GetRedisConfig(common.GetBflUser(), source, "last_extractor_time").(string)
	lastExtractorTime, _ := strconv.ParseInt(lastExtractorTimeStr, 10, 64)
	common.Logger.Info("extractor start", zap.String("source:", source), zap.Int64("last crawler time:", lastCrawlerTime), zap.Int64("last extractor time:", lastExtractorTime))
	if lastCrawlerTime < lastExtractorTime || lastCrawlerTimeStr == "" {
		common.Logger.Info("extractor is end no execute", zap.String("source:", source), zap.String("last crawler time:", lastCrawlerTimeStr), zap.String("last extractor time:", lastExtractorTimeStr))
		return
	}
	limit := 50
	extractData := api.GetUnextractedData(limit)
	sum := extractData.Count
	if sum > 0 {
		for i := 0; i*limit <= sum; i++ {
			addList := make([]*model.EntryAddModel, 0)
			delList := make([]string, 0)
			for _, rank := range extractData.Items {
				if rank.RawContent == "" {
					continue
				}
				//fullContent, pureContent, _, _, _, _, _, _ := processor.ArticleReadabilityExtractor(rank.RawContent, rank.Url, "", "", true)
				fullContent, pureContent := processor.ArticleContentExtractor(rank.RawContent, rank.Url, "", "")
				var contentLen int
				if rank.Language != "zh-cn" {
					contentArr := strings.Split(pureContent, " ")
					contentLen = countENNonEmpty(contentArr)
				} else {
					usedText := strings.Replace(pureContent, " ", "", -1)
					contentLen = len(usedText)
				}

				if fullContent != "" && contentLen > 200 {
					var addEntry model.EntryAddModel
					addEntry.Url = rank.Url
					addEntry.Source = rank.Source
					addEntry.FullContent = fullContent
					addEntry.Crawler = true
					addEntry.Extract = true

					addList = append(addList, &addEntry)
				} else {
					common.Logger.Info("extract pure less than 100 ", zap.String("language", rank.Language), zap.String("url", rank.Url), zap.Int("content len:", contentLen))
					delList = append(delList, rank.Url)
				}
			}
			api.UpdateEntriesInMongo(addList)
			api.DelEntriesInMongo(delList)
			if (i+1)*limit <= sum {
				common.Logger.Info("get unextract data ", zap.Int("page", i))
				extractData = api.GetUnextractedData(limit)
			}
			//time.Sleep(time.Second * 1)
		}
	}
	api.SetRedisConfig(source, "last_extractor_time", startTimestamp)
	common.Logger.Info("extractor  end")
}
