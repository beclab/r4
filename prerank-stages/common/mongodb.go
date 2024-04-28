package common

import (
	"context"
	"log"
	"os"

	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
	"go.uber.org/zap"
)

const (
	defaultMongoDbName          = "document"
	defaultMongoEntryCollection = "entries"
	defaultMongoFeedCollection  = "feeds"
)

func MONGO_DB_NAME() string {
	envDir := os.Getenv("TERMINUS_RECOMMEND_MONGODB_NAME")
	if envDir == "" {
		log.Printf("mongo use default db name :%s", defaultMongoDbName)
		return defaultMongoDbName
	}
	//log.Printf("mongo db name :%s", envDir)
	return envDir
}
func FEED_COLLECTION_NAME() string {
	envDir := os.Getenv("MONGODB_FEED_COLLECTION_NAME")
	if envDir == "" {
		return defaultMongoFeedCollection
	}
	return envDir
}
func ENTRY_COLLECTION_NAME() string {
	envDir := os.Getenv("MONGODB_ENTRY_COLLECTION_NAME")
	if envDir == "" {
		return defaultMongoEntryCollection
	}
	return envDir
}

func NewMongodbConnection() (*mongo.Client, error, func() error) {
	uri := os.Getenv("TERMINUS_RECOMMEND_MONGODB_URI")
	if uri == "" {
		uri = "mongodb://localhost:27017"
	}
	log.Printf("mongodb connection uri:%s", uri)
	var mongoClient *mongo.Client
	var connectMongoClientError error

	mongoClient, connectMongoClientError = mongo.Connect(context.Background(), options.Client().ApplyURI(uri))
	if connectMongoClientError != nil {
		return nil, connectMongoClientError, nil
	}

	if connectMongoClientError = mongoClient.Ping(context.Background(), nil); connectMongoClientError != nil {
		return nil, connectMongoClientError, nil
	}

	disconnectMongoClientFunc := func() error {
		if disconnectErr := mongoClient.Disconnect(context.TODO()); disconnectErr != nil {
			Logger.Error("disconnect mongo client error", zap.Error(disconnectErr))
			return disconnectErr
		}
		return nil
	}
	return mongoClient, connectMongoClientError, disconnectMongoClientFunc
}
