package common

import (
	"context"
	"log"
	"os"

	"github.com/redis/go-redis/v9"
)

var RedisCtx = context.Background()

func GetRDBClient() *redis.Client {
	add := os.Getenv("TERMINUS_RECOMMEND_REDIS_ADDR")
	password := os.Getenv("TERMINUS_RECOMMEND_REDIS_PASSOWRD")
	log.Printf("redis connection uri:%s,password:%s", add, password)
	var rdb = redis.NewClient(&redis.Options{
		Addr:     add,
		Password: password,
		//Username: os.Getenv("TERMINUS_RECOMMEND_REDIS_USERNAME"),
		DB: 0,
	})
	return rdb

}
