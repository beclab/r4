package model

type RedisConfig struct {
	Value interface{} `json:"value"`
}

type RedisConfigResponseModel struct {
	Code    int         `json:"code"`
	Message string      `json:"message"`
	Data    interface{} `json:"data"`
}
