package common

import (
	"fmt"
	"os"
	"path/filepath"
	"strconv"
)

const (
	defaultSyncProvider  = "bytetrade"
	defaultSyncFeedName  = "zhcntech"
	defaultSyncModelName = "bert_v3"

	FeedPathPrefix   = "/feed/"
	EntryPathPrefix  = "/entry/"
	RecallPathPrefix = "/recall/"

	defaultSupportLanguage = "zh-cn"
	defaultSource          = "algo"
	defaultTimeliness      = "all"
	defaultKnowledgeApiUrl = "http://localhost:3010"

	defaultWeChatEntryContentUrl = "http://127.0.0.1:8080/api/wechat/entry/content"
	defaultNfSDir                = "/Users/simon/Desktop/workspace/pp/apps/rss-termius-v2/recommend_protocol/data1/nfs"
	defaultJUICEFSDir            = "/Users/simon/Desktop/workspace/pp/apps/rss-termius-v2/recommend_protocol/data1/juicefs"
)

func GetSyncProvider() string {
	source := os.Getenv("SYNC_PROVIDER")
	if source == "" {
		return defaultSyncProvider
	}
	return source
}

func GetSyncFeedName() string {
	source := os.Getenv("SYNC_FEED_NAME")
	if source == "" {
		return defaultSyncFeedName
	}
	return source
}

func GetSyncModelName() string {
	source := os.Getenv("SYNC_MODEL_NAME")
	if source == "" {
		return defaultSyncModelName
	}
	return source
}

func GetAlgorithmSource() string {
	source := os.Getenv("TERMINUS_RECOMMEND_SOURCE_NAME")
	if source == "" {
		return defaultSource
	}
	return source
}

func GetFeedSyncDataRedisKey() string {
	return "feed_sync_data"
}

func GetEntrySyncDataRedisKey() string {
	return "entry_sync_data"
}

func GetSupportTimeliness() int {
	return ParseInt(os.Getenv("SUPPORT_TIMELINESS"), 0)
}

func GetBflUser() string {
	return os.Getenv("BFL_USER")
}

func GetSupportTimelinessShow() string {
	timeliness := GetSupportTimeliness()
	if timeliness == 0 {
		return defaultTimeliness
	}
	return strconv.Itoa(timeliness)
}

func GetSupportLanguage() string {
	languages := os.Getenv("SUPPORT_LANGUAGE")
	if languages == "" {
		return defaultSupportLanguage
	}
	return languages
}

func NFSRootDirectory() string {
	envDir := os.Getenv("NFS_ROOT_DIRECTORY")
	if envDir == "" {
		return defaultNfSDir
	}
	return envDir
}

func JUICEFSRootDirectory() string {

	envDir := os.Getenv("JUICEFS_ROOT_DIRECTORY")
	if envDir == "" {
		return defaultJUICEFSDir
	}
	return envDir
}

func knowledgeBaseUrl() string {
	env := os.Getenv("KNOWLEDGE_BASE_API_URL")
	if env == "" {
		return defaultKnowledgeApiUrl
	}
	return env
}

func FeedMonogoApiUrl() string {
	return knowledgeBaseUrl() + "/knowledge/feed/algorithm/"
}

func AlgorithMonogoApiUrl() string {
	return knowledgeBaseUrl() + "/knowledge/algorithm/"
}

func EntryMonogoEntryApiUrl() string {
	return knowledgeBaseUrl() + "/knowledge/entry/"
}

func EntryAlgorithmMonogoApiUrl() string {
	return knowledgeBaseUrl() + "/knowledge/entry/algorithm/"
}

func RedisConfigApiUrl() string {
	return knowledgeBaseUrl() + "/knowledge/config/"
}

func WeChatEntryContentApiUrl() string {
	env := os.Getenv("WECHAT_ENTRY_CONTENT_GET_API_URL")
	if env == "" {
		return defaultWeChatEntryContentUrl
	}
	return env
}

func SyncFeedDirectory(provider, packageName string) string {
	path := filepath.Join(JUICEFSRootDirectory(), FeedPathPrefix, provider, packageName)
	return path
}

func SyncEntryDirectory(provider, feedName, modelName string) string {
	path := filepath.Join(JUICEFSRootDirectory(), EntryPathPrefix, provider, feedName, modelName)
	return path
}

func RecallDirectory(source string) string {
	path := filepath.Join(NFSRootDirectory(), RecallPathPrefix, source)
	return path
}

func RecallFilePath(source, language string) string {
	path := filepath.Join(NFSRootDirectory(), RecallPathPrefix, source, fmt.Sprintf("%s.recall", language))
	return path
}

func ParseInt(value string, defaultV int) int {
	if value == "" {
		return defaultV
	}

	v, err := strconv.Atoi(value)
	if err != nil {
		return defaultV
	}
	return v
}
