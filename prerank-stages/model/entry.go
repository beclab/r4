package model

import (
	"encoding/json"

	"bytetrade.io/web3os/prerank_stages/common"
	"bytetrade.io/web3os/prerank_stages/protobuf_entity"

	"go.uber.org/zap"
)

type EntryModel struct {
	Url                  string
	Title                string
	FeedUrl              string
	CreatedAt            int64
	PublishedAt          int64
	Author               string
	KeywordList          []string
	Language             string
	ImageUrl             string
	Embedding            []float32
	RecallPoint          float32
	PrerankPoint         float32
	PublishedAtTimestamp int64
}

type EntryAddModel struct {
	Source      string   `json:"source,omitempty"`
	Url         string   `json:"url,omitempty"`
	Title       string   `json:"title,omitempty"`
	FeedUrl     string   `json:"feed_url,omitempty"`
	PublishedAt int64    `json:"published_at,omitempty"`
	Author      string   `json:"author,omitempty"`
	KeywordList []string `json:"keyword,omitempty"`
	Language    string   `json:"language,omitempty"`
	ImageUrl    string   `json:"image_url,omitempty"`
	Crawler     bool     `json:"crawler,omitempty"`
	Extract     bool     `json:"extract,omitempty"`

	Starred     bool   `json:"starred,omitempty"`
	Saved       bool   `json:"saved,omitempty"`
	Unread      bool   `json:"unread,omitempty"`
	Readlater   bool   `json:"readlater,omitempty"`
	Disabled    bool   `json:"disabled,omitempty"`
	RawContent  string `json:"raw_content,omitempty"`
	FullContent string `json:"full_content,omitempty"`

	FileType string `json:"file_type,omitempty"`
}

type EntryDelModel struct {
	EntryUrls []string `json:"entry_urls"`
}

type EntryAddResponseModel struct {
	ID     string `json:"id,omitempty" `
	Source string `json:"source"`
	Url    string `json:"url"`
}

type MongoEntryApiResponseModel struct {
	Code    int                     `json:"code"`
	Message string                  `json:"message"`
	Data    []EntryAddResponseModel `json:"data"`
}

type KnowledgeApiResponseModel struct {
	Code    int      `json:"code"`
	Message string   `json:"message"`
	Data    []string `json:"data"`
}

type AlgorithmAddModel struct {
	Source string                 `json:"source"`
	Entry  string                 `json:"entry"`
	Ranked bool                   `json:"ranked"`
	Extra  map[string]interface{} `json:"extra"`
}

func GetAddAlgorithmModel(entryID string, recallPoint, prerankPoing float32, embedding []float32) *AlgorithmAddModel {
	var model AlgorithmAddModel
	model.Source = common.GetAlgorithmSource()
	model.Entry = entryID
	model.Ranked = false
	//model.Score = prerankPoing

	extraMap := make(map[string]interface{}, 0)
	//extraMap["reall_score"] = recallPoint
	extraMap["prerank_score"] = prerankPoing
	extraMap["embedding"] = embedding

	model.Extra = extraMap

	_, err := json.Marshal(model)
	if err != nil {
		common.Logger.Error("add algorith json marshal  fail", zap.String("entryID", entryID), zap.Float32("recallPoint", recallPoint), zap.Float32("prerankPoing", prerankPoing), zap.Error(err))
	}

	return &model

}

type EntryApiDataResponseModel struct {
	Count int             `json:"count"`
	Items []EntryAddModel `json:"items"`
}

type EntryApiResponseModel struct {
	Code    int                       `json:"code"`
	Message string                    `json:"message"`
	Data    EntryApiDataResponseModel `json:"data"`
}

func GetEntryModel(protoEntity *protobuf_entity.Entry) *EntryModel {
	var result EntryModel
	result.Url = protoEntity.Url
	result.Title = protoEntity.Title
	result.CreatedAt = protoEntity.CreatedAt
	result.PublishedAt = protoEntity.PublishedAt
	result.Author = protoEntity.Author
	result.FeedUrl = protoEntity.FeedUrl
	result.ImageUrl = protoEntity.ImageUrl
	result.KeywordList = protoEntity.KeywordList
	result.Language = protoEntity.Language
	result.Embedding = protoEntity.Embedding
	result.RecallPoint = float32(protoEntity.RecallPoint)
	result.PublishedAtTimestamp = protoEntity.PublishedAtTimestamp
	return &result
}

func GetEntryAddModel(entryModel *EntryModel) *EntryAddModel {
	var result EntryAddModel
	result.Url = entryModel.Url
	result.Title = entryModel.Title
	result.PublishedAt = entryModel.PublishedAtTimestamp
	result.Author = entryModel.Author
	result.FeedUrl = entryModel.FeedUrl
	result.ImageUrl = entryModel.ImageUrl
	result.KeywordList = entryModel.KeywordList
	result.Language = entryModel.Language
	result.FileType = "article"
	result.Source = common.GetAlgorithmSource()
	//result.Crawler = false
	result.Extract = false
	result.Readlater = false
	result.Starred = false
	result.Disabled = false
	result.Saved = false
	result.Unread = true
	return &result
}
