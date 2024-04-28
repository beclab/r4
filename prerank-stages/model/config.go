package model

type AlgorithmConfig struct {
	RecallItemNum  int       `json:"recall_item_num"`
	PrerankItemNum int       `json:"prerank_item_num"`
	Embedding      []float32 `json:"embedding"`
}
